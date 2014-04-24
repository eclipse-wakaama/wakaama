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


static void prv_output_buffer(uint8_t * buffer,
                              int length)
{
    int i;
    uint8_t array[16];

    i = 0;
    while (i < length)
    {
        int j;
        fprintf(stderr, "  ");

        memcpy(array, buffer+i, 16);

        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            fprintf(stderr, "%02X ", array[j]);
        }
        while (j < 16)
        {
            fprintf(stderr, "   ");
            j++;
        }
        fprintf(stderr, "  ");
        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            if (isprint(array[j]))
                fprintf(stderr, "%c ", array[j]);
            else
                fprintf(stderr, ". ");
        }
        fprintf(stderr, "\n");

        i += 16;
    }
}

static int prv_get_object_tlv(char ** bufferP,
                              firmware_data_t* dataP)
{
    int length = 0;
    int result;

    *bufferP = (uint8_t *)malloc(PRV_TLV_BUFFER_SIZE);

    if (NULL == *bufferP) return 0;
    result = lwm2m_intToTLV(TLV_RESSOURCE, dataP->state, 3, *bufferP, PRV_TLV_BUFFER_SIZE);

    if (0 == result) goto error;
    length += result;

    result = lwm2m_boolToTLV(TLV_RESSOURCE, dataP->supported, 4, *bufferP + length, PRV_TLV_BUFFER_SIZE - length);

    if (0 == result) goto error;
    length += result;

    result = lwm2m_intToTLV(TLV_RESSOURCE, dataP->result, 5, *bufferP + length, PRV_TLV_BUFFER_SIZE - length);

    if (0 == result) goto error;
    length += result;


    fprintf(stderr, "TLV (%d bytes):\r\n", length);
    prv_output_buffer(*bufferP, length);

    return length;

error:
    fprintf(stderr, "TLV generation failed:\r\n");
    free(*bufferP);
    *bufferP = NULL;
    return 0;
}

static uint8_t prv_firmware_read(lwm2m_uri_t * uriP,
                               char ** bufferP,
                               int * lengthP,
                               lwm2m_object_t * objectP)
{
    *bufferP = NULL;
    *lengthP = 0;

    firmware_data_t * data = (firmware_data_t*)(objectP->userData);

    // this is a single instance object
    if (LWM2M_URI_IS_SET_INSTANCE(uriP) && uriP->instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (!LWM2M_URI_IS_SET_RESOURCE(uriP))
    {
        *lengthP = prv_get_object_tlv(bufferP, (firmware_data_t*)(objectP->userData));
        if (0 != *lengthP)
        {
            return COAP_205_CONTENT;
        }
        else
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }
    // else a simple switch structure is used to respond at the specified resource asked
    switch (uriP->resourceId)
    {
    case 0:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 1:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 2:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 3:
        // firmware update state (int)
        *lengthP = lwm2m_int8ToPlainText(data->state, bufferP);
        if (0 != *lengthP)
        {
            return COAP_205_CONTENT;
        }
        else
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    case 4:
        *lengthP = lwm2m_int8ToPlainText(data->supported, bufferP);
        if (0 != *lengthP)
        {
            return COAP_205_CONTENT;
        }
        else
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    case 5:
        *lengthP = lwm2m_int8ToPlainText(data->result, bufferP);
        if (0 != *lengthP)
        {
            return COAP_205_CONTENT;
        }
        else
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    default:
        return COAP_404_NOT_FOUND;
    }

}

static uint8_t prv_firmware_write(lwm2m_uri_t * uriP,
                                char * buffer,
                                int length,
                                lwm2m_object_t * objectP)
{
    // this is a single instance object
    if (LWM2M_URI_IS_SET_INSTANCE(uriP) && uriP->instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    if (!LWM2M_URI_IS_SET_RESOURCE(uriP)) return COAP_501_NOT_IMPLEMENTED;

    firmware_data_t * data = (firmware_data_t*)(objectP->userData);

    switch (uriP->resourceId)
    {
    case 0:
        // inline firmware binary
        return COAP_204_CHANGED;
    case 1:
        // URL for download the firmware
        return COAP_204_CHANGED;
    case 4:
        if (1 ==  length && buffer[0] == '0')
        {
            data->supported = 0;
            return COAP_204_CHANGED;
        }
        else if (1 ==  length && buffer[0] == '1')
        {
            data->supported = 1;
            return COAP_204_CHANGED;

        }
        else
        {
            return COAP_400_BAD_REQUEST;
        }
    default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

static uint8_t prv_firmware_execute(lwm2m_uri_t * uriP,
                                  char * buffer,
                                  int length,
                                  lwm2m_object_t * objectP)
{
    // this is a single instance object
    if (LWM2M_URI_IS_SET_INSTANCE(uriP) && uriP->instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    if (length != 0) return COAP_400_BAD_REQUEST;

    firmware_data_t * data = (firmware_data_t*)(objectP->userData);

    // for execute callback, resId is always set.
    switch (uriP->resourceId)
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

    firmwareObj = (lwm2m_object_t *)malloc(sizeof(lwm2m_object_t));

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
        firmwareObj->userData = malloc(sizeof(firmware_data_t));

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
