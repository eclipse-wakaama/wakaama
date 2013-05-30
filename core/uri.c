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

#include "liblwm2m.h"
#include "internals.h"
#include <stdlib.h>
#include <string.h>


static int prv_get_number(const char * uriString,
                          size_t uriLength,
                          int * headP)
{
    int result = 0;
    int mul = 0;

    while (*headP < uriLength && uriString[*headP] != '/')
    {
        if ('0' <= uriString[*headP] && uriString[*headP] <= '9')
        {
            if (0 == mul)
            {
                mul = 10;
            }
            else
            {
                result *= mul;
                mul *= 10;
            }
            result += uriString[*headP] - '0';
        }
        else
        {
            return -1;
        }
        *headP += 1;
    }

    if (uriString[*headP] == '/')
        *headP += 1;

    return result;
}


lwm2m_uri_t * lwm2m_decode_uri(const char * uriString,
                               size_t uriLength)
{
    lwm2m_uri_t * uriP;
    int head = 0;
    int readNum;

    if (NULL == uriString || 0 == uriLength) return NULL;

    uriP = (lwm2m_uri_t *)malloc(sizeof(lwm2m_uri_t));
    if (NULL == uriP) return NULL;

    memset(uriP, 0xFF, sizeof(lwm2m_uri_t));

    // Read object ID
    readNum = prv_get_number(uriString, uriLength, &head);
    if (readNum < 0 || readNum >= LWM2M_URI_NOT_DEFINED) goto error;
    uriP->objID = (uint16_t)readNum;
    if (head >= uriLength) return uriP;

    // Read object instance
    if (uriString[head] == '/')
    {
        // no instance
        head++;
    }
    else
    {
        readNum = prv_get_number(uriString, uriLength, &head);
        if (readNum < 0 || readNum >= LWM2M_URI_NOT_DEFINED) goto error;
        uriP->objInstance = (uint16_t)readNum;
    }
    if (head >= uriLength) return uriP;

    // Read ressource ID
    if (uriString[head] == '/')
    {
        // no ID
        head++;
    }
    else
    {
        readNum = prv_get_number(uriString, uriLength, &head);
        if (readNum < 0 || readNum >= LWM2M_URI_NOT_DEFINED) goto error;
        uriP->resID = (uint16_t)readNum;
    }
    if (head >= uriLength) return uriP;

    // Read ressource instance
    if (uriString[head] == '/')
    {
        // no instance
        head++;
    }
    else
    {
        if (LWM2M_URI_NOT_DEFINED == uriP->resID) goto error;
        readNum = prv_get_number(uriString, uriLength, &head);
        if (readNum < 0 || readNum >= LWM2M_URI_NOT_DEFINED) goto error;
        uriP->resInstance = (uint16_t)readNum;
    }
    if (head < uriLength) goto error;

    return uriP;

error:
    free(uriP);
    return NULL;
}
