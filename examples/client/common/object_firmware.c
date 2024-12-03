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
 *    Julien Vermillard - initial implementation
 *    Fabien Fleutot - Please refer to git log
 *    David Navarro, Intel Corporation - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Gregory Lemercier - Please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *
 *******************************************************************************/

/*
 * This object is single instance only, and provide firmware upgrade functionality.
 * Object ID is 5.
 */

/*
 * resources:
 * 0 package                   write
 * 1 package url               write
 * 2 update                    exec
 * 3 state                     read
 * 5 update result             read
 * 6 package name              read
 * 7 package version           read
 * 8 update protocol support   read
 * 9 update delivery method    read
 */

#include "liblwm2m.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---- private object "Firmware" specific defines ----
// Resource Id's:
#define RES_M_PACKAGE 0
#define RES_M_PACKAGE_URI 1
#define RES_M_UPDATE 2
#define RES_M_STATE 3
#define RES_M_UPDATE_RESULT 5
#define RES_O_PKG_NAME 6
#define RES_O_PKG_VERSION 7
#define RES_O_UPDATE_PROTOCOL 8
#define RES_M_UPDATE_METHOD 9

#define LWM2M_FIRMWARE_PROTOCOL_NUM 4
#define LWM2M_FIRMWARE_PROTOCOL_NULL ((uint8_t) - 1)

typedef struct {
    uint8_t state;
    uint8_t result;
    char pkg_name[256];
    char pkg_version[256];
    uint8_t protocol_support[LWM2M_FIRMWARE_PROTOCOL_NUM];
    uint8_t delivery_method;
} firmware_data_t;

static uint8_t prv_firmware_read(lwm2m_context_t *contextP, uint16_t instanceId, int *numDataP,
                                 lwm2m_data_t **dataArrayP, lwm2m_object_t *objectP) {
    int i;
    uint8_t result;
    firmware_data_t *data = (firmware_data_t *)(objectP->userData);

    /* unused parameter */
    (void)contextP;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0) {
        *dataArrayP = lwm2m_data_new(6);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 6;
        (*dataArrayP)[0].id = 3;
        (*dataArrayP)[1].id = 5;
        (*dataArrayP)[2].id = 6;
        (*dataArrayP)[3].id = 7;
        (*dataArrayP)[4].id = 8;
        (*dataArrayP)[5].id = 9;
    }

    i = 0;
    do {
        switch ((*dataArrayP)[i].id) {
        case RES_M_PACKAGE:
        case RES_M_PACKAGE_URI:
        case RES_M_UPDATE:
            result = COAP_405_METHOD_NOT_ALLOWED;
            break;

        case RES_M_STATE:
            // firmware update state (int)
            if ((*dataArrayP)[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_int(data->state, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;

        case RES_M_UPDATE_RESULT:
            if ((*dataArrayP)[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_int(data->result, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;

        case RES_O_PKG_NAME:
            if ((*dataArrayP)[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_string(data->pkg_name, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;

        case RES_O_PKG_VERSION:
            if ((*dataArrayP)[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_string(data->pkg_version, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;

        case RES_O_UPDATE_PROTOCOL: {
            lwm2m_data_t *subTlvP;
            size_t count;
            size_t ri;
            int num = 0;

            while ((num < LWM2M_FIRMWARE_PROTOCOL_NUM) && (data->protocol_support[num] != LWM2M_FIRMWARE_PROTOCOL_NULL))
                num++;

            if ((*dataArrayP)[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
                count = (*dataArrayP)[i].value.asChildren.count;
                subTlvP = (*dataArrayP)[i].value.asChildren.array;
            } else {
                count = num;
                if (!count)
                    count = 1;
                subTlvP = lwm2m_data_new(count);
                for (ri = 0; ri < count; ri++)
                    subTlvP[ri].id = ri;
                lwm2m_data_encode_instances(subTlvP, count, *dataArrayP + i);
            }

            if (num) {
                for (ri = 0; ri < count; ri++) {
                    if (subTlvP[ri].id >= num)
                        return COAP_404_NOT_FOUND;
                    lwm2m_data_encode_int(data->protocol_support[subTlvP[ri].id], subTlvP + ri);
                }
            } else {
                /* If no protocol is provided, use CoAP as default (per spec) */
                for (ri = 0; ri < count; ri++) {
                    if (subTlvP[ri].id != 0)
                        return COAP_404_NOT_FOUND;
                    lwm2m_data_encode_int(0, subTlvP + ri);
                }
            }
            result = COAP_205_CONTENT;
            break;
        }

        case RES_M_UPDATE_METHOD:
            if ((*dataArrayP)[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_int(data->delivery_method, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;

        default:
            result = COAP_404_NOT_FOUND;
        }

        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

static uint8_t prv_firmware_write(lwm2m_context_t *contextP, uint16_t instanceId, int numData, lwm2m_data_t *dataArray,
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
        case RES_M_PACKAGE:
            // inline firmware binary
            result = COAP_204_CHANGED;
            break;

        case RES_M_PACKAGE_URI:
            // URL for download the firmware
            result = COAP_204_CHANGED;
            break;

        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
        }

        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

static uint8_t prv_firmware_execute(lwm2m_context_t *contextP, uint16_t instanceId, uint16_t resourceId,
                                    uint8_t *buffer, int length, lwm2m_object_t *objectP) {
    firmware_data_t *data = (firmware_data_t *)(objectP->userData);

    /* unused parameter */
    (void)contextP;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    if (length != 0)
        return COAP_400_BAD_REQUEST;

    // for execute callback, resId is always set.
    switch (resourceId) {
    case RES_M_UPDATE:
        if (data->state == 1) {
            fprintf(stdout, "\n\t FIRMWARE UPDATE\r\n\n");
            // trigger your firmware download and update logic
            data->state = 2;
            return COAP_204_CHANGED;
        } else {
            // firmware update already running
            return COAP_400_BAD_REQUEST;
        }
    default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

#ifdef LWM2M_RAW_BLOCK1_REQUESTS
static uint8_t prv_firmware_raw_block1_write(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_media_type_t format,
                                             uint8_t *buffer, int length, lwm2m_object_t *objectP, uint32_t block_num,
                                             uint8_t block_more) {
    // a full implementation would store data to flash and update the
    // firmware upload state machine and result here.
    if (block_more == 0) {
        return COAP_204_CHANGED;
    }

    return COAP_231_CONTINUE;
}
#endif

void display_firmware_object(lwm2m_object_t *object) {
    firmware_data_t *data = (firmware_data_t *)object->userData;
    fprintf(stdout, "  /%u: Firmware object:\r\n", object->objID);
    if (NULL != data) {
        fprintf(stdout, "    state: %u, result: %u\r\n", data->state, data->result);
    }
}

lwm2m_object_t *get_object_firmware(void) {
    /*
     * The get_object_firmware function create the object itself and return a pointer to the structure that represent
     * it.
     */
    lwm2m_object_t *firmwareObj;

    firmwareObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != firmwareObj) {
        memset(firmwareObj, 0, sizeof(lwm2m_object_t));

        /*
         * It assigns its unique ID
         * The 5 is the standard ID for the optional object "Object firmware".
         */
        firmwareObj->objID = LWM2M_FIRMWARE_UPDATE_OBJECT_ID;

        /*
         * and its unique instance
         *
         */
        firmwareObj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != firmwareObj->instanceList) {
            memset(firmwareObj->instanceList, 0, sizeof(lwm2m_list_t));
        } else {
            lwm2m_free(firmwareObj);
            return NULL;
        }

        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library
         * don't need to know the resources of the object, only the server does.
         */
        firmwareObj->readFunc = prv_firmware_read;
        firmwareObj->writeFunc = prv_firmware_write;
        firmwareObj->executeFunc = prv_firmware_execute;
#ifdef LWM2M_RAW_BLOCK1_REQUESTS
        firmwareObj->rawBlock1WriteFunc = prv_firmware_raw_block1_write;
#endif
        firmwareObj->userData = lwm2m_malloc(sizeof(firmware_data_t));

        /*
         * Also some user data can be stored in the object with a private structure containing the needed variables
         */
        if (NULL != firmwareObj->userData) {
            firmware_data_t *data = (firmware_data_t *)(firmwareObj->userData);

            data->state = 1;
            data->result = 0;
            strcpy(data->pkg_name, "lwm2mclient"); // NOSONAR
            strcpy(data->pkg_version, "1.0");      // NOSONAR

            /* Only support CoAP based protocols */
            data->protocol_support[0] = 0;
            data->protocol_support[1] = 1;
            data->protocol_support[2] = LWM2M_FIRMWARE_PROTOCOL_NULL;
            data->protocol_support[3] = LWM2M_FIRMWARE_PROTOCOL_NULL;

            /* Only support push method */
            data->delivery_method = 1;
        } else {
            lwm2m_free(firmwareObj);
            firmwareObj = NULL;
        }
    }

    return firmwareObj;
}

void free_object_firmware(lwm2m_object_t *objectP) {
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
