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

#ifdef LWM2M_SUPPORT_JSON

#define PRV_JSON_BUFFER_SIZE 1024

#define JSON_MIN_ARRAY_LEN      21      // e":[{"n":"N","v":X}]}
#define JSON_MIN_BASE_LEN        7      // n":"N",
#define JSON_ITEM_MAX_SIZE      36      // with ten characters for value

#define JSON_FALSE_STRING  "false"
#define JSON_TRUE_STRING   "true"

#define JSON_RES_ITEM_TEMPLATE      "{\"n\":\"%d\","
#define JSON_INST_ITEM_TEMPLATE     "{\"n\":\"%d/%d\","
#define JSON_ITEM_BOOL_TRUE         "\"bv\":true},"
#define JSON_ITEM_BOOL_FALSE        "\"bv\":false},"
#define JSON_ITEM_FLOAT_TEMPLATE    "\"v\":%.16g},"
#define JSON_ITEM_INTEGER_TEMPLATE  "\"v\":%"PRId64"},"
#define JSON_ITEM_STRING_BEGIN      "\"sv\":\""
#define JSON_ITEM_STRING_END        "\"},"

#define JSON_HEADER             "{\"e\":["
#define JSON_FOOTER             "]}"
#define JSON_HEADER_SIZE        6
#define JSON_FOOTER_SIZE        2


#define _GO_TO_NEXT_CHAR(I,B,L)         \
    {                                   \
        I++;                            \
        I += prv_skipSpace(B+I, L-I);   \
        if (I == L) return -1;    \
    }

typedef enum
{
    _STEP_START,
    _STEP_TOKEN,
    _STEP_ANY_SEPARATOR,
    _STEP_SEPARATOR,
    _STEP_QUOTED_VALUE,
    _STEP_VALUE,
    _STEP_DONE
} _itemState;

typedef enum
{
    _TYPE_UNSET,
    _TYPE_FALSE,
    _TYPE_TRUE,
    _TYPE_FLOAT,
    _TYPE_STRING
} _type;

typedef struct
{
    uint16_t    resId;
    uint16_t    resInstId;
    _type       type;
    uint8_t *      value;
    size_t      valueLen;
} _record_t;

static int prv_isReserved(char sign)
{
    if (sign == '['
     || sign == '{'
     || sign == ']'
     || sign == '}'
     || sign == ':'
     || sign == ','
     || sign == '"')
    {
        return 1;
    }

    return 0;
}

static int prv_isWhiteSpace(uint8_t sign)
{
    if (sign == 0x20
     || sign == 0x09
     || sign == 0x0A
     || sign == 0x0D)
    {
        return 1;
    }

    return 0;
}

static size_t prv_skipSpace(uint8_t * buffer,
                            size_t bufferLen)
{
    size_t i;

    i = 0;
    while ((i < bufferLen)
        && prv_isWhiteSpace(buffer[i]))
    {
        i++;
    }

    return i;
}

static int prv_split(uint8_t * buffer,
                     size_t bufferLen,
                     int * tokenStartP,
                     int * tokenLenP,
                     int * valueStartP,
                     int * valueLenP)
{
    size_t index;
    _itemState step;

    index = 0;
    step = _STEP_START;

    index = prv_skipSpace(buffer + index, bufferLen - index);
    if (index == bufferLen) return -1;

    while ((index < bufferLen)
        && (buffer[index] != ','))
    {
        switch (step)
        {
        case _STEP_START:
            if (buffer[index] != '"') return -1;
            *tokenStartP = index+1;
            step = _STEP_TOKEN;
            break;

        case _STEP_TOKEN:
            if (buffer[index] == '"')
            {
                *tokenLenP = index - *tokenStartP;
                step = _STEP_ANY_SEPARATOR;
            }
            break;

        case _STEP_ANY_SEPARATOR:
            if (buffer[index] != ':') return -1;
            step = _STEP_SEPARATOR;
            break;

        case _STEP_SEPARATOR:
            if (buffer[index] == '"')
            {
                *valueStartP = index;
                step = _STEP_QUOTED_VALUE;
            } else if (!prv_isReserved(buffer[index]))
            {
                *valueStartP = index;
                step = _STEP_VALUE;
            } else
            {
                return -1;
            }
            break;

        case _STEP_QUOTED_VALUE:
            if (buffer[index] == '"' && buffer[index-1] != '\\' )
            {
                *valueLenP = index - *valueStartP + 1;
                step = _STEP_DONE;
            }
            break;

        case _STEP_VALUE:
            if (prv_isWhiteSpace(buffer[index]))
            {
                *valueLenP = index - *valueStartP;
                step = _STEP_DONE;
            }
            break;

        case _STEP_DONE:
        default:
            return -1;
        }

        index++;
        if (step == _STEP_START
         || step == _STEP_ANY_SEPARATOR
         || step == _STEP_SEPARATOR
         || step == _STEP_DONE)
        {
            index += prv_skipSpace(buffer + index, bufferLen - index);
        }
    }

    if (step == _STEP_VALUE)
    {
        *valueLenP = index - *valueStartP;
        step = _STEP_DONE;
    }

    if (step != _STEP_DONE) return -1;

    return (int)index;
}

static int prv_countItems(uint8_t * buffer,
                          size_t bufferLen)
{
    int count;
    size_t index;
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

static int prv_parseItem(uint8_t * buffer,
                         size_t bufferLen,
                         _record_t * recordP)
{
    size_t index;

    recordP->resId = LWM2M_MAX_ID;
    recordP->resInstId = LWM2M_MAX_ID;
    recordP->type = _TYPE_UNSET;
    recordP->value = NULL;
    recordP->valueLen = 0;

    index = 0;
    do
    {
        int tokenStart;
        int tokenLen;
        int valueStart;
        int valueLen;
        int next;

        next = prv_split(buffer+index, bufferLen-index, &tokenStart, &tokenLen, &valueStart, &valueLen);
        if (next < 0) return -1;

        switch (tokenLen)
        {
        case 1:
        {
            switch (buffer[index+tokenStart])
            {
            case 'n':
            {
                int i;
                uint32_t readId;

                if (recordP->resId != LWM2M_MAX_ID) return -1;

                // Check for " around URI
                if (valueLen < 3
                 || buffer[index+valueStart] != '"'
                 || buffer[index+valueStart+valueLen-1] != '"')
                {
                    return -1;
                }
                i = 1;
                readId = 0;
                while (i < valueLen-1 && buffer[index+valueStart+i] != '/')
                {
                    if (buffer[index+valueStart+i] < '0'
                     || buffer[index+valueStart+i] > '9')
                    {
                        return -1;
                    }
                    readId *= 10;
                    readId += buffer[index+valueStart+i] - '0';
                    if (readId > LWM2M_MAX_ID) return -1;
                    i++;
                }
                recordP->resId = readId;
                if (buffer[index+valueStart+i] == '/')
                {
                    int j;

                    if (i == valueLen-1) return -1;

                    j = 1;
                    readId = 0;
                    while (i+j < valueLen-1)
                    {
                        if (buffer[index+valueStart+i+j] < '0'
                         || buffer[index+valueStart+i+j] > '9')
                        {
                            return -1;
                        }
                        readId *= 10;
                        readId += buffer[index+valueStart+i+j] - '0';
                        if (readId > LWM2M_MAX_ID) return -1;
                        i++;
                    }
                    recordP->resInstId = readId;
                }
                // TODO: support more URIs than just res and res/instance
            }
            break;

            case 'v':
                if (recordP->type != _TYPE_UNSET) return -1;
                recordP->type = _TYPE_FLOAT;
                recordP->value = buffer + index + valueStart;
                recordP->valueLen = valueLen;
                break;

            case 't':
                // TODO: support time
                break;

            default:
                return -1;
            }
        }
        break;

        case 2:
        {
            // "bv", "ov", or "sv"
            if (buffer[index+tokenStart+1] != 'v') return -1;
            switch (buffer[index+tokenStart])
            {
            case 'b':
                if (recordP->type != _TYPE_UNSET) return -1;
                if (0 == lwm2m_strncmp(JSON_TRUE_STRING, (char *)buffer + index + valueStart, valueLen))
                {
                    recordP->type = _TYPE_TRUE;
                }
                else if (0 == lwm2m_strncmp(JSON_FALSE_STRING, (char *)buffer + index + valueStart, valueLen))
                {
                    recordP->type = _TYPE_FALSE;
                }
                else
                {
                    return -1;
                }
                break;

            case 'o':
                if (recordP->type != _TYPE_UNSET) return -1;
                // TODO: support object link
                break;

            case 's':
                if (recordP->type != _TYPE_UNSET) return -1;
                // Check for " around value
                if (valueLen < 2
                 || buffer[index+valueStart] != '"'
                 || buffer[index+valueStart+valueLen-1] != '"')
                {
                    return -1;
                }
                recordP->type = _TYPE_STRING;
                recordP->value = buffer + index + valueStart + 1;
                recordP->valueLen = valueLen - 2;
                break;

            default:
                return -1;
            }
        }
        break;

        default:
            return -1;
        }

        index += next + 1;
    } while (index < bufferLen);

    return 0;
}

static int prv_convertRecord(_record_t * recordArray,
                             int count,
                             lwm2m_data_t ** dataP)
{
    int index;
    int tlvIndex;
    lwm2m_data_t * tlvP;

    // may be overkill
    tlvP = lwm2m_data_new(count);
    if (NULL == tlvP) return -1;
    tlvIndex = 0;

    for (index = 0 ; index < count ; index++)
    {
        lwm2m_data_t * targetP;

        if (recordArray[index].resInstId == LWM2M_MAX_ID)
        {
            targetP = tlvP + tlvIndex;
            targetP->type = LWM2M_TYPE_RESOURCE;
            targetP->id = recordArray[index].resId;
            tlvIndex++;
        }
        else
        {
            int resIndex;

            resIndex = 0;
            while (resIndex < tlvIndex
                && tlvP[resIndex].id != recordArray[index].resId)
            {
                resIndex++;
            }
            if (resIndex == tlvIndex)
            {
                targetP = lwm2m_data_new(1);
                if (NULL == targetP) goto error;

                tlvP[resIndex].type = LWM2M_TYPE_MULTIPLE_RESOURCE;
                tlvP[resIndex].id = recordArray[index].resId;
                tlvP[resIndex].length = 1;
                tlvP[resIndex].value = (uint8_t *)targetP;

                tlvIndex++;
            }
            else
            {
                targetP = lwm2m_data_new(tlvP[resIndex].length + 1);
                if (NULL == targetP) goto error;

                memcpy(targetP + 1, tlvP[resIndex].value, tlvP[resIndex].length * sizeof(lwm2m_data_t));
                lwm2m_free(tlvP[resIndex].value);   // do not use lwm2m_data_free() to preserve value pointers
                tlvP[resIndex].value = (uint8_t *)targetP;
                tlvP[resIndex].length++;
            }

            targetP->type = LWM2M_TYPE_RESOURCE_INSTANCE;
            targetP->id = recordArray[index].resInstId;
        }
        switch (recordArray[index].type)
        {
        case _TYPE_FALSE:
            lwm2m_data_encode_bool(false, targetP);
            break;
        case _TYPE_TRUE:
            lwm2m_data_encode_bool(true, targetP);
            break;
        case _TYPE_FLOAT:
        {
            size_t i;

            i = 0;
            while (i < recordArray[index].valueLen
                && recordArray[index].value[i] != '.')
            {
                i++;
            }
            if (i == recordArray[index].valueLen)
            {
                int64_t value;

                if ( 1 != lwm2m_PlainTextToInt64(recordArray[index].value,
                                                 recordArray[index].valueLen,
                                                 &value))
                {
                    goto error;
                }

                lwm2m_data_encode_int(value, targetP);
            }
            else
            {
                double value;

                if ( 1 != lwm2m_PlainTextToFloat64(recordArray[index].value,
                                                   recordArray[index].valueLen,
                                                   &value))
                {
                    goto error;
                }

                lwm2m_data_encode_float(value, targetP);
            }
        }
        break;

            // TODO: Copy string instead of pointing to it
        case _TYPE_STRING:
            targetP->flags = LWM2M_TLV_FLAG_STATIC_DATA | LWM2M_TLV_FLAG_TEXT_FORMAT;
            targetP->dataType = LWM2M_TYPE_STRING;      // or opaque ?
            targetP->length = recordArray[index].valueLen;
            targetP->value = recordArray[index].value;
            break;

        case _TYPE_UNSET:
        default:
            goto error;
        }
    }

    *dataP = tlvP;
    return tlvIndex;

error:
    lwm2m_data_free(count, tlvP);
    return -1;
}

int lwm2m_json_parse(uint8_t * buffer,
                     size_t bufferLen,
                     lwm2m_data_t ** dataP)
{
    size_t index;
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
        _record_t * recordArray;
        int recordIndex;

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
        recordArray = (_record_t*)lwm2m_malloc(count * sizeof(_record_t));
        if (recordArray == NULL) return -1;
        // at this point we are sure buffer[index] is '{' and all { and } are matching
        recordIndex = 0;
        while (recordIndex < count)
        {
            int itemLen;

            if (buffer[index] != '{') return -1;
            itemLen = 0;
            while (buffer[index + itemLen] != '}') itemLen++;
            if (0 != prv_parseItem(buffer + index + 1, itemLen - 1, recordArray + recordIndex))
            {
                lwm2m_free(recordArray);
                return -1;
            }
            recordIndex++;
            index += itemLen;
            _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
            switch (buffer[index])
            {
            case ',':
                _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
                break;
            case ']':
                if (recordIndex == count) break;
                // else this is an error
            default:
                lwm2m_free(recordArray);
                return -1;
            }
        }
        if (buffer[index] != ']')
        {
            lwm2m_free(recordArray);
            return -1;
        }
        count = prv_convertRecord(recordArray, count, dataP);
        lwm2m_free(recordArray);
    }
    break;

    case 'b':
    default:
        // TODO: support for basename and base time.
        return 0;
    }

    return count;
}

static int prv_serializeValue(lwm2m_data_t * tlvP,
                              uint8_t * buffer,
                              size_t bufferLen)
{
    int res;
    int head;

    res = lwm2m_snprintf((char *)(char *)buffer, bufferLen, JSON_RES_ITEM_TEMPLATE, tlvP->id);
    if (res <= 0 || res >= bufferLen) return -1;

    switch (tlvP->dataType)
    {
    case LWM2M_TYPE_STRING:
        res = lwm2m_snprintf((char *)buffer, bufferLen, JSON_ITEM_STRING_BEGIN);
        if (res <= 0 || res >= bufferLen) return -1;
        head = res;
        if (tlvP->length >= bufferLen - head) return -1;
        memcpy(buffer + head, tlvP->value, tlvP->length);
        head += tlvP->length;
        res = lwm2m_snprintf((char *)buffer + head, bufferLen - head, JSON_ITEM_STRING_END);
        if (res <= 0 || res >= bufferLen - head) return -1;
        res += head;
        break;

    case LWM2M_TYPE_INTEGER:
    case LWM2M_TYPE_TIME:
    {
        int64_t value;

        if (0 == lwm2m_data_decode_int(tlvP, &value)) return -1;
        res = lwm2m_snprintf((char *)buffer, bufferLen, JSON_ITEM_INTEGER_TEMPLATE, value);
        if (res <= 0 || res >= bufferLen) return -1;
    }
    break;

    case LWM2M_TYPE_FLOAT:
    {
        double value;

        if (0 == lwm2m_data_decode_float(tlvP, &value)) return -1;
        res = lwm2m_snprintf((char *)buffer, bufferLen, JSON_ITEM_FLOAT_TEMPLATE, value);
        if (res <= 0 || res >= bufferLen) return -1;
    }
    break;

    case LWM2M_TYPE_BOOLEAN:
    {
        bool value;

        if (0 == lwm2m_data_decode_bool(tlvP, &value)) return -1;
        res = lwm2m_snprintf((char *)buffer, bufferLen, value?JSON_ITEM_BOOL_TRUE:JSON_ITEM_BOOL_FALSE);
        if (res <= 0 || res >= bufferLen) return -1;
    }
    break;

    case LWM2M_TYPE_OPAQUE:
        // TODO: base64 encoding
        res = lwm2m_snprintf((char *)buffer, bufferLen, JSON_ITEM_STRING_BEGIN);
        if (res <= 0 || res >= bufferLen) return -1;
        head = res;
        if (tlvP->length >= bufferLen - head) return -1;
        memcpy(buffer + head, tlvP->value, tlvP->length);
        head += tlvP->length;
        res = lwm2m_snprintf((char *)buffer + head, bufferLen - head, JSON_ITEM_STRING_END);
        if (res <= 0 || res >= bufferLen - head) return -1;
        res += head;
        break;

    case LWM2M_TYPE_OBJECT_LINK:
        // TODO: implement
        return -1;

    case LWM2M_TYPE_UNDEFINED:
        if ((tlvP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
        {
            res = lwm2m_snprintf((char *)buffer, bufferLen, JSON_ITEM_STRING_BEGIN);
            if (res <= 0 || res >= bufferLen) return -1;
            head = res;
            if (tlvP->length >= bufferLen - head) return -1;
            memcpy(buffer + head, tlvP->value, tlvP->length);
            head += tlvP->length;
            res = lwm2m_snprintf((char *)buffer + head, bufferLen - head, JSON_ITEM_STRING_END);
            if (res <= 0 || res >= bufferLen - head) return -1;
            res += head;
        } else
        {
            return -1;
        }
        break;

    default:
        break;
    }

    return res;
}

int lwm2m_json_serialize(int size,
                         lwm2m_data_t * tlvP,
                         uint8_t ** bufferP)
{
    int index;
    size_t head;
    uint8_t bufferJSON[PRV_JSON_BUFFER_SIZE];

    memcpy(bufferJSON, JSON_HEADER, JSON_HEADER_SIZE);
    head = JSON_HEADER_SIZE;

    // HACK START
    if (tlvP->type == LWM2M_TYPE_OBJECT_INSTANCE)
    {
        if (size == 1)
        {
            size = tlvP->length;
            tlvP = (lwm2m_data_t *)tlvP->value;
        }
        else return 0;
    }
    // HACK END
    for (index = 0 ; index < size && head < PRV_JSON_BUFFER_SIZE ; index++)
    {
        int res;

        // TODO: handle non-text format
        switch (tlvP[index].type)
        {
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
        {
            lwm2m_data_t * subTlvP;
            size_t i;

            subTlvP = (lwm2m_data_t *)tlvP[index].value;
            for (i = 0 ; i < tlvP[index].length ; i++)
            {
                res = lwm2m_snprintf((char *)bufferJSON + head, PRV_JSON_BUFFER_SIZE - head, JSON_INST_ITEM_TEMPLATE, tlvP[index].id, subTlvP[i].id);
                if (res <= 0 || res >= PRV_JSON_BUFFER_SIZE - head) return 0;
                head += res;

                res = prv_serializeValue(subTlvP + i, bufferJSON + head, PRV_JSON_BUFFER_SIZE -head);
                if (res < 0) return -1;
                head += res;
            }
        }
            break;

        case LWM2M_TYPE_RESOURCE:
            res = lwm2m_snprintf((char *)bufferJSON + head, PRV_JSON_BUFFER_SIZE - head, JSON_RES_ITEM_TEMPLATE, tlvP[index].id);
            if (res <= 0 || res >= PRV_JSON_BUFFER_SIZE - head) return 0;
            head += res;

            res = prv_serializeValue(tlvP + index, bufferJSON + head, PRV_JSON_BUFFER_SIZE -head);
            if (res < 0) return 0;
            head += res;
            break;

        case LWM2M_TYPE_OBJECT_INSTANCE:
            // TODO: support this. Hacked for now.
            break;

        case LWM2M_TYPE_RESOURCE_INSTANCE:
        default:
            return 0;
            break;
        }
    }

    if (head + JSON_FOOTER_SIZE - 1 > PRV_JSON_BUFFER_SIZE) return -1;

    memcpy(bufferJSON + head - 1, JSON_FOOTER, JSON_FOOTER_SIZE);
    head = head - 1 + JSON_FOOTER_SIZE;

    *bufferP = (uint8_t *)lwm2m_malloc(head);
    if (*bufferP == NULL) return 0;
    memcpy(*bufferP, bufferJSON, head);

    return (int)head;
}

#endif
