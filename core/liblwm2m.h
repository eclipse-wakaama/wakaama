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
#include <netinet/in.h>

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


/*
 *  Ressource values
 */

// defined in utils.c
/*
 * These utility functions allocate a new buffer storing the plain text
 * representation of data. They return the size in bytes of the buffer
 * or 0 in case of error.
 * There is no trailing '\0' character in the buffer.
 */
int lwm2m_int8ToPlainText(int8_t data, char ** bufferP);
int lwm2m_int16ToPlainText(int16_t data, char ** bufferP);
int lwm2m_int32ToPlainText(int32_t data, char ** bufferP);
int lwm2m_int64ToPlainText(int64_t data, char ** bufferP);
int lwm2m_float32ToPlainText(float data, char ** bufferP);
int lwm2m_float64ToPlainText(double data, char ** bufferP);
int lwm2m_boolToPlainText(bool data, char ** bufferP);


/*
 * LWM2M Objects
 */

typedef uint8_t (*lwm2m_read_callback_t) (lwm2m_uri_t * uriP, char ** bufferP, int * lengthP, void * userData);
typedef uint8_t (*lwm2m_write_callback_t) (lwm2m_uri_t * uriP, char * buffer, int length, void * userData);
typedef uint8_t (*lwm2m_execute_callback_t) (lwm2m_uri_t * uriP, void * userData);
typedef void (*lwm2m_close_callback_t) (void * userData);

typedef struct
{
    uint16_t                 objID;
    lwm2m_read_callback_t    readFunc;
    lwm2m_write_callback_t   writeFunc;
    lwm2m_execute_callback_t executeFunc;
    lwm2m_close_callback_t   closeFunc;
    void *                   userData;
} lwm2m_object_t;


/*
 * LWM2M Context
 */

typedef struct
{
    uint16_t          numObject;
    lwm2m_object_t ** objectList;
} lwm2m_context_t;

lwm2m_context_t * lwm2m_init();
void lwm2m_close(lwm2m_context_t * contextP);
int lwm2m_add_object(lwm2m_context_t * contextP, lwm2m_object_t * objectP);
int lwm2m_handle_packet(lwm2m_context_t * contextP, uint8_t * buffer, int length, int socket, struct sockaddr_storage fromAddr, socklen_t fromAddrLen);

#endif
