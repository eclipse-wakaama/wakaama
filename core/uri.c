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


int prv_get_number(const char * uriString,
                   size_t uriLength)
{
    int result = 0;
    int mul = 0;
    int i = 0;

    while (i < uriLength)
    {
        if ('0' <= uriString[i] && uriString[i] <= '9')
        {
            if (0 == mul)
            {
                mul = 10;
            }
            else
            {
                result *= mul;
            }
            result += uriString[i] - '0';
        }
        else
        {
            return -1;
        }
        i++;
    }

    return result;
}


lwm2m_uri_type_t lwm2m_decode_uri(multi_option_t *uriPath,
                                  lwm2m_uri_t * uriP)
{
    int readNum;

    if (NULL == uriPath) return URI_UNKNOWN;

    memset(uriP, 0xFF, sizeof(lwm2m_uri_t));

    // Read object ID
    if (URI_REGISTRATION_SEGMENT_LEN == uriPath->len
     && 0 == strncmp(URI_REGISTRATION_SEGMENT, uriPath->data, uriPath->len))
    {
        return URI_REGISTRATION;
    }
    if (URI_BOOTSTRAP_SEGMENT_LEN == uriPath->len
     && 0 == strncmp(URI_BOOTSTRAP_SEGMENT, uriPath->data, uriPath->len))
    {
        return URI_BOOTSTRAP;
    }

    readNum = prv_get_number(uriPath->data, uriPath->len);
    if (readNum < 0 || readNum >= LWM2M_URI_NOT_DEFINED) goto error;
    uriP->objID = (uint16_t)readNum;
    if (NULL == uriPath->next) return URI_DM;
    uriPath = uriPath->next;

    // Read object instance
    if (uriPath->len == 0)
    {
        // default instance
        uriP->objInstance = 0;
    }
    else
    {
        readNum = prv_get_number(uriPath->data, uriPath->len);
        if (readNum < 0 || readNum >= LWM2M_URI_NOT_DEFINED) goto error;
        uriP->objInstance = (uint16_t)readNum;
    }
    if (NULL == uriPath->next) return URI_DM;
    uriPath = uriPath->next;

    // Read ressource ID
    if (uriPath->len != 0)
    {
        readNum = prv_get_number(uriPath->data, uriPath->len);
        if (readNum < 0 || readNum >= LWM2M_URI_NOT_DEFINED) goto error;
        uriP->resID = (uint16_t)readNum;
    }

    // must be the last segment
    if (NULL == uriPath->next) return URI_DM;

error:
    memset(uriP, 0xFF, sizeof(lwm2m_uri_t));
    return URI_UNKNOWN;
}
