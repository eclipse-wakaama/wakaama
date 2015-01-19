/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Toby Jaffey - Please refer to git log
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
#include <stdio.h>
#include <inttypes.h>


int lwm2m_PlainTextToInt64(char * buffer,
                           int length,
                           int64_t * dataP)
{
    uint64_t result = 0;
    int sign = 1;
    int mul = 0;
    int i = 0;

    if (0 == length) return 0;

    if (buffer[0] == '-')
    {
        sign = -1;
        i = 1;
    }

    while (i < length)
    {
        if ('0' <= buffer[i] && buffer[i] <= '9')
        {
            if (0 == mul)
            {
                mul = 10;
            }
            else
            {
                result *= mul;
            }
            result += buffer[i] - '0';
        }
        else
        {
            return 0;
        }
        i++;
    }

    *dataP = result * sign;
    return 1;
}

int lwm2m_int8ToPlainText(int8_t data,
                          char ** bufferP)
{
    return lwm2m_int64ToPlainText((int64_t)data, bufferP);
}

int lwm2m_int16ToPlainText(int16_t data,
                           char ** bufferP)
{
    return lwm2m_int64ToPlainText((int64_t)data, bufferP);
}

int lwm2m_int32ToPlainText(int32_t data,
                           char ** bufferP)
{
    return lwm2m_int64ToPlainText((int64_t)data, bufferP);
}

int lwm2m_int64ToPlainText(int64_t data,
                           char ** bufferP)
{
    char string[32];
    int len;

    len = snprintf(string, 32, "%" PRId64, data);
    if (len > 0)
    {
        *bufferP = (char *)lwm2m_malloc(len);

        if (NULL != *bufferP)
        {
            strncpy(*bufferP, string, len);
        }
        else
        {
            len = 0;
        }
    }

    return len;
}


int lwm2m_float32ToPlainText(float data,
                             char ** bufferP)
{
    return lwm2m_float64ToPlainText((double)data, bufferP);
}


int lwm2m_float64ToPlainText(double data,
                             char ** bufferP)
{
    char string[64];
    int len;

    len = snprintf(string, 64, "%lf", data);
    if (len > 0)
    {
        *bufferP = (char *)lwm2m_malloc(len);

        if (NULL != *bufferP)
        {
            strncpy(*bufferP, string, len);
        }
        else
        {
            len = 0;
        }
    }

    return len;
}


int lwm2m_boolToPlainText(bool data,
                          char ** bufferP)
{
    return lwm2m_int64ToPlainText((int64_t)(data?1:0), bufferP);
}

lwm2m_binding_t lwm2m_stringToBinding(const char *buffer,
                                      size_t length)
{
    // test order is important
    if (strncmp(buffer, "U", length) == 0)
    {
        return BINDING_U;
    }
    if (strncmp(buffer, "S", length) == 0)
    {
        return BINDING_S;
    }
    if (strncmp(buffer, "UQ", length) == 0)
    {
        return BINDING_UQ;
    }
    if (strncmp(buffer, "SQ", length) == 0)
    {
        return BINDING_SQ;
    }
    if (strncmp(buffer, "US", length) == 0)
    {
        return BINDING_UQ;
    }
    if (strncmp(buffer, "UQS", length) == 0)
    {
        return BINDING_UQ;
    }

    return BINDING_UNKNOWN;
}

#define CODE_TO_STRING(X) 	case X : return "(" #X ")"

const char* lwm2m_statusToString(int status)
{
	switch(status) {
	CODE_TO_STRING(COAP_201_CREATED);
	CODE_TO_STRING(COAP_202_DELETED);
	CODE_TO_STRING(COAP_204_CHANGED);
	CODE_TO_STRING(COAP_205_CONTENT);
	CODE_TO_STRING(COAP_400_BAD_REQUEST);
	CODE_TO_STRING(COAP_401_UNAUTHORIZED);
	CODE_TO_STRING(COAP_404_NOT_FOUND);
	CODE_TO_STRING(COAP_405_METHOD_NOT_ALLOWED);
	CODE_TO_STRING(COAP_406_NOT_ACCEPTABLE);
	CODE_TO_STRING(COAP_500_INTERNAL_SERVER_ERROR);
	CODE_TO_STRING(COAP_501_NOT_IMPLEMENTED);
	CODE_TO_STRING(COAP_503_SERVICE_UNAVAILABLE);
	default: return "";
	}
}

int lwm2m_adjustTimeout(time_t nextTime, time_t currentTime, struct timeval* timeoutP)
{
    int left = 0; 
    time_t interval;

    if (nextTime > currentTime)
    {
        interval = nextTime - currentTime;
        left = interval;
    }
    else
    {
        interval = 1;
    }

    if (timeoutP->tv_sec > interval)
    {
        
        timeoutP->tv_sec = interval;
    }
    LOG("time %ld, next %ld, timeout %ld, left %d\n", currentTime, nextTime, timeoutP->tv_sec, left);

    return left;
}
