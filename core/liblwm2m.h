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

#ifndef _LWM2M_CLIENT_H_
#define _LWM2M_CLIENT_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


/*
 *  Ressource URI
 */

#define LWM2M_URI_NOT_DEFINED   ((uint16_t)0xFFFF)

typedef struct
{
    uint16_t    objID;
    uint16_t    objInstance;
    uint16_t    resID;
    uint16_t    resInstance;
} lwm2m_uri_t;

// defined in uri.c
lwm2m_uri_t * lwm2m_decode_uri(const char * uriString, size_t uriLength);


/*
 *  Ressource values
 */

typedef enum
{
    LWM2M_OPAQUE_TYPE = 0,
    LWM2M_STRING_TYPE,
    LWM2M_INTEGER_TYPE,
    LWM2M_FLOAT_TYPE,
    LWM2M_BOOLEAN_TYPE,
    LWM2M_TIME_TYPE
} lwm2m_data_type_t;

typedef struct
{
    lwm2m_data_type_t   type;
    size_t              length;
    uint8_t             buffer[8];
} lwm2m_value_t;

// defined in value.c
lwm2m_value_t * lwm2m_opaqueToValue(uint8_t * buffer, size_t length);
lwm2m_value_t * lwm2m_stringToValue(char * stringP, size_t length);
lwm2m_value_t * lwm2m_int8ToValue(int8_t data);
lwm2m_value_t * lwm2m_int16ToValue(int16_t data);
lwm2m_value_t * lwm2m_int32ToValue(int32_t data);
lwm2m_value_t * lwm2m_int64ToValue(int64_t data);
lwm2m_value_t * lwm2m_float32ToValue(float data);
lwm2m_value_t * lwm2m_float64ToValue(double data);
lwm2m_value_t * lwm2m_boolToValue(bool data);
lwm2m_value_t * lwm2m_timeToValue(int64_t data);


/*
 * LWM2M Objects
 */
typedef uint8_t (*lwm2m_read_callback_t) (lwm2m_uri_t * uriP, lwm2m_value_t ** valueP, void * userData);
typedef uint8_t (*lwm2m_write_callback_t) (lwm2m_uri_t * uriP, lwm2m_value_t * valueP, void * userData);
typedef uint8_t (*lwm2m_execute_callback_t) (lwm2m_uri_t * uriP, void * userData);

typedef struct
{
    uint16_t                 objID;
    lwm2m_read_callback_t    readFunc;
    lwm2m_write_callback_t   writeFunc;
    lwm2m_execute_callback_t executeFunc;
    void *                   userData;
} lwm2m_object_t;

#endif
