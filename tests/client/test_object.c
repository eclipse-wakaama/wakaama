/*
Copyright (c) 2013, Intel Corporation

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
 * Implements an object for testing purpose
 *
 *                 Multiple
 * Object |  ID  | Instances | Mandatoty |
 *  Test  | 1024 |    Yes    |    No     |
 *
 *  Ressources:
 *              Supported    Multiple
 *  Name | ID | Operations | Instances | Mandatory |  Type   | Range | Units | Description |
 *  test |  1 |    R/W     |    No     |    Yes    | Integer | 0-255 |       |             |
 *  exec |  2 |     E      |    No     |    Yes    |         |       |       |             |
 *
 */

#include "core/liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define PRV_TLV_BUFFER_SIZE 64

typedef struct _prv_instance_
{
    struct _prv_instance_ * next;   // matches lwm2m_list_t::next
    uint16_t shortID;               // matches lwm2m_list_t::id
    uint8_t  test;
} prv_instance_t;

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

static uint8_t prv_read(lwm2m_uri_t * uriP,
                        char ** bufferP,
                        int * lengthP,
                        lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;

    *bufferP = NULL;
    *lengthP = 0;

    if (!LWM2M_URI_IS_SET_INSTANCE(uriP))
    {
        *bufferP = (uint8_t *)malloc(PRV_TLV_BUFFER_SIZE);
        if (NULL == *bufferP) return COAP_500_INTERNAL_SERVER_ERROR;

        // TLV
        for (targetP = (prv_instance_t *)(objectP->instanceList); targetP != NULL ; targetP = targetP->next)
        {
            char temp_buffer[16];
            int temp_length = 0;
            int result;

            result = lwm2m_intToTLV(TLV_RESSOURCE, targetP->test, 1, temp_buffer, 16);
            if (0 == result)
            {
                *lengthP = 0;
                break;
            }
            temp_length += result;

            result = lwm2m_opaqueToTLV(TLV_OBJECT_INSTANCE, temp_buffer, temp_length, targetP->shortID, *bufferP + *lengthP, PRV_TLV_BUFFER_SIZE - *lengthP);
            if (0 == result)
            {
                *lengthP = 0;
                break;
            }
            *lengthP += result;
        }
        if (*lengthP == 0)
        {
            free(*bufferP);
            *bufferP = NULL;
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        prv_output_buffer(*bufferP, *lengthP);
        return COAP_205_CONTENT;
    }
    else
    {
        targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, uriP->instanceId);
        if (NULL == targetP) return COAP_404_NOT_FOUND;

        if (!LWM2M_URI_IS_SET_RESOURCE(uriP))
        {
            // TLV
            *bufferP = (uint8_t *)malloc(PRV_TLV_BUFFER_SIZE);
            if (NULL == *bufferP) return COAP_500_INTERNAL_SERVER_ERROR;

            *lengthP = lwm2m_intToTLV(TLV_RESSOURCE, targetP->test, 1, *bufferP, PRV_TLV_BUFFER_SIZE);
            if (0 == *lengthP)
            {
                free(*bufferP);
                *bufferP = NULL;
                return COAP_500_INTERNAL_SERVER_ERROR;
            }
            prv_output_buffer(*bufferP, *lengthP);
            return COAP_205_CONTENT;
        }
        switch (uriP->resourceId)
        {
        case 1:
            // we use int16 because value is unsigned
            *lengthP = lwm2m_int16ToPlainText(targetP->test, bufferP);
            if (*lengthP <= 0) return COAP_500_INTERNAL_SERVER_ERROR;
            return COAP_205_CONTENT;

        default:
            return COAP_404_NOT_FOUND;
        }
    }
}

static uint8_t prv_write(lwm2m_uri_t * uriP,
                         char * buffer,
                         int length,
                         lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    int64_t value;

    // for write, instance ID is always set
    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, uriP->instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    if (!LWM2M_URI_IS_SET_INSTANCE(uriP)) return COAP_501_NOT_IMPLEMENTED;

    switch (uriP->resourceId)
    {
    case 1:
        if (1 == lwm2m_PlainTextToInt64(buffer, length, &value))
        {
            if (value >= 0 && value <= 0xFF)
            {
                targetP->test = value;
                return COAP_204_CHANGED;
            }
        }
        return COAP_400_BAD_REQUEST;
    default:
        return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_create(lwm2m_uri_t * uriP,
                          char * buffer,
                          int length,
                          lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    lwm2m_tlv_type_t type;
    uint16_t newId;
    uint16_t resID;
    size_t dataIndex;
    size_t dataLen;
    int result;
    int64_t value;

    if (LWM2M_URI_IS_SET_INSTANCE(uriP))
    {
        targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, uriP->instanceId);
        if (targetP != NULL) return COAP_406_NOT_ACCEPTABLE;
        newId = uriP->instanceId;
    }
    else
    {
        // determine a new unique ID
        newId = lwm2m_list_newId(objectP->instanceList);
    }

    result = lwm2m_decodeTLV(buffer, length, &type, &resID, &dataIndex, &dataLen);
    if (result != length)
    {
        // decode failure or too much data for our single ressource object
        return COAP_400_BAD_REQUEST;
    }
    if (type != TLV_RESSOURCE || resID != 1)
    {
        return COAP_400_BAD_REQUEST;
    }
    result = lwm2m_opaqueToInt(buffer + dataIndex, dataLen, &value);
    if (result == 0 || value < 0 || value > 255)
        return COAP_400_BAD_REQUEST;

    targetP = (prv_instance_t *)malloc(sizeof(prv_instance_t));
    if (NULL == targetP) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(prv_instance_t));
    targetP->shortID = newId;
    targetP->test = value;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);

    return COAP_201_CREATED;
}

static uint8_t prv_delete(uint16_t id,
                          lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    int64_t value;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&targetP);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    free(targetP);

    return COAP_202_DELETED;
}

static uint8_t prv_exec(lwm2m_uri_t * uriP,
                        char * buffer,
                        int length,
                        lwm2m_object_t * objectP)
{
    int64_t value;

    if (NULL == lwm2m_list_find(objectP->instanceList, uriP->instanceId)) return COAP_404_NOT_FOUND;

    switch (uriP->resourceId)
    {
    case 1:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 2:
        fprintf(stdout, "\r\n-----------------\r\n"
                        "Execute on %hu/%d/%d\r\n"
                        " Parameter (%d bytes):\r\n",
                        uriP->objectId, uriP->instanceId, uriP->resourceId, length);
        prv_output_buffer(buffer, length);
        fprintf(stdout, "-----------------\r\n\r\n");
        return COAP_204_CHANGED;
    default:
        return COAP_404_NOT_FOUND;
    }
}

lwm2m_object_t * get_test_object()
{
    lwm2m_object_t * testObj;

    testObj = (lwm2m_object_t *)malloc(sizeof(lwm2m_object_t));

    if (NULL != testObj)
    {
        int i;
        prv_instance_t * targetP;

        memset(testObj, 0, sizeof(lwm2m_object_t));

        testObj->objID = 1024;
        for (i=0 ; i < 3 ; i++)
        {
            targetP = (prv_instance_t *)malloc(sizeof(prv_instance_t));
            if (NULL == targetP) return NULL;
            memset(targetP, 0, sizeof(prv_instance_t));
            targetP->shortID = 10 + i;
            targetP->test = 20 + i;
            testObj->instanceList = LWM2M_LIST_ADD(testObj->instanceList, targetP);
        }
        testObj->readFunc = prv_read;
        testObj->writeFunc = prv_write;
        testObj->createFunc = prv_create;
        testObj->deleteFunc = prv_delete;
        testObj->executeFunc = prv_exec;
    }

    return testObj;
}
