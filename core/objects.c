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

#include <string.h>

coap_status_t object_get_response(lwm2m_context_t * contextP,
                                  coap_method_t method,
                                  lwm2m_uri_t * uriP,
                                  coap_packet_t * response)
{
    int i;
    coap_status_t result;
    lwm2m_value_t * valueP = NULL;

    i = 0;
    while (i < contextP->numObject && contextP->objectList[i]->objID < uriP->objID)
    {
        i++;
    }
    if (i == contextP->numObject)
    {
        result = NOT_FOUND_4_04;
    }
    else
    {
        result = METHOD_NOT_ALLOWED_4_05;
        switch (method)
        {
        case COAP_GET:
            if (NULL != contextP->objectList[i]->readFunc)
            {
                result = contextP->objectList[i]->readFunc(uriP, &valueP, contextP->objectList[i]->userData);
            }
            break;
        case COAP_POST:
            break;
        case COAP_PUT:
            break;
        case COAP_DELETE:
            break;
        default:
            result = BAD_REQUEST_4_00;
            break;
        }
    }

    coap_set_status_code(response, result);
    if (NULL != valueP)
    {
        coap_set_payload(response, valueP->buffer, valueP->length);
    }

    return result;
}
