/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    domedambrosio - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Axel Lorente - Please refer to git log
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define PRV_MANUFACTURER      "Open Mobile Alliance"
#define PRV_MODEL_NUMBER      "Lightweight M2M Client"
#define PRV_SERIAL_NUMBER     "345000123"
#define PRV_FIRMWARE_VERSION  "1.0"
#define PRV_POWER_SOURCE_1    1
#define PRV_POWER_SOURCE_2    5
#define PRV_POWER_VOLTAGE_1   3800
#define PRV_POWER_VOLTAGE_2   5000
#define PRV_POWER_CURRENT_1   125
#define PRV_POWER_CURRENT_2   900
#define PRV_BATTERY_LEVEL     100
#define PRV_MEMORY_FREE       15
#define PRV_ERROR_CODE        0
#define PRV_BINDING_MODE      "U"

#define PRV_OFFSET_MAXLEN   7 //+HH:MM\0 at max
#define PRV_TLV_BUFFER_SIZE 128

typedef struct
{
    int64_t time;
    char time_offset[PRV_OFFSET_MAXLEN];
} device_data_t;


// basic check that the time offset value is at ISO 8601 format
// bug: +12:30 is considered a valid value by this function
static int prv_check_time_offset(char * buffer,
                                 int length)
{
    int min_index;

    if (length != 3 && length != 5 && length != 6) return 0;
    if (buffer[0] != '-' && buffer[0] != '+') return 0;
    switch (buffer[1])
    {
    case '0':
        if (buffer[2] < '0' || buffer[2] > '9') return 0;
        break;
    case '1':
        if (buffer[2] < '0' || buffer[2] > '2') return 0;
        break;
    default:
        return 0;
    }
    switch (length)
    {
    case 3:
        return 1;
    case 5:
        min_index = 3;
        break;
    case 6:
        if (buffer[3] != ':') return 0;
        min_index = 4;
        break;
    default:
        // never happen
        return 0;
    }
    if (buffer[min_index] < '0' || buffer[min_index] > '5') return 0;
    if (buffer[min_index+1] < '0' || buffer[min_index+1] > '9') return 0;

    return 1;
}

static uint8_t prv_set_value(lwm2m_tlv_t * tlvP,
                             device_data_t * devDataP)
{
    // a simple switch structure is used to respond at the specified resource asked
    switch (tlvP->id)
    {
    case 0:
        tlvP->value = PRV_MANUFACTURER;
        tlvP->length = strlen(PRV_MANUFACTURER);
        tlvP->flags = LWM2M_TLV_FLAG_STATIC_DATA;
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        return COAP_205_CONTENT;

    case 1:
        tlvP->value = PRV_MODEL_NUMBER;
        tlvP->length = strlen(PRV_MODEL_NUMBER);
        tlvP->flags = LWM2M_TLV_FLAG_STATIC_DATA;
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        return COAP_205_CONTENT;

    case 2:
        tlvP->value = PRV_SERIAL_NUMBER;
        tlvP->length = strlen(PRV_SERIAL_NUMBER);
        tlvP->flags = LWM2M_TLV_FLAG_STATIC_DATA;
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        return COAP_205_CONTENT;

    case 3:
        tlvP->value = PRV_FIRMWARE_VERSION;
        tlvP->length = strlen(PRV_FIRMWARE_VERSION);
        tlvP->flags = LWM2M_TLV_FLAG_STATIC_DATA;
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        return COAP_205_CONTENT;

    case 4:
        return COAP_405_METHOD_NOT_ALLOWED;

    case 5:
        return COAP_405_METHOD_NOT_ALLOWED;

    case 6:
    {
        lwm2m_tlv_t * subTlvP;

        subTlvP = lwm2m_tlv_new(2);

        subTlvP[0].flags = 0;
        subTlvP[0].id = 0;
        subTlvP[0].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        lwm2m_tlv_encode_int(PRV_POWER_SOURCE_1, subTlvP);
        if (0 == subTlvP[0].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        subTlvP[1].flags = 0;
        subTlvP[1].id = 1;
        subTlvP[1].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        lwm2m_tlv_encode_int(PRV_POWER_SOURCE_2, subTlvP + 1);
        if (0 == subTlvP[1].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        tlvP->flags = 0;
        tlvP->type = LWM2M_TYPE_MULTIPLE_RESSOURCE;
        tlvP->length = 2;
        tlvP->value = (uint8_t *)subTlvP;

        return COAP_205_CONTENT;
    }

    case 7:
    {
        lwm2m_tlv_t * subTlvP;

        subTlvP = lwm2m_tlv_new(2);

        subTlvP[0].flags = 0;
        subTlvP[0].id = 0;
        subTlvP[0].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        lwm2m_tlv_encode_int(PRV_POWER_VOLTAGE_1, subTlvP);
        if (0 == subTlvP[0].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        subTlvP[1].flags = 0;
        subTlvP[1].id = 1;
        subTlvP[1].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        lwm2m_tlv_encode_int(PRV_POWER_VOLTAGE_2, subTlvP + 1);
        if (0 == subTlvP[1].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        tlvP->flags = 0;
        tlvP->type = LWM2M_TYPE_MULTIPLE_RESSOURCE;
        tlvP->length = 2;
        tlvP->value = (uint8_t *)subTlvP;

        return COAP_205_CONTENT;
    }

    case 8:
    {
        lwm2m_tlv_t * subTlvP;

        subTlvP = lwm2m_tlv_new(2);

        subTlvP[0].flags = 0;
        subTlvP[0].id = 0;
        subTlvP[0].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        lwm2m_tlv_encode_int(PRV_POWER_CURRENT_1, subTlvP);
        if (0 == subTlvP[0].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        subTlvP[1].flags = 0;
        subTlvP[1].id = 1;
        subTlvP[1].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
        lwm2m_tlv_encode_int(PRV_POWER_CURRENT_2, subTlvP + 1);
        if (0 == subTlvP[1].length)
        {
            lwm2m_tlv_free(2, subTlvP);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        tlvP->flags = 0;
        tlvP->type = LWM2M_TYPE_MULTIPLE_RESSOURCE;
        tlvP->length = 2;
        tlvP->value = (uint8_t *)subTlvP;

        return COAP_205_CONTENT;
    }

    case 9:
        lwm2m_tlv_encode_int(PRV_BATTERY_LEVEL, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;

        if (0 != tlvP->length) return COAP_205_CONTENT;
        else return COAP_500_INTERNAL_SERVER_ERROR;

    case 10:
        lwm2m_tlv_encode_int(PRV_MEMORY_FREE, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;

        if (0 != tlvP->length) return COAP_205_CONTENT;
        else return COAP_500_INTERNAL_SERVER_ERROR;

    case 11:
        lwm2m_tlv_encode_int(PRV_ERROR_CODE, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;

        if (0 != tlvP->length) return COAP_205_CONTENT;
        else return COAP_500_INTERNAL_SERVER_ERROR;

    case 12:
        return COAP_405_METHOD_NOT_ALLOWED;

    case 13:
        lwm2m_tlv_encode_int(devDataP->time, tlvP);
        tlvP->type = LWM2M_TYPE_RESSOURCE;

        if (0 != tlvP->length) return COAP_205_CONTENT;
        else return COAP_500_INTERNAL_SERVER_ERROR;

    case 14:
        tlvP->value = devDataP->time_offset;
        tlvP->length = strlen(devDataP->time_offset);
        tlvP->flags = LWM2M_TLV_FLAG_STATIC_DATA;
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        return COAP_205_CONTENT;

    case 15:
        tlvP->value = PRV_BINDING_MODE;
        tlvP->length = strlen(PRV_BINDING_MODE);
        tlvP->flags = LWM2M_TLV_FLAG_STATIC_DATA;
        tlvP->type = LWM2M_TYPE_RESSOURCE;
        return COAP_205_CONTENT;

    default:
        return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_device_read(uint16_t instanceId,
                               int * numDataP,
                               lwm2m_tlv_t ** dataArrayP,
                               lwm2m_object_t * objectP)
{
    uint8_t result;
    int i;

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 13, 14, 15};
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = lwm2m_tlv_new(nbRes);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do
    {
        result = prv_set_value((*dataArrayP) + i, (device_data_t*)(objectP->userData));
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

static uint8_t prv_device_write(uint16_t instanceId,
                                int numData,
                                lwm2m_tlv_t * dataArray,
                                lwm2m_object_t * objectP)
{
    int i;
    uint8_t result;

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    i = 0;

    do
    {
        switch (dataArray[i].id)
        {
        case 13:
            if (1 == lwm2m_tlv_decode_int(dataArray + i, &((device_data_t*)(objectP->userData))->time))
            {
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;

        case 14:
            if (1 == prv_check_time_offset(dataArray[i].value, dataArray[i].length))
            {
                strncpy(((device_data_t*)(objectP->userData))->time_offset, dataArray[i].value, dataArray[i].length);
                ((device_data_t*)(objectP->userData))->time_offset[dataArray[i].length] = 0;
                result = COAP_204_CHANGED;
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;

        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
        }

        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

static uint8_t prv_device_execute(uint16_t instanceId,
                                  uint16_t resourceId,
                                  char * buffer,
                                  int length,
                                  lwm2m_object_t * objectP)
{
    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    if (length != 0) return COAP_400_BAD_REQUEST;

    switch (resourceId)
    {
    case 4:
        fprintf(stdout, "\n\t REBOOT\r\n\n");
        return COAP_204_CHANGED;
    case 5:
        fprintf(stdout, "\n\t FACTORY RESET\r\n\n");
        return COAP_204_CHANGED;
    case 12:
        fprintf(stdout, "\n\t RESET ERROR CODE\r\n\n");
        return COAP_204_CHANGED;
    default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

lwm2m_object_t * get_object_device()
{
    /*
     * The get_object_device function create the object itself and return a pointer to the structure that represent it.
     */
    lwm2m_object_t * deviceObj;

    deviceObj = (lwm2m_object_t *)malloc(sizeof(lwm2m_object_t));

    if (NULL != deviceObj)
    {
        memset(deviceObj, 0, sizeof(lwm2m_object_t));

        /*
         * It assign his unique ID
         * The 3 is the standard ID for the mandatory object "Object device".
         */
        deviceObj->objID = 3;

        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library don't need to
         * know the resources of the object, only the server does.
         */
        deviceObj->readFunc = prv_device_read;
        deviceObj->writeFunc = prv_device_write;
        deviceObj->executeFunc = prv_device_execute;
        deviceObj->userData = malloc(sizeof(device_data_t));

        /*
         * Also some user data can be stored in the object with a private structure containing the needed variables 
         */
        if (NULL != deviceObj->userData)
        {
            ((device_data_t*)deviceObj->userData)->time = 1367491215;
            strcpy(((device_data_t*)deviceObj->userData)->time_offset, "+02:00");
        }
        else
        {
            free(deviceObj);
            deviceObj = NULL;
        }
    }

    return deviceObj;
}
