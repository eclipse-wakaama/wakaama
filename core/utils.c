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
#include <float.h>


int lwm2m_PlainTextToInt64(uint8_t * buffer,
                           int length,
                           int64_t * dataP)
{
    uint64_t result = 0;
    int sign = 1;
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
            if (result > (UINT64_MAX / 10)) return 0;
            result *= 10;
            result += buffer[i] - '0';
        }
        else
        {
            return 0;
        }
        i++;
    }

    if (result > INT64_MAX) return 0;

    if (sign == -1)
    {
        *dataP = 0 - result;
    }
    else
    {
        *dataP = result;
    }

    return 1;
}

int lwm2m_PlainTextToFloat64(uint8_t * buffer,
                             int length,
                             double * dataP)
{
    double result;
    int sign;
    int i;

    if (0 == length) return 0;

    if (buffer[0] == '-')
    {
        sign = -1;
        i = 1;
    }
    else
    {
        sign = 1;
        i = 0;
    }

    result = 0;
    while (i < length && buffer[i] != '.')
    {
        if ('0' <= buffer[i] && buffer[i] <= '9')
        {
            if (result > (DBL_MAX / 10)) return 0;
            result *= 10;
            result += (buffer[i] - '0');
        }
        else
        {
            return 0;
        }
        i++;
    }
    if (buffer[i] == '.')
    {
        double dec;

        i++;
        if (i == length) return 0;

        dec = 0.1;
        while (i < length)
        {
            if ('0' <= buffer[i] && buffer[i] <= '9')
            {
                if (result > (DBL_MAX - 1)) return 0;
                result += (buffer[i] - '0') * dec;
                dec /= 10;
            }
            else
            {
                return 0;
            }
            i++;
        }
    }

    *dataP = result * sign;
    return 1;
}

static size_t prv_intToText(int64_t data,
                            uint8_t * string,
                            size_t length)
{
    int index;
    bool minus;

    if (data < 0)
    {
        minus = true;
        data = 0 - data;
    }
    else
    {
        minus = false;
    }

    index = length - 1;
    do
    {
        string[index] = '0' + data%10;
        data /= 10;
        index --;
    } while (index >= 0 && data > 0);

    if (data > 0) return 0;

    if (minus == true)
    {
        if (index == 0) return 0;
        string[index] = '-';
    }
    else
    {
        index++;
    }

    return length - index;
}

size_t lwm2m_int64ToPlainText(int64_t data,
                              uint8_t ** bufferP)
{
#define _PRV_STR_LENGTH 32
    uint8_t string[_PRV_STR_LENGTH];
    size_t length;

    length = prv_intToText(data, string, _PRV_STR_LENGTH);
    if (length == 0) return 0;

    *bufferP = (uint8_t *)lwm2m_malloc(length);
    if (NULL == *bufferP) return 0;

    memcpy(*bufferP, string + _PRV_STR_LENGTH - length, length);

    return length;
}


size_t lwm2m_float64ToPlainText(double data,
                                uint8_t ** bufferP)
{
#define _PRV_PRECISION 16
    uint8_t intString[_PRV_STR_LENGTH];
    size_t intLength;
    uint8_t decString[_PRV_STR_LENGTH];
    size_t decLength;
    int64_t intPart;
    double decPart;
    int i;

    intPart = (int64_t)data;
    decPart = data - intPart;
    if (decPart < 0)
    {
        decPart = 1 - decPart;
    }
    else
    {
        decPart = 1 + decPart;
    }

    if (decPart <= 1 + FLT_EPSILON)
    {
        return lwm2m_int64ToPlainText(intPart, bufferP);
    }

    intLength = prv_intToText(intPart, intString, _PRV_STR_LENGTH);
    if (intLength == 0) return 0;

    i = 0;
    do
    {
        decPart *= 10;
        i++;
    } while ((decPart - (int64_t)decPart > 0)
          && (i < _PRV_PRECISION));

    decLength = prv_intToText(decPart, decString, _PRV_STR_LENGTH);
    if (decLength <= 1) return 0;

    *bufferP = (uint8_t *)lwm2m_malloc(intLength + 1 + decLength);
    if (NULL == *bufferP) return 0;

    memcpy(*bufferP, intString + _PRV_STR_LENGTH - intLength, intLength);
    (*bufferP)[intLength] = '.';
    memcpy(*bufferP + intLength + 1, decString + _PRV_STR_LENGTH - decLength + 1, decLength - 1);

    return intLength + decLength;   // +1 for dot, -1 for extra "1" in decPart
}


size_t lwm2m_boolToPlainText(bool data,
                             uint8_t ** bufferP)
{
    return lwm2m_int64ToPlainText((int64_t)(data?1:0), bufferP);
}

lwm2m_binding_t lwm2m_stringToBinding(uint8_t * buffer,
                                      size_t length)
{
    if (length == 0) return BINDING_UNKNOWN;

    switch (buffer[0])
    {
    case 'U':
        switch (length)
        {
        case 1:
            return BINDING_U;
        case 2:
            switch (buffer[1])
            {
            case 'Q':
                 return BINDING_UQ;
            case 'S':
                 return BINDING_US;
            default:
                break;
            }
            break;
        case 3:
            if (buffer[1] == 'Q' && buffer[2] == 'S')
            {
                return BINDING_UQS;
            }
            break;
        default:
            break;
        }
        break;

        case 'S':
            switch (length)
            {
            case 1:
                return BINDING_S;
            case 2:
                if (buffer[1] == 'Q')
                {
                    return BINDING_SQ;
                }
                break;
            default:
                break;
            }
            break;

        default:
            break;
    }

    return BINDING_UNKNOWN;
}

#ifdef LWM2M_CLIENT_MODE
lwm2m_server_t * prv_findServer(lwm2m_context_t * contextP,
                                void * fromSessionH)
{
    lwm2m_server_t * targetP;

    targetP = contextP->serverList;
    while (targetP != NULL
        && targetP->sessionH != fromSessionH)
    {
        targetP = targetP->next;
    }

    return targetP;
}
#endif

lwm2m_server_t * utils_findBootstrapServer(lwm2m_context_t * contextP,
                                           void * fromSessionH)
{
#ifdef LWM2M_CLIENT_MODE

    lwm2m_server_t * targetP;

    targetP = contextP->bootstrapServerList;
    while (targetP != NULL
        && targetP->sessionH != fromSessionH)
    {
        targetP = targetP->next;
    }

    return targetP;

#else

    return NULL;

#endif
}

int prv_isAltPathValid(char * altPath)
{
    int i;

    if (altPath == NULL) return 0;

    if (altPath[0] != '/') return 0;

    for (i = 1 ; altPath[i] != 0 ; i++)
    {
        // TODO: Support multi-segment alternative path
        if (altPath[i] == '/') return 0;
        // TODO: Check needs for sub-delims, ':' and '@'
        if ((altPath[i] < 'A' || altPath[i] > 'Z')      // ALPHA
         && (altPath[i] < 'a' || altPath[i] > 'z')
         && (altPath[i] < '0' || altPath[i] > '9')      // DIGIT
         && (altPath[i] != '-')                         // Other unreserved
         && (altPath[i] != '.')
         && (altPath[i] != '_')
         && (altPath[i] != '~')
         && (altPath[i] != '%'))                        // pct_encoded
        {
            return 0;
        }

    }
    return 1;
}

#ifndef LWM2M_EMBEDDED_MODE
time_t lwm2m_gettime(void)
{
    struct timeval tv;

    if (0 != gettimeofday(&tv, NULL))
    {
        return -1;
    }

    return tv.tv_sec;
}
#endif
