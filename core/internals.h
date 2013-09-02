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

#ifndef _LWM2M_INTERNALS_H_
#define _LWM2M_INTERNALS_H_

#include "liblwm2m.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "externals/er-coap-13/er-coap-13.h"


#define URI_REGISTRATION_SEGMENT        "rd"
#define URI_REGISTRATION_SEGMENT_LEN    2
#define URI_BOOTSTRAP_SEGMENT           "bs"
#define URI_BOOTSTRAP_SEGMENT_LEN       2

typedef enum
{
    URI_UNKNOWN = 0,
    URI_REGISTRATION,
    URI_BOOTSTRAP,
    URI_DM,
} lwm2m_uri_type_t;

// defined in uri.c
lwm2m_uri_type_t lwm2m_decode_uri(multi_option_t *uriPath, lwm2m_uri_t * uriP);

// defined in objects.c
coap_status_t object_read(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, char ** bufferP, int * lengthP);
coap_status_t object_write(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, char * buffer, int length);
coap_status_t object_create(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, char * buffer, int length);
coap_status_t object_delete(lwm2m_context_t * contextP, lwm2m_uri_t * uriP);
int prv_getRegisterPayload(lwm2m_context_t * contextP, char * buffer, size_t length);

// defined in transaction.c
lwm2m_transaction_t * transaction_new(uint16_t mID, lwm2m_endpoint_type_t peerType, void * peerP);
int transaction_send(lwm2m_context_t * contextP, lwm2m_transaction_t * transacP);

void handle_server_reply(lwm2m_context_t * contextP, lwm2m_transaction_t * transacP, coap_packet_t * message);

#endif
