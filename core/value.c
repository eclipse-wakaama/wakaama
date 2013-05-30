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

#include "internals.h"
#include <stdlib.h>
#include <string.h>


lwm2m_value_t * lwm2m_opaqueToValue(uint8_t * buffer, size_t length)
{
    lwm2m_value_t * valueP;

    if (length > 8)
    {
        valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t) + length - 8);
    }
    else
    {
        valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    }

    if (NULL != valueP)
    {
        valueP->type = LWM2M_OPAQUE_TYPE;
        valueP->length = length;
        memcpy(valueP->buffer, buffer, length);
    }

    return valueP;
}


lwm2m_value_t * lwm2m_stringToValue(char * stringP, size_t length)
{
    lwm2m_value_t * valueP;

    valueP = lwm2m_opaqueToValue((uint8_t *)stringP, length);

    if (NULL != valueP)
    {
        valueP->type = LWM2M_STRING_TYPE;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_int8ToValue(int8_t data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_INTEGER_TYPE;
        valueP->length = 1;
        *((int8_t *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_int16ToValue(int16_t data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_INTEGER_TYPE;
        valueP->length = 2;
        *((int16_t *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_int32ToValue(int32_t data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_INTEGER_TYPE;
        valueP->length = 4;
        *((int32_t *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_int64ToValue(int64_t data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_INTEGER_TYPE;
        valueP->length = 8;
        *((int64_t *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_float32ToValue(float data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_FLOAT_TYPE;
        valueP->length = 4;
        *((float *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_float64ToValue(double data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_FLOAT_TYPE;
        valueP->length = 8;
        *((double *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_boolToValue(bool data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_BOOLEAN_TYPE;
        valueP->length = sizeof(bool);
        *((bool *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_timeToValue(int64_t data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_TIME_TYPE;
        valueP->length = 8;
        *((int64_t *)valueP->buffer) = data;
    }

    return valueP;
}
