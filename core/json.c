/*******************************************************************************
 *
 * Copyright (c) 2015 Intel Corporation and others.
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
 *
 *******************************************************************************/

#include "internals.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#define JSON_ARRAY_TOKEN            "\"e\""
#define JSON_ARRAY_TOKEN_LEN        3
#define JSON_BASE_NAME_TOKEN        "\"bn\""
#define JSON_BASE_NAME_TOKEN_LEN    4
#define JSON_BASE_TIME_TOKEN        "\"bt\""
#define JSON_BASE_TIME_TOKEN_LEN    4
#define JSON_NAME_TOKEN             "\"n\""
#define JSON_NAME_TOKEN_LEN         3
#define JSON_FLOAT_TOKEN            "\"v\""
#define JSON_FLOAT_TOKEN_LEN        3
#define JSON_BOOLEAN_TOKEN          "\"bv\""
#define JSON_BOOLEAN_TOKEN_LEN      4
#define JSON_STRING_TOKEN           "\"sv\""
#define JSON_STRING_TOKEN_LEN       4

#define JSON_MIN_ARRAY_LEN      21  // e":[{"n":"N","v":X}]}
#define JSON_MIN_BASE_LEN        7  // n":"N",

#define _GO_TO_NEXT_CHAR(I,B,L)         \
    {                                   \
        I++;                            \
        I += prv_skipSpace(B+I, L-I);   \
        if (I == L) return -1;    \
    }

static int prv_skipSpace(char * buffer,
                         size_t bufferLen)
{
    int i;

    i = 0;
    while ((i < bufferLen)
        && (buffer[i] == 0x20
         || buffer[i] == 0x09
         || buffer[i] == 0x0A
         || buffer[i] == 0x0D))
    {
        i++;
    }

    return i;
}

static int prv_findSeparator(char * buffer,
                             size_t bufferLen)
{
    int i;

    i = 0;
    while (i < bufferLen)
    {
        if (buffer[i] == ':') return i;
        if (buffer[i] == '"')
        {
            // skip strings
            i++;
            while ((i < bufferLen)
                && (buffer[i] != '"'))
            {
                i++;
            }
        }
        // reserved characters
        switch(buffer[i])
        {
        case ',':
        case '{':
        case '}':
             return 0;
        }
        i++;
    }
    if (i == bufferLen) i = 0;

    return i;
}

static int prv_countItems(char * buffer,
                          size_t bufferLen)
{
    int count;
    int index;
    int in;

    count = 0;
    index = 0;
    in = 0;

    while (index < bufferLen)
    {
        if (in == 0)
        {
            if (buffer[index] != '{') return -1;
            in = 1;
            _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
        }
        else
        {
            if (buffer[index] == '{') return -1;
            if (buffer[index] == '}')
            {
                in = 0;
                count++;
                _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
                if (buffer[index] == ']')
                {
                    break;
                }
                if (buffer[index] != ',') return -1;
                _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
            }
            else
            {
                _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
            }
        }
    }
    if (in == 1) return -1;

    return count;
}

static int prv_parseItem(char * buffer,
                         size_t bufferLen,
                         lwm2m_tlv_t * tlvP)
{
    return 0;
}

int lwm2m_tlv_parse_json(char * buffer,
                         size_t bufferLen,
                         lwm2m_tlv_t ** dataP)
{
    int index;
    int count = 0;
    bool eFound = false;
    bool bnFound = false;
    bool btFound = false;

    index = prv_skipSpace(buffer, bufferLen);
    if (index == bufferLen) return -1;

    if (buffer[index] != '{') return -1;
    _GO_TO_NEXT_CHAR(index, buffer+index, bufferLen-index);
    if (buffer[index] != '"') return -1;
    if (index++ >= bufferLen) return -1;
    switch (buffer[index])
    {
    case 'e':
    {
        lwm2m_tlv_t * tlvP;
        int tlvIndex;

        if (eFound == true) return -1;
        eFound = true;

        if (bufferLen-index < JSON_MIN_ARRAY_LEN) return -1;
        index++;
        if (buffer[index] != '"') return -1;
        _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
        if (buffer[index] != ':') return -1;
        _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
        if (buffer[index] != '[') return -1;
        _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
        count = prv_countItems(buffer + index, bufferLen - index);
        if (count <= 0) return -1;
        tlvP = lwm2m_tlv_new(count);
        if (tlvP == NULL) return -1;
        // at this point we are sure buffer[index] is '{' and all { and } are matching
        tlvIndex = 0;
        while (tlvIndex < count)
        {
            int itemLen;

            if (buffer[index] != '{') return -1;
            itemLen = 0;
            while (buffer[index + itemLen] != '}') itemLen++;
            if (0 != prv_parseItem(buffer + index, itemLen, tlvP + tlvIndex))
            {
                lwm2m_tlv_free(count, tlvP);
                return -1;
            }
            tlvIndex++;
            index += itemLen;
            _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
            switch (buffer[index])
            {
            case ',':
                _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
                break;
            case ']':
                if (tlvIndex == count) break;
                // else this is an error
            default:
                lwm2m_tlv_free(count, tlvP);
                return -1;
            }
        }
        if (buffer[index] != ']')
        {
            lwm2m_tlv_free(count, tlvP);
            return -1;
        }
        // temp
        *dataP = tlvP;
    }
    break;

    case 'b':
    default:
        // unsupported
        return -1;
    }

    return count;
}

int lwm2m_tlv_serialize_json(int size,
                             lwm2m_tlv_t * tlvP,
                             char ** bufferP)
{
    return 0;
}
