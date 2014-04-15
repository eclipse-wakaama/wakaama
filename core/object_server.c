/*
Copyright (c) 2014, Intel Corporation

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

coap_status_t object_server_read(lwm2m_context_t * contextP,
                                 lwm2m_uri_t * uriP,
                                 char ** bufferP,
                                 int * lengthP)
{
    return COAP_501_NOT_IMPLEMENTED;
}

coap_status_t object_server_write(lwm2m_context_t * contextP,
                                  lwm2m_uri_t * uriP,
                                  char * buffer,
                                  int length)
{
    return COAP_501_NOT_IMPLEMENTED;
}

coap_status_t object_server_execute(lwm2m_context_t * contextP,
                                    lwm2m_uri_t * uriP,
                                    char * buffer,
                                    int length)
{
    return COAP_501_NOT_IMPLEMENTED;
}

coap_status_t object_server_create(lwm2m_context_t * contextP,
                                   lwm2m_uri_t * uriP,
                                   char * buffer,
                                   int length)
{
    return COAP_501_NOT_IMPLEMENTED;
}

coap_status_t object_server_delete(lwm2m_context_t * contextP,
                                   lwm2m_uri_t * uriP)
{
    return COAP_501_NOT_IMPLEMENTED;
}

coap_status_t object_security_create(lwm2m_context_t * contextP,
                                     lwm2m_uri_t * uriP,
                                     char * buffer,
                                     int length)
{
    return COAP_501_NOT_IMPLEMENTED;
}

coap_status_t object_security_delete(lwm2m_context_t * contextP,
                                     lwm2m_uri_t * uriP)
{
    return COAP_501_NOT_IMPLEMENTED;
}
