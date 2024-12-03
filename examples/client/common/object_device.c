/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    domedambrosio - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>

*/

/*
 * This object is single instance only, and is mandatory to all LWM2M device as it describe the object such as its
 * manufacturer, model, etc...
 */

#include "liblwm2m.h"
#include "lwm2mclient.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PRV_MANUFACTURER "Open Mobile Alliance"
#define PRV_MODEL_NUMBER "Lightweight M2M Client"
#define PRV_SERIAL_NUMBER "345000123"
#define PRV_FIRMWARE_VERSION "1.0"
#define PRV_POWER_SOURCE_1 1
#define PRV_POWER_SOURCE_2 5
#define PRV_POWER_VOLTAGE_1 3800
#define PRV_POWER_VOLTAGE_2 5000
#define PRV_POWER_CURRENT_1 125
#define PRV_POWER_CURRENT_2 900
#define PRV_BATTERY_LEVEL 100
#define PRV_MEMORY_FREE 15
#define PRV_ERROR_CODE 0
#define PRV_TIME_ZONE "Europe/Berlin"
#define PRV_BINDING_MODE "U"

#define PRV_OFFSET_MAXLEN 7 //+HH:MM\0 at max
#define PRV_TLV_BUFFER_SIZE 128

// Resource Id's:
#define RES_O_MANUFACTURER 0
#define RES_O_MODEL_NUMBER 1
#define RES_O_SERIAL_NUMBER 2
#define RES_O_FIRMWARE_VERSION 3
#define RES_M_REBOOT 4
#define RES_O_FACTORY_RESET 5
#define RES_O_AVL_POWER_SOURCES 6
#define RES_O_POWER_SOURCE_VOLTAGE 7
#define RES_O_POWER_SOURCE_CURRENT 8
#define RES_O_BATTERY_LEVEL 9
#define RES_O_MEMORY_FREE 10
#define RES_M_ERROR_CODE 11
#define RES_O_RESET_ERROR_CODE 12
#define RES_O_CURRENT_TIME 13
#define RES_O_UTC_OFFSET 14
#define RES_O_TIMEZONE 15
#define RES_M_BINDING_MODES 16
// since TS 20141126-C:
#define RES_O_DEVICE_TYPE 17
#define RES_O_HARDWARE_VERSION 18
#define RES_O_SOFTWARE_VERSION 19
#define RES_O_BATTERY_STATUS 20
#define RES_O_MEMORY_TOTAL 21

typedef struct {
    int64_t free_memory;
    int64_t error;
    int64_t time;
    uint8_t battery_level;
    char time_offset[PRV_OFFSET_MAXLEN];
} device_data_t;

// basic check that the time offset value is at ISO 8601 format
// bug: +12:30 is considered a valid value by this function
static int prv_check_time_offset(char *buffer, int length) {
    int min_index;

    if (length != 3 && length != 5 && length != 6)
        return 0;
    if (buffer[0] != '-' && buffer[0] != '+')
        return 0;
    switch (buffer[1]) {
    case '0':
        if (buffer[2] < '0' || buffer[2] > '9')
            return 0;
        break;
    case '1':
        if (buffer[2] < '0' || (buffer[0] == '-' && buffer[2] > '2') || (buffer[0] == '+' && buffer[2] > '4'))
            return 0;
        break;
    default:
        return 0;
    }
    switch (length) {
    case 3:
        return 1;
    case 5:
        min_index = 3;
        break;
    case 6:
        if (buffer[3] != ':')
            return 0;
        min_index = 4;
        break;
    default:
        // never happen
        return 0;
    }
    if (buffer[min_index] < '0' || buffer[min_index] > '5')
        return 0;
    if (buffer[min_index + 1] < '0' || buffer[min_index + 1] > '9')
        return 0;

    return 1;
}

static uint8_t prv_set_value(lwm2m_data_t *dataP, device_data_t *devDataP) {
    lwm2m_data_t *subTlvP;
    size_t count;
    size_t i;
    // a simple switch structure is used to respond at the specified resource asked
    switch (dataP->id) {
    case RES_O_MANUFACTURER:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(PRV_MANUFACTURER, dataP);
        return COAP_205_CONTENT;

    case RES_O_MODEL_NUMBER:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(PRV_MODEL_NUMBER, dataP);
        return COAP_205_CONTENT;

    case RES_O_SERIAL_NUMBER:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(PRV_SERIAL_NUMBER, dataP);
        return COAP_205_CONTENT;

    case RES_O_FIRMWARE_VERSION:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(PRV_FIRMWARE_VERSION, dataP);
        return COAP_205_CONTENT;

    case RES_M_REBOOT:
        return COAP_405_METHOD_NOT_ALLOWED;

    case RES_O_FACTORY_RESET:
        return COAP_405_METHOD_NOT_ALLOWED;

    case RES_O_AVL_POWER_SOURCES: {
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            count = dataP->value.asChildren.count;
            subTlvP = dataP->value.asChildren.array;
        } else {
            count = 2;
            subTlvP = lwm2m_data_new(count);
            for (i = 0; i < count; i++)
                subTlvP[i].id = i;
            lwm2m_data_encode_instances(subTlvP, count, dataP);
        }

        for (i = 0; i < count; i++) {
            switch (subTlvP[i].id) {
            case 0:
                lwm2m_data_encode_int(PRV_POWER_SOURCE_1, subTlvP + i);
                break;
            case 1:
                lwm2m_data_encode_int(PRV_POWER_SOURCE_2, subTlvP + i);
                break;
            default:
                return COAP_404_NOT_FOUND;
            }
        }

        return COAP_205_CONTENT;
    }

    case RES_O_POWER_SOURCE_VOLTAGE: {
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            count = dataP->value.asChildren.count;
            subTlvP = dataP->value.asChildren.array;
        } else {
            count = 2;
            subTlvP = lwm2m_data_new(count);
            for (i = 0; i < count; i++)
                subTlvP[i].id = i;
            lwm2m_data_encode_instances(subTlvP, count, dataP);
        }

        for (i = 0; i < count; i++) {
            switch (subTlvP[i].id) {
            case 0:
                lwm2m_data_encode_int(PRV_POWER_VOLTAGE_1, subTlvP + i);
                break;
            case 1:
                lwm2m_data_encode_int(PRV_POWER_VOLTAGE_2, subTlvP + i);
                break;
            default:
                return COAP_404_NOT_FOUND;
            }
        }

        return COAP_205_CONTENT;
    }

    case RES_O_POWER_SOURCE_CURRENT: {
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            count = dataP->value.asChildren.count;
            subTlvP = dataP->value.asChildren.array;
        } else {
            count = 2;
            subTlvP = lwm2m_data_new(count);
            for (i = 0; i < count; i++)
                subTlvP[i].id = i;
            lwm2m_data_encode_instances(subTlvP, count, dataP);
        }

        for (i = 0; i < count; i++) {
            switch (subTlvP[i].id) {
            case 0:
                lwm2m_data_encode_int(PRV_POWER_CURRENT_1, subTlvP + i);
                break;
            case 1:
                lwm2m_data_encode_int(PRV_POWER_CURRENT_2, subTlvP + i);
                break;
            default:
                return COAP_404_NOT_FOUND;
            }
        }

        return COAP_205_CONTENT;
    }

    case RES_O_BATTERY_LEVEL:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_int(devDataP->battery_level, dataP);
        return COAP_205_CONTENT;

    case RES_O_MEMORY_FREE:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_int(devDataP->free_memory, dataP);
        return COAP_205_CONTENT;

    case RES_M_ERROR_CODE: {
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            count = dataP->value.asChildren.count;
            subTlvP = dataP->value.asChildren.array;
        } else {
            count = 1;
            subTlvP = lwm2m_data_new(count);
            for (i = 0; i < count; i++)
                subTlvP[i].id = i;
            lwm2m_data_encode_instances(subTlvP, count, dataP);
        }

        for (i = 0; i < count; i++) {
            switch (subTlvP[i].id) {
            case 0:
                lwm2m_data_encode_int(devDataP->error, subTlvP + i);
                break;
            default:
                return COAP_404_NOT_FOUND;
            }
        }

        return COAP_205_CONTENT;
    }
    case RES_O_RESET_ERROR_CODE:
        return COAP_405_METHOD_NOT_ALLOWED;

    case RES_O_CURRENT_TIME:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_int(time(NULL) + devDataP->time, dataP);
        return COAP_205_CONTENT;

    case RES_O_UTC_OFFSET:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(devDataP->time_offset, dataP);
        return COAP_205_CONTENT;

    case RES_O_TIMEZONE:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(PRV_TIME_ZONE, dataP);
        return COAP_205_CONTENT;

    case RES_M_BINDING_MODES:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(PRV_BINDING_MODE, dataP);
        return COAP_205_CONTENT;

    default:
        return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_device_read(lwm2m_context_t *contextP, uint16_t instanceId, int *numDataP, lwm2m_data_t **dataArrayP,
                               lwm2m_object_t *objectP) {
    uint8_t result;
    int i;

    /* unused parameter */
    (void)contextP;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0) {
        uint16_t resList[] = {RES_O_MANUFACTURER, RES_O_MODEL_NUMBER, RES_O_SERIAL_NUMBER, RES_O_FIRMWARE_VERSION,
                              // E: RES_M_REBOOT,
                              // E: RES_O_FACTORY_RESET,
                              RES_O_AVL_POWER_SOURCES, RES_O_POWER_SOURCE_VOLTAGE, RES_O_POWER_SOURCE_CURRENT,
                              RES_O_BATTERY_LEVEL, RES_O_MEMORY_FREE, RES_M_ERROR_CODE,
                              // E: RES_O_RESET_ERROR_CODE,
                              RES_O_CURRENT_TIME, RES_O_UTC_OFFSET, RES_O_TIMEZONE, RES_M_BINDING_MODES};
        int nbRes = sizeof(resList) / sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0; i < nbRes; i++) {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do {
        result = prv_set_value((*dataArrayP) + i, (device_data_t *)(objectP->userData));
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

static uint8_t prv_device_discover(lwm2m_context_t *contextP, uint16_t instanceId, int *numDataP,
                                   lwm2m_data_t **dataArrayP, lwm2m_object_t *objectP) {
    uint8_t result;
    int i;

    /* unused parameter */
    (void)contextP;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    result = COAP_205_CONTENT;

    // is the server asking for the full object ?
    if (*numDataP == 0) {
        uint16_t resList[] = {RES_O_MANUFACTURER,
                              RES_O_MODEL_NUMBER,
                              RES_O_SERIAL_NUMBER,
                              RES_O_FIRMWARE_VERSION,
                              RES_M_REBOOT,
                              RES_O_FACTORY_RESET,
                              RES_O_AVL_POWER_SOURCES,
                              RES_O_POWER_SOURCE_VOLTAGE,
                              RES_O_POWER_SOURCE_CURRENT,
                              RES_O_BATTERY_LEVEL,
                              RES_O_MEMORY_FREE,
                              RES_M_ERROR_CODE,
                              RES_O_RESET_ERROR_CODE,
                              RES_O_CURRENT_TIME,
                              RES_O_UTC_OFFSET,
                              RES_O_TIMEZONE,
                              RES_M_BINDING_MODES};
        int nbRes = sizeof(resList) / sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0; i < nbRes; i++) {
            (*dataArrayP)[i].id = resList[i];
        }
    } else {
        for (i = 0; i < *numDataP && result == COAP_205_CONTENT; i++) {
            switch ((*dataArrayP)[i].id) {
            case RES_O_MANUFACTURER:
            case RES_O_MODEL_NUMBER:
            case RES_O_SERIAL_NUMBER:
            case RES_O_FIRMWARE_VERSION:
            case RES_M_REBOOT:
            case RES_O_FACTORY_RESET:
            case RES_O_AVL_POWER_SOURCES:
            case RES_O_POWER_SOURCE_VOLTAGE:
            case RES_O_POWER_SOURCE_CURRENT:
            case RES_O_BATTERY_LEVEL:
            case RES_O_MEMORY_FREE:
            case RES_M_ERROR_CODE:
            case RES_O_RESET_ERROR_CODE:
            case RES_O_CURRENT_TIME:
            case RES_O_UTC_OFFSET:
            case RES_O_TIMEZONE:
            case RES_M_BINDING_MODES:
                break;
            default:
                result = COAP_404_NOT_FOUND;
            }
        }
    }

    return result;
}

static uint8_t prv_device_write(lwm2m_context_t *contextP, uint16_t instanceId, int numData, lwm2m_data_t *dataArray,
                                lwm2m_object_t *objectP, lwm2m_write_type_t writeType) {
    int i;
    uint8_t result;

    /* unused parameter */
    (void)contextP;

    // All write types are treated the same here
    (void)writeType;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    i = 0;

    do {
        /* No multiple instance resources */
        if (dataArray[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            result = COAP_404_NOT_FOUND;
            continue;
        }

        switch (dataArray[i].id) {
        case RES_O_CURRENT_TIME:
            if (1 == lwm2m_data_decode_int(dataArray + i, &((device_data_t *)(objectP->userData))->time)) {
                ((device_data_t *)(objectP->userData))->time -= time(NULL);
                result = COAP_204_CHANGED;
            } else {
                result = COAP_400_BAD_REQUEST;
            }
            break;

        case RES_O_UTC_OFFSET:
            if (1 ==
                prv_check_time_offset((char *)dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length)) {
                strncpy(((device_data_t *)(objectP->userData))->time_offset, // NOSONAR
                        (char *)dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length);
                ((device_data_t *)(objectP->userData))->time_offset[dataArray[i].value.asBuffer.length] = 0;
                result = COAP_204_CHANGED;
            } else {
                result = COAP_400_BAD_REQUEST;
            }
            break;

        case RES_O_TIMEZONE:
            // ToDo IANA TZ Format
            result = COAP_501_NOT_IMPLEMENTED;
            break;

        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
        }

        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

static uint8_t prv_device_execute(lwm2m_context_t *contextP, uint16_t instanceId, uint16_t resourceId, uint8_t *buffer,
                                  int length, lwm2m_object_t *objectP) {
    /* unused parameter */
    (void)contextP;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    if (length != 0)
        return COAP_400_BAD_REQUEST;

    switch (resourceId) {
    case RES_M_REBOOT:
        fprintf(stdout, "\n\t REBOOT\r\n\n");
        g_reboot = 1;
        return COAP_204_CHANGED;
    case RES_O_FACTORY_RESET:
        fprintf(stdout, "\n\t FACTORY RESET\r\n\n");
        return COAP_204_CHANGED;
    case RES_O_RESET_ERROR_CODE:
        fprintf(stdout, "\n\t RESET ERROR CODE\r\n\n");
        ((device_data_t *)(objectP->userData))->error = 0;
        return COAP_204_CHANGED;
    default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

void display_device_object(lwm2m_object_t *object) {
    device_data_t *data = (device_data_t *)object->userData;
    fprintf(stdout, "  /%u: Device object:\r\n", object->objID);
    if (NULL != data) {
        fprintf(stdout, "    time: %lld, time_offset: %s\r\n", (long long)data->time, data->time_offset);
    }
}

lwm2m_object_t *get_object_device(void) {
    /*
     * The get_object_device function create the object itself and return a pointer to the structure that represent it.
     */
    lwm2m_object_t *deviceObj;

    deviceObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != deviceObj) {
        memset(deviceObj, 0, sizeof(lwm2m_object_t));

        /*
         * It assigns his unique ID
         * The 3 is the standard ID for the mandatory object "Object device".
         */
        deviceObj->objID = LWM2M_DEVICE_OBJECT_ID;

        /*
         * and its unique instance
         *
         */
        deviceObj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != deviceObj->instanceList) {
            memset(deviceObj->instanceList, 0, sizeof(lwm2m_list_t));
        } else {
            lwm2m_free(deviceObj);
            return NULL;
        }

        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library
         * don't need to know the resources of the object, only the server does.
         */
        deviceObj->readFunc = prv_device_read;
        deviceObj->discoverFunc = prv_device_discover;
        deviceObj->writeFunc = prv_device_write;
        deviceObj->executeFunc = prv_device_execute;
        deviceObj->userData = lwm2m_malloc(sizeof(device_data_t));

        /*
         * Also some user data can be stored in the object with a private structure containing the needed variables
         */
        if (NULL != deviceObj->userData) {
            ((device_data_t *)deviceObj->userData)->battery_level = PRV_BATTERY_LEVEL;
            ((device_data_t *)deviceObj->userData)->free_memory = PRV_MEMORY_FREE;
            ((device_data_t *)deviceObj->userData)->error = PRV_ERROR_CODE;
            ((device_data_t *)deviceObj->userData)->time = 1367491215;
            strcpy(((device_data_t *)deviceObj->userData)->time_offset, "+01:00"); // NOSONAR
        } else {
            lwm2m_free(deviceObj->instanceList);
            lwm2m_free(deviceObj);
            deviceObj = NULL;
        }
    }

    return deviceObj;
}

void free_object_device(lwm2m_object_t *objectP) {
    if (NULL != objectP->userData) {
        lwm2m_free(objectP->userData);
        objectP->userData = NULL;
    }
    if (NULL != objectP->instanceList) {
        lwm2m_free(objectP->instanceList);
        objectP->instanceList = NULL;
    }

    lwm2m_free(objectP);
}

uint8_t device_change(lwm2m_data_t *dataArray, lwm2m_object_t *objectP) {
    uint8_t result;

    switch (dataArray->id) {
    case RES_O_BATTERY_LEVEL: {
        int64_t value;
        if (1 == lwm2m_data_decode_int(dataArray, &value)) {
            if ((0 <= value) && (100 >= value)) {
                ((device_data_t *)(objectP->userData))->battery_level = value;
                result = COAP_204_CHANGED;
            } else {
                result = COAP_400_BAD_REQUEST;
            }
        } else {
            result = COAP_400_BAD_REQUEST;
        }
    } break;
    case RES_M_ERROR_CODE:
        if (1 == lwm2m_data_decode_int(dataArray, &((device_data_t *)(objectP->userData))->error)) {
            result = COAP_204_CHANGED;
        } else {
            result = COAP_400_BAD_REQUEST;
        }
        break;
    case RES_O_MEMORY_FREE:
        if (1 == lwm2m_data_decode_int(dataArray, &((device_data_t *)(objectP->userData))->free_memory)) {
            result = COAP_204_CHANGED;
        } else {
            result = COAP_400_BAD_REQUEST;
        }
        break;
    default:
        result = COAP_405_METHOD_NOT_ALLOWED;
        break;
    }

    return result;
}
