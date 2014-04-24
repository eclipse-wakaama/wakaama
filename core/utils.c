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

