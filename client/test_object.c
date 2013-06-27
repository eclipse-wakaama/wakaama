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
 *
 */

#include "core/liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "externals/er-coap-13/er-coap-13.h"

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

    if (LWM2M_URI_NOT_DEFINED == uriP->objInstance)
    {
        *bufferP = (uint8_t *)malloc(PRV_TLV_BUFFER_SIZE);
        if (NULL == *bufferP) return INTERNAL_SERVER_ERROR_5_00;

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
            return INTERNAL_SERVER_ERROR_5_00;
        }
        prv_output_buffer(*bufferP, *lengthP);
        return CONTENT_2_05;
    }
    else
    {
        targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, uriP->objInstance);
        if (NULL == targetP) return NOT_FOUND_4_04;

        switch (uriP->resID)
        {
        case 1:
            // we use int16 because value is unsigned
            *lengthP = lwm2m_int16ToPlainText(targetP->test, bufferP);
            if (*lengthP <= 0) return INTERNAL_SERVER_ERROR_5_00;
            return CONTENT_2_05;

        case LWM2M_URI_NOT_DEFINED:
            // TLV
            *bufferP = (uint8_t *)malloc(PRV_TLV_BUFFER_SIZE);
            if (NULL == *bufferP) return INTERNAL_SERVER_ERROR_5_00;

            *lengthP = lwm2m_intToTLV(TLV_RESSOURCE, targetP->test, 1, *bufferP, PRV_TLV_BUFFER_SIZE);
            if (0 == *lengthP)
            {
                free(*bufferP);
                *bufferP = NULL;
                return INTERNAL_SERVER_ERROR_5_00;
            }
            prv_output_buffer(*bufferP, *lengthP);
            return CONTENT_2_05;

        default:
            return NOT_FOUND_4_04;
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

    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, uriP->objInstance);
    if (NULL == targetP) return NOT_FOUND_4_04;

    switch (uriP->resID)
    {
    case 1:
        if (1 == lwm2m_PlainTextToInt64(buffer, length, &value))
        {
            if (value >= 0 && value <= 0xFF)
            {
                targetP->test = value;
                return CHANGED_2_04;
            }
        }
        return BAD_REQUEST_4_00;
    case LWM2M_URI_NOT_DEFINED:
        return NOT_IMPLEMENTED_5_01;
    default:
        return NOT_FOUND_4_04;
    }
}

static uint8_t prv_delete(uint16_t id,
                          lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    int64_t value;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&targetP);
    if (NULL == targetP) return NOT_FOUND_4_04;

    free(targetP);

    return DELETED_2_02;
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
        testObj->deleteFunc = prv_delete;
    }

    return testObj;
}
