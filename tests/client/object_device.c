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

static int prv_get_object_tlv(char ** bufferP,
                              device_data_t* dataP)
{
    int length = 0;
    int result;
    char temp_buffer[16];
    int temp_length;

    *bufferP = (uint8_t *)malloc(PRV_TLV_BUFFER_SIZE);

    if (NULL == *bufferP) return 0;

    result = lwm2m_opaqueToTLV(TLV_RESSOURCE,
                               PRV_MANUFACTURER, strlen(PRV_MANUFACTURER),
                               0,
                               *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_opaqueToTLV(TLV_RESSOURCE,
                               PRV_MODEL_NUMBER, strlen(PRV_MODEL_NUMBER),
                               1,
                               *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_opaqueToTLV(TLV_RESSOURCE,
                               PRV_SERIAL_NUMBER, strlen(PRV_SERIAL_NUMBER),
                               2,
                               *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_opaqueToTLV(TLV_RESSOURCE,
                               PRV_FIRMWARE_VERSION, strlen(PRV_FIRMWARE_VERSION),
                               3,
                               *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;


    result = lwm2m_intToTLV(TLV_RESSOURCE_INSTANCE, PRV_POWER_SOURCE_1, 0, temp_buffer, 16);
    if (0 == result) goto error;
    temp_length = result;
    result = lwm2m_intToTLV(TLV_RESSOURCE_INSTANCE, PRV_POWER_SOURCE_2, 1, temp_buffer+result, 16-result);
    if (0 == result) goto error;
    temp_length += result;
    result = lwm2m_opaqueToTLV(TLV_MULTIPLE_INSTANCE,
                               temp_buffer, temp_length,
                               6,
                               *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_intToTLV(TLV_RESSOURCE_INSTANCE, PRV_POWER_VOLTAGE_1, 0, temp_buffer, 16);
    if (0 == result) goto error;
    temp_length = result;
    result = lwm2m_intToTLV(TLV_RESSOURCE_INSTANCE, PRV_POWER_VOLTAGE_2, 1, temp_buffer+result, 16-result);
    if (0 == result) goto error;
    temp_length += result;
    result = lwm2m_opaqueToTLV(TLV_MULTIPLE_INSTANCE,
                               temp_buffer, temp_length,
                               7,
                               *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_intToTLV(TLV_RESSOURCE_INSTANCE, PRV_POWER_CURRENT_1, 0, temp_buffer, 16);
    if (0 == result) goto error;
    temp_length = result;
    result = lwm2m_intToTLV(TLV_RESSOURCE_INSTANCE, PRV_POWER_CURRENT_2, 1, temp_buffer+result, 16-result);
    if (0 == result) goto error;
    temp_length += result;
    result = lwm2m_opaqueToTLV(TLV_MULTIPLE_INSTANCE,
                               temp_buffer, temp_length,
                               8,
                               *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_intToTLV(TLV_RESSOURCE,
                            PRV_BATTERY_LEVEL,
                            9,
                            *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_intToTLV(TLV_RESSOURCE,
                            PRV_MEMORY_FREE,
                            10,
                            *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_intToTLV(TLV_RESSOURCE_INSTANCE, PRV_ERROR_CODE, 0, temp_buffer, 16);
    if (0 == result) goto error;
    temp_length = result;
    result = lwm2m_opaqueToTLV(TLV_MULTIPLE_INSTANCE,
                               temp_buffer, temp_length,
                               11,
                               *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_intToTLV(TLV_RESSOURCE,
                            dataP->time,
                            13,
                            *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_opaqueToTLV(TLV_RESSOURCE,
                               dataP->time_offset, strlen(dataP->time_offset),
                               14,
                               *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_opaqueToTLV(TLV_RESSOURCE,
                               PRV_BINDING_MODE, strlen(PRV_BINDING_MODE),
                               15,
                               *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
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

static uint8_t prv_device_read(lwm2m_uri_t * uriP,
                               char ** bufferP,
                               int * lengthP,
                               lwm2m_object_t * objectP)
{
    *bufferP = NULL;
    *lengthP = 0;

    // this is a single instance object
    if (LWM2M_URI_IS_SET_INSTANCE(uriP) && uriP->instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (!LWM2M_URI_IS_SET_RESOURCE(uriP))
    {
        *lengthP = prv_get_object_tlv(bufferP, (device_data_t*)(objectP->userData));
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
        *bufferP = strdup(PRV_MANUFACTURER);
        if (NULL != *bufferP)
        {
            *lengthP = strlen(*bufferP);
            return COAP_205_CONTENT;
        }
        else
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    case 1:
        *bufferP = strdup(PRV_MODEL_NUMBER);
        if (NULL != *bufferP)
        {
            *lengthP = strlen(*bufferP);
            return COAP_205_CONTENT;
        }
        else
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    case 2:
        *bufferP = strdup(PRV_SERIAL_NUMBER);
        if (NULL != *bufferP)
        {
            *lengthP = strlen(*bufferP);
            return COAP_205_CONTENT;
        }
        else
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    case 3:
        *bufferP = strdup(PRV_FIRMWARE_VERSION);
        if (NULL != *bufferP)
        {
            *lengthP = strlen(*bufferP);
            return COAP_205_CONTENT;
        }
        else
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    case 4:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 5:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 6:
    {
        char buffer1[16];
        char buffer2[16];
        int result;
        int instance_length;

        result = lwm2m_intToTLV(TLV_RESSOURCE_INSTANCE, PRV_POWER_SOURCE_1, 0, buffer1, 16);
        if (0 == result) return COAP_500_INTERNAL_SERVER_ERROR;
        instance_length = result;
        result = lwm2m_intToTLV(TLV_RESSOURCE_INSTANCE, PRV_POWER_SOURCE_2, 1, buffer1+result, 16-result);
        if (0 == result) return COAP_500_INTERNAL_SERVER_ERROR;
        instance_length += result;

        *lengthP = lwm2m_opaqueToTLV(TLV_MULTIPLE_INSTANCE, buffer1, instance_length, 6, buffer2, 16);
        if (0 == *lengthP) return COAP_500_INTERNAL_SERVER_ERROR;

        *bufferP = (char *)malloc(*lengthP);
        if (NULL == *bufferP) return COAP_500_INTERNAL_SERVER_ERROR;

        memmove(*bufferP, buffer2, *lengthP);

        fprintf(stderr, "TLV (%d bytes):\r\n", *lengthP);
        prv_output_buffer(*bufferP, *lengthP);

        return COAP_205_CONTENT;
    }
    case 7:
    {
        char buffer1[16];
        char buffer2[16];
        int result;
        int instance_length;

        result = lwm2m_intToTLV(TLV_RESSOURCE_INSTANCE, PRV_POWER_VOLTAGE_1, 0, buffer1, 16);
        if (0 == result) return COAP_500_INTERNAL_SERVER_ERROR;
        instance_length = result;
        result = lwm2m_intToTLV(TLV_RESSOURCE_INSTANCE, PRV_POWER_VOLTAGE_2, 1, buffer1+result, 16-result);
        if (0 == result) return COAP_500_INTERNAL_SERVER_ERROR;
        instance_length += result;

        *lengthP = lwm2m_opaqueToTLV(TLV_MULTIPLE_INSTANCE, buffer1, instance_length, 7, buffer2, 16);
        if (0 == *lengthP) return COAP_500_INTERNAL_SERVER_ERROR;

        *bufferP = (char *)malloc(*lengthP);
        if (NULL == *bufferP) return COAP_500_INTERNAL_SERVER_ERROR;

        memmove(*bufferP, buffer2, *lengthP);

        fprintf(stderr, "TLV (%d bytes):\r\n", *lengthP);
        prv_output_buffer(*bufferP, *lengthP);

        return COAP_205_CONTENT;
    }
    case 8:
    {
        char buffer1[16];
        char buffer2[16];
        int result;
        int instance_length;

        result = lwm2m_intToTLV(TLV_RESSOURCE_INSTANCE, PRV_POWER_CURRENT_1, 0, buffer1, 16);
        if (0 == result) return COAP_500_INTERNAL_SERVER_ERROR;
        instance_length = result;
        result = lwm2m_intToTLV(TLV_RESSOURCE_INSTANCE, PRV_POWER_CURRENT_2, 1, buffer1+result, 16-result);
        if (0 == result) return COAP_500_INTERNAL_SERVER_ERROR;
        instance_length += result;

        *lengthP = lwm2m_opaqueToTLV(TLV_MULTIPLE_INSTANCE, buffer1, instance_length, 8, buffer2, 16);
        if (0 == *lengthP) return COAP_500_INTERNAL_SERVER_ERROR;

        *bufferP = (char *)malloc(*lengthP);
        if (NULL == *bufferP) return COAP_500_INTERNAL_SERVER_ERROR;

        memmove(*bufferP, buffer2, *lengthP);

        fprintf(stderr, "TLV (%d bytes):\r\n", *lengthP);
        prv_output_buffer(*bufferP, *lengthP);

        return COAP_205_CONTENT;
    }
    case 9:
        *lengthP = lwm2m_int8ToPlainText(PRV_BATTERY_LEVEL, bufferP);
        if (0 != *lengthP)
        {
            return COAP_205_CONTENT;
        }
        else
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    case 10:
        *lengthP = lwm2m_int8ToPlainText(PRV_MEMORY_FREE, bufferP);
        if (0 != *lengthP)
        {
            return COAP_205_CONTENT;
        }
        else
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    case 11:
        *lengthP = lwm2m_int8ToPlainText(PRV_ERROR_CODE, bufferP);
        if (0 != *lengthP)
        {
            return COAP_205_CONTENT;
        }
        else
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    case 12:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 13:
        *lengthP = lwm2m_int64ToPlainText(((device_data_t*)(objectP->userData))->time, bufferP);
        if (0 != *lengthP)
        {
            return COAP_205_CONTENT;
        }
        else
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    case 14:
        *bufferP = strdup(((device_data_t*)(objectP->userData))->time_offset);
        if (NULL != *bufferP)
        {
            *lengthP = strlen(*bufferP);
            return COAP_205_CONTENT;
        }
        else
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    case 15:
        *bufferP = strdup(PRV_BINDING_MODE);
        if (NULL != *bufferP)
        {
            *lengthP = strlen(*bufferP);
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

static uint8_t prv_device_write(lwm2m_uri_t * uriP,
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

    switch (uriP->resourceId)
    {
    case 13:
        if (1 == lwm2m_PlainTextToInt64(buffer, length, &((device_data_t*)(objectP->userData))->time))
        {
            return COAP_204_CHANGED;
        }
        else
        {
            return COAP_400_BAD_REQUEST;
        }
    case 14:
        if (1 == prv_check_time_offset(buffer, length))
        {
            strncpy(((device_data_t*)(objectP->userData))->time_offset, buffer, length);
            ((device_data_t*)(objectP->userData))->time_offset[length] = 0;
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

static uint8_t prv_device_execute(lwm2m_uri_t * uriP,
								  char * rBuffer,
								  int rLength,
								  char ** wBuffer,
								  int *wLength,
                                  lwm2m_object_t * objectP)
{
    // this is a single instance object
    if (LWM2M_URI_IS_SET_INSTANCE(uriP) && uriP->instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    if (rLength != 0) return COAP_400_BAD_REQUEST;

    // for execute callback, resId is always set.
    switch (uriP->resourceId)
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
