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
 *    Julien Vermillard - initial implementation
 *    Fabien Fleutot - Please refer to git log
 *    David Navarro, Intel Corporation - Please refer to git log
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
 * 4 update supported objects  read/write
 * 5 update result             read
 */

#include "liblwm2m.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define PRV_TLV_BUFFER_SIZE 128

typedef struct
{
    uint8_t state;
    uint8_t supported;
    uint8_t result;
} firmware_data_t;


static uint8_t prv_firmware_read(uint16_t instanceId,
                                 int * numDataP,
                                 lwm2m_tlv_t ** dataArrayP,
                                 lwm2m_object_t * objectP)
{
    int i;
    uint8_t result;
    firmware_data_t * data = (firmware_data_t*)(objectP->userData);

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        *dataArrayP = lwm2m_tlv_new(3);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 3;
        (*dataArrayP)[0].id = 3;
        (*dataArrayP)[1].id = 4;
        (*dataArrayP)[2].id = 5;
    }

    i = 0;
    do
    {
        switch ((*dataArrayP)[i].id)
        {
        case 0:
        case 1:
        case 2:
            result = COAP_405_METHOD_NOT_ALLOWED;
            break;

        case 3:
            // firmware update state (int)
            lwm2m_tlv_encode_int(data->state, *dataArrayP + i);
            (*dataArrayP)[i].type = LWM2M_TYPE_RESSOURCE;

            if (0 != (*dataArrayP)[i].length) result = COAP_205_CONTENT;
            else result = COAP_500_INTERNAL_SERVER_ERROR;

            break;

        case 4:
            lwm2m_tlv_encode_int(data->supported, *dataArrayP + i);
            (*dataArrayP)[i].type = LWM2M_TYPE_RESSOURCE;

            if (0 != (*dataArrayP)[i].length) result = COAP_205_CONTENT;
            else result = COAP_500_INTERNAL_SERVER_ERROR;

            break;

        case 5:
            lwm2m_tlv_encode_int(data->result, *dataArrayP + i);
            (*dataArrayP)[i].type = LWM2M_TYPE_RESSOURCE;

            if (0 != (*dataArrayP)[i].length) result = COAP_205_CONTENT;
            else result = COAP_500_INTERNAL_SERVER_ERROR;

            break;

        default:
            result = COAP_404_NOT_FOUND;
        }

        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

static uint8_t prv_firmware_write(uint16_t instanceId,
                                  int numData,
                                  lwm2m_tlv_t * dataArray,
                                  lwm2m_object_t * objectP)
{
    int i;
    bool bvalue;
    uint8_t result;
    firmware_data_t * data = (firmware_data_t*)(objectP->userData);

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
        case 0:
            // inline firmware binary
            result = COAP_204_CHANGED;
            break;

        case 1:
            // URL for download the firmware
            result = COAP_204_CHANGED;
            break;

        case 4:
        	if (lwm2m_tlv_decode_bool(&dataArray[i], &bvalue) == 1) {
                data->supported = bvalue ? 1 : 0;
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

static uint8_t prv_firmware_execute(uint16_t instanceId,
                                    uint16_t resourceId,
                                    char * buffer,
                                    int length,
                                    lwm2m_object_t * objectP)
{
    firmware_data_t * data = (firmware_data_t*)(objectP->userData);

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    if (length != 0) return COAP_400_BAD_REQUEST;

    // for execute callback, resId is always set.
    switch (resourceId)
    {
    case 2:
        if (data->state == 1)
        {
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

lwm2m_object_t * get_object_firmware()
{
    /*
     * The get_object_firmware function create the object itself and return a pointer to the structure that represent it.
     */
    lwm2m_object_t * firmwareObj;

    firmwareObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != firmwareObj)
    {
        memset(firmwareObj, 0, sizeof(lwm2m_object_t));

        /*
         * It assign his unique ID
         * The 5 is the standard ID for the optional object "Object firmware".
         */
        firmwareObj->objID = 5;

        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library don't need to
         * know the resources of the object, only the server does.
         */
        firmwareObj->readFunc = prv_firmware_read;
        firmwareObj->writeFunc = prv_firmware_write;
        firmwareObj->executeFunc = prv_firmware_execute;
        firmwareObj->userData = lwm2m_malloc(sizeof(firmware_data_t));

        /*
         * Also some user data can be stored in the object with a private structure containing the needed variables
         */
        if (NULL != firmwareObj->userData)
        {
            ((firmware_data_t*)firmwareObj->userData)->state = 1;
            ((firmware_data_t*)firmwareObj->userData)->supported = 0;
            ((firmware_data_t*)firmwareObj->userData)->result = 0;
        }
        else
        {
            free(firmwareObj);
            firmwareObj = NULL;
        }
    }

    return firmwareObj;
}
