/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Fabien Fleutot - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

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
#include <ctype.h>

static int prv_parseNumber(uint8_t * uriString,
                            size_t uriLength,
                            size_t * headP)
{
    int result = 0;

    if (uriString[*headP] == '/')
    {
        // empty Object Instance ID with resource ID is not allowed
        return -1;
    }
    while (*headP < uriLength && uriString[*headP] != '/')
    {
        if ('0' <= uriString[*headP] && uriString[*headP] <= '9')
        {
            result *= 10;
            result += uriString[*headP] - '0';
        }
        else
        {
            return -1;
        }
        *headP += 1;
    }

    return result;
}


int uri_getNumber(uint8_t * uriString,
                   size_t uriLength)
{
    size_t index = 0;

    return prv_parseNumber(uriString, uriLength, &index);
}


lwm2m_request_type_t uri_decode(char * altPath,
                                multi_option_t *uriPath,
                                uint8_t code,
                                lwm2m_uri_t * uriP)
{
    int readNum;
    lwm2m_request_type_t requestType = LWM2M_REQUEST_TYPE_DM;

    LOG_ARG("altPath: \"%s\"", STR_NULL2EMPTY(altPath));

    LWM2M_URI_RESET(uriP);

    // Read object ID
    if (NULL != uriPath
     && URI_REGISTRATION_SEGMENT_LEN == uriPath->len
     && 0 == strncmp(URI_REGISTRATION_SEGMENT, (char *)uriPath->data, uriPath->len))
    {
        requestType = LWM2M_REQUEST_TYPE_REGISTRATION;
        uriPath = uriPath->next;
        if (uriPath == NULL) return requestType;
    }
    else if (NULL != uriPath
     && URI_BOOTSTRAP_SEGMENT_LEN == uriPath->len
     && 0 == strncmp(URI_BOOTSTRAP_SEGMENT, (char *)uriPath->data, uriPath->len))
    {
        uriPath = uriPath->next;
        if (uriPath != NULL) goto error;
        return LWM2M_REQUEST_TYPE_BOOTSTRAP;
    }

    if (requestType != LWM2M_REQUEST_TYPE_REGISTRATION)
    {
        // Read altPath if any
        if (altPath != NULL)
        {
            int i;
            if (NULL == uriPath)
            {
                return LWM2M_REQUEST_TYPE_UNKNOWN;
            }
            for (i = 0 ; i < uriPath->len ; i++)
            {
                if (uriPath->data[i] != altPath[i+1])
                {
                    return LWM2M_REQUEST_TYPE_UNKNOWN;
                }
            }
            uriPath = uriPath->next;
        }
        if (NULL == uriPath || uriPath->len == 0)
        {
            if (COAP_DELETE == code)
            {
                return LWM2M_REQUEST_TYPE_DELETE_ALL;
            }
            else
            {
                return LWM2M_REQUEST_TYPE_DM;
            }
        }
    }

    readNum = uri_getNumber(uriPath->data, uriPath->len);
    if (readNum < 0 || readNum >= LWM2M_MAX_ID) goto error;
    uriP->objectId = (uint16_t)readNum;
    uriPath = uriPath->next;

    if (requestType == LWM2M_REQUEST_TYPE_REGISTRATION)
    {
        if (uriPath != NULL) goto error;
        return requestType;
    }

    if (uriPath == NULL) return requestType;

    // Read object instance
    if (uriPath->len != 0)
    {
        readNum = uri_getNumber(uriPath->data, uriPath->len);
        if (readNum < 0 || readNum >= LWM2M_MAX_ID) goto error;
        uriP->instanceId = (uint16_t)readNum;
    }
    uriPath = uriPath->next;

    if (uriPath == NULL) return requestType;

    // Read resource ID
    if (uriPath->len != 0)
    {
        // resource ID without an instance ID is not allowed
        if (!LWM2M_URI_IS_SET_INSTANCE(uriP)) goto error;

        readNum = uri_getNumber(uriPath->data, uriPath->len);
        if (readNum < 0 || readNum >= LWM2M_MAX_ID) goto error;
        uriP->resourceId = (uint16_t)readNum;
    }

#ifndef LWM2M_VERSION_1_0
    uriPath = uriPath->next;
    if (uriPath == NULL) return requestType;
    // Read resource instance ID
    if (uriPath->len != 0)
    {
        // resource instance ID without a resource ID is not allowed
        if (!LWM2M_URI_IS_SET_RESOURCE(uriP)) goto error;

        readNum = uri_getNumber(uriPath->data, uriPath->len);
        if (readNum < 0 || readNum >= LWM2M_MAX_ID) goto error;
        uriP->resourceInstanceId = (uint16_t)readNum;
    }
#endif

    // must be the last segment
    if (NULL == uriPath->next)
    {
        LOG_URI(uriP);
        return requestType;
    }

error:
    LOG("Exiting on error");
    LWM2M_URI_RESET(uriP);
    return LWM2M_REQUEST_TYPE_UNKNOWN;
}

int lwm2m_stringToUri(const char * buffer,
                      size_t buffer_len,
                      lwm2m_uri_t * uriP)
{
    size_t head;
    int readNum;

    LOG_ARG("buffer_len: %u, buffer: \"%.*s\"", buffer_len, buffer_len, STR_NULL2EMPTY(buffer));

    if (uriP == NULL) return 0;

    LWM2M_URI_RESET(uriP);

    if (buffer == NULL || buffer_len == 0) return 0;

    // Skip any white space
    head = 0;
    while (head < buffer_len && isspace(buffer[head]&0xFF))
    {
        head++;
    }
    if (head == buffer_len) return 0;

    // Check the URI start with a '/'
    if (buffer[head] != '/') return 0;
    head++;
    if (head < buffer_len)
    {
        // Read object ID
        readNum = prv_parseNumber((uint8_t *)buffer, buffer_len, &head);
        if (readNum < 0 || readNum >= LWM2M_MAX_ID) return 0;
        uriP->objectId = (uint16_t)readNum;

        if (head < buffer_len && buffer[head] == '/') head += 1;
        if (head < buffer_len)
        {
            readNum = prv_parseNumber((uint8_t *)buffer, buffer_len, &head);
            if (readNum < 0 || readNum >= LWM2M_MAX_ID) return 0;
            uriP->instanceId = (uint16_t)readNum;

            if (head < buffer_len && buffer[head] == '/') head += 1;
            if (head < buffer_len)
            {

                readNum = prv_parseNumber((uint8_t *)buffer, buffer_len, &head);
                if (readNum < 0 || readNum >= LWM2M_MAX_ID) return 0;
                uriP->resourceId = (uint16_t)readNum;

#ifndef LWM2M_VERSION_1_0
                if (head < buffer_len && buffer[head] == '/') head += 1;
                if (head < buffer_len)
                {
                    readNum = prv_parseNumber((uint8_t *)buffer, buffer_len, &head);
                    if (readNum < 0 || readNum >= LWM2M_MAX_ID) return 0;
                    uriP->resourceInstanceId = (uint16_t)readNum;
                }
#endif

                if (head != buffer_len) return 0;
            }
        }
    }

    LOG_ARG("Parsed characters: %u", head);
    LOG_URI(uriP);

    return head;
}

int uri_toString(const lwm2m_uri_t * uriP,
                 uint8_t * buffer,
                 size_t bufferLen,
                 uri_depth_t * depthP)
{
    size_t head = 0;
    uri_depth_t depth = URI_DEPTH_NONE;

    LOG_ARG("bufferLen: %u", bufferLen);
    LOG_URI(uriP);

    if (uriP && LWM2M_URI_IS_SET_OBJECT(uriP))
    {
        int res;
        depth = URI_DEPTH_OBJECT;
        if (head >= bufferLen - 1) return -1;
        buffer[head++] = '/';
        res = utils_intToText(uriP->objectId, buffer + head, bufferLen - head);
        if (res <= 0) return -1;
        head += res;

        if (LWM2M_URI_IS_SET_INSTANCE(uriP))
        {
            depth = URI_DEPTH_OBJECT_INSTANCE;
            if (head >= bufferLen - 1) return -1;
            buffer[head++] = '/';
            res = utils_intToText(uriP->instanceId,
                                  buffer + head,
                                  bufferLen - head);
            if (res <= 0) return -1;
            head += res;
            if (LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                depth = URI_DEPTH_RESOURCE;
                if (head >= bufferLen - 1) return -1;
                buffer[head++] = '/';
                res = utils_intToText(uriP->resourceId,
                                      buffer + head,
                                      bufferLen - head);
                if (res <= 0) return -1;
                head += res;
#ifndef LWM2M_VERSION_1_0
                if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP))
                {
                    depth = URI_DEPTH_RESOURCE_INSTANCE;
                    if (head >= bufferLen - 1) return -1;
                    buffer[head++] = '/';
                    res = utils_intToText(uriP->resourceInstanceId,
                                          buffer + head,
                                          bufferLen - head);
                    if (res <= 0) return -1;
                    head += res;
                }
#endif
            }
        }
    }
    else if(uriP)
    {
        if (head >= bufferLen - 1) return -1;
        buffer[head++] = '/';
    }

    if (depthP) *depthP = depth;

    LOG_ARG("length: %u, buffer: \"%.*s\"", head, head, buffer);

    return head;
}
