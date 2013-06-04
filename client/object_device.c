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

#include "core/liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "externals/er-coap-13/er-coap-13.h"

typedef struct
{
    uint64_t time;
    uint8_t  time_zone;
} device_data_t;

static uint8_t prv_device_read(lwm2m_uri_t * uriP,
                               char ** bufferP,
                               int * lengthP,
                               void * userData)
{
    *bufferP = NULL;
    *lengthP = 0;

    // this is a single instance object
    // TODO: uriP->objInstance == LWM2M_URI_NOT_DEFINED should be handled
    if (0 != uriP->objInstance)
    {
        return NOT_FOUND_4_04;
    }

    switch (uriP->resID)
    {
    case 0:
        *bufferP = strdup("Open Mobile Alliance");
        if (NULL != *bufferP)
        {
            *lengthP = strlen(*bufferP);
            return CONTENT_2_05;
        }
        else
        {
            return MEMORY_ALLOCATION_ERROR;
        }
    case 1:
        *bufferP = strdup("Lightweight M2M Client");
        if (NULL != *bufferP)
        {
            *lengthP = strlen(*bufferP);
            return CONTENT_2_05;
        }
        else
        {
            return MEMORY_ALLOCATION_ERROR;
        }
    case 2:
        *bufferP = strdup("345000123");
        if (NULL != *bufferP)
        {
            *lengthP = strlen(*bufferP);
            return CONTENT_2_05;
        }
        else
        {
            return MEMORY_ALLOCATION_ERROR;
        }
    case 3:
        *bufferP = strdup("1.0");
        if (NULL != *bufferP)
        {
            *lengthP = strlen(*bufferP);
            return CONTENT_2_05;
        }
        else
        {
            return MEMORY_ALLOCATION_ERROR;
        }
    case 4:
        return METHOD_NOT_ALLOWED_4_05;
    case 5:
        return METHOD_NOT_ALLOWED_4_05;
    case 6:
        *lengthP = lwm2m_int8ToPlainText(0, bufferP);
        if (0 != *lengthP)
        {
            return CONTENT_2_05;
        }
        else
        {
            return MEMORY_ALLOCATION_ERROR;
        }
    case 7:
        *lengthP = lwm2m_int8ToPlainText(100, bufferP);
        if (0 != *lengthP)
        {
            return CONTENT_2_05;
        }
        else
        {
            return MEMORY_ALLOCATION_ERROR;
        }
    case 8:
        *lengthP = lwm2m_int8ToPlainText(15, bufferP);
        if (0 != *lengthP)
        {
            return CONTENT_2_05;
        }
        else
        {
            return MEMORY_ALLOCATION_ERROR;
        }
    case 9:
        // TODO: this is a multi-instance ressource
        *lengthP = lwm2m_int8ToPlainText(0, bufferP);
        if (0 != *lengthP)
        {
            return CONTENT_2_05;
        }
        else
        {
            return MEMORY_ALLOCATION_ERROR;
        }
    case 10:
        return METHOD_NOT_ALLOWED_4_05;
    case 11:
        *lengthP = lwm2m_int64ToPlainText(((device_data_t*)userData)->time, bufferP);
        if (0 != *lengthP)
        {
            return CONTENT_2_05;
        }
        else
        {
            return MEMORY_ALLOCATION_ERROR;
        }
    case 12:
        *lengthP = lwm2m_int8ToPlainText(((device_data_t*)userData)->time_zone, bufferP);
        if (0 != *lengthP)
        {
            return CONTENT_2_05;
        }
        else
        {
            return MEMORY_ALLOCATION_ERROR;
        }
    case LWM2M_URI_NOT_DEFINED:
        // TODO: return whole object
        return NOT_IMPLEMENTED_5_01;
    default:
        return NOT_FOUND_4_04;
    }

}

static uint8_t prv_device_write(lwm2m_uri_t * uriP,
                                char * buffer,
                                int length,
                                void * userData)
{
    // this is a single instance object
    if (0 != uriP->objInstance)
    {
        return NOT_FOUND_4_04;
    }

    switch (uriP->resID)
    {
    case 11:
        if (1 == lwm2m_PlainTextToUInt64(buffer, length, &((device_data_t*)userData)->time))
        {
            return CHANGED_2_04;
        }
        else
        {
            return BAD_REQUEST_4_00;
        }
    case 12:
        return NOT_IMPLEMENTED_5_01;
    default:
        return METHOD_NOT_ALLOWED_4_05;
    }
}

static uint8_t prv_device_execute(lwm2m_uri_t * uriP,
                                  void * userData)
{
    // this is a single instance object
    if (0 != uriP->objInstance)
    {
        return NOT_FOUND_4_04;
    }

    switch (uriP->resID)
    {
    case 4:
        fprintf(stdout, "\n\t REBOOT\r\n\n");
        return CHANGED_2_04;
    case 5:
        fprintf(stdout, "\n\t FACTORY RESET\r\n\n");
        return CHANGED_2_04;
    case 10:
        fprintf(stdout, "\n\t RESET ERROR CODE\r\n\n");
        return CHANGED_2_04;
    default:
        return METHOD_NOT_ALLOWED_4_05;
    }
}

lwm2m_object_t * get_object_device()
{
    lwm2m_object_t * deviceObj;

    deviceObj = (lwm2m_object_t *)malloc(sizeof(lwm2m_object_t));

    if (NULL != deviceObj)
    {
        memset(deviceObj, 0, sizeof(lwm2m_object_t));

        deviceObj->objID = 3;
        deviceObj->readFunc = prv_device_read;
        deviceObj->writeFunc = prv_device_write;
        deviceObj->executeFunc = prv_device_execute;
        deviceObj->userData = malloc(sizeof(device_data_t));
        if (NULL != deviceObj->userData)
        {
            ((device_data_t*)deviceObj->userData)->time = 1367491215;
            ((device_data_t*)deviceObj->userData)->time_zone = 2;
        }
        else
        {
            free(deviceObj);
            deviceObj = NULL;
        }
    }

    return deviceObj;
}
