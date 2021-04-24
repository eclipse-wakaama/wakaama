/*******************************************************************************
 *
 * Copyright (c) 2015 Intel Corporation and others.
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
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *
 *******************************************************************************/

#include "internals.h"
#include <float.h>

#if defined(LWM2M_SUPPORT_JSON) || defined(LWM2M_SUPPORT_SENML_JSON)

#define _GO_TO_NEXT_CHAR(I,B,L)         \
    {                                   \
        I++;                            \
        I += json_skipSpace(B+I, L-I);   \
        if (I == L) goto error;         \
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

size_t json_skipSpace(const uint8_t * buffer,size_t bufferLen)
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

int json_split(const uint8_t * buffer,
               size_t bufferLen,
               size_t * tokenStartP,
               size_t * tokenLenP,
               size_t * valueStartP,
               size_t * valueLenP)
{
    size_t index;
    _itemState step;

    *tokenStartP = 0;
    *tokenLenP = 0;
    *valueStartP = 0;
    *valueLenP = 0;
    index = 0;
    step = _STEP_START;

    index = json_skipSpace(buffer + index, bufferLen - index);
    if (index == bufferLen) return -1;

    while (index < bufferLen)
    {
        switch (step)
        {
        case _STEP_START:
            if (buffer[index] == ',') goto loop_exit;
            if (buffer[index] != '"') return -1;
            *tokenStartP = index+1;
            step = _STEP_TOKEN;
            break;

        case _STEP_TOKEN:
            if (buffer[index] == ',') goto loop_exit;
            if (buffer[index] == '"')
            {
                *tokenLenP = index - *tokenStartP;
                step = _STEP_ANY_SEPARATOR;
            }
            break;

        case _STEP_ANY_SEPARATOR:
            if (buffer[index] == ',') goto loop_exit;
            if (buffer[index] != ':') return -1;
            step = _STEP_SEPARATOR;
            break;

        case _STEP_SEPARATOR:
            if (buffer[index] == ',') goto loop_exit;
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
            if (buffer[index] == ',') goto loop_exit;
            if (prv_isWhiteSpace(buffer[index]))
            {
                *valueLenP = index - *valueStartP;
                step = _STEP_DONE;
            }
            break;

        case _STEP_DONE:
            if (buffer[index] == ',') goto loop_exit;
            return -1;

        default:
            return -1;
        }

        index++;
        if (step == _STEP_START
         || step == _STEP_ANY_SEPARATOR
         || step == _STEP_SEPARATOR
         || step == _STEP_DONE)
        {
            index += json_skipSpace(buffer + index, bufferLen - index);
        }
    }
loop_exit:

    if (step == _STEP_VALUE)
    {
        *valueLenP = index - *valueStartP;
        step = _STEP_DONE;
    }

    if (step != _STEP_DONE) return -1;

    return (int)index;
}

int json_itemLength(const uint8_t * buffer, size_t bufferLen)
{
    size_t index;
    int in;

    index = 0;
    in = 0;

    while (index < bufferLen)
    {
        switch (in)
        {
        case 0:
            if (buffer[index] != '{') goto error;
            in = 1;
            _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
            break;
        case 1:
            if (buffer[index] == '{') goto error;
            if (buffer[index] == '}')
            {
                return index + 1;
            }
            else if (buffer[index] == '"')
            {
                in = 2;
                _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
            }
            else
            {
                _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
            }
            break;
        case 2:
            if (buffer[index] == '"')
            {
                in = 1;
            }
            else if (buffer[index] == '\\')
            {
                /* Escape in string. Skip the next character. */
                index++;
                if (index == bufferLen) goto error;
            }
            _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
            break;
        default:
            goto error;
        }
    }

error:
    return -1;
}

int json_countItems(const uint8_t * buffer, size_t bufferLen)
{
    int count;
    size_t index;
    int in;

    count = 0;
    index = 0;
    in = 0;

    while (index < bufferLen)
    {
        int len = json_itemLength(buffer + index,  bufferLen - index);
        if (len <= 0) return -1;
        count++;
        index += len - 1;
        _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
        if (buffer[index] == ']')
        {
            /* Done processing */
            index = bufferLen;
        }
        else if (buffer[index] == ',')
        {
            _GO_TO_NEXT_CHAR(index, buffer, bufferLen);
        }
        else return -1;
    }
    if (in > 0) goto error;

    return count;

error:
    return -1;
}


int json_convertNumeric(const uint8_t *value,
                        size_t valueLen,
                        lwm2m_data_t *targetP)
{
    int result = 0;
    size_t i = 0;
    while (i < valueLen && value[i] != '.' && value[i] != 'e' && value[i] != 'E')
    {
        i++;
    }
    if (i == valueLen)
    {
        if (value[0] == '-')
        {
            int64_t val;

            result = utils_textToInt(value, valueLen, &val);
            if (result)
            {
                lwm2m_data_encode_int(val, targetP);
            }
        }
        else
        {
            uint64_t val;

            result = utils_textToUInt(value, valueLen, &val);
            if (result)
            {
                lwm2m_data_encode_uint(val, targetP);
            }
        }
    }
    else
    {
        double val;

        result = utils_textToFloat(value, valueLen, &val, true);
        if (result)
        {
            lwm2m_data_encode_float(val, targetP);
        }
    }

    return result;
}

int json_convertTime(const uint8_t *valueStart, size_t valueLen, time_t *t)
{
    int64_t value;
    int res = utils_textToInt(valueStart, valueLen, &value);
    if (res)
    {
        *t = (time_t)value;
    }
    return res;
}

static uint8_t prv_hexValue(uint8_t digit)
{
    if (digit >= '0' && digit <= '9') return digit - '0';
    if (digit >= 'a' && digit <= 'f') return digit - 'a' + 10;
    if (digit >= 'A' && digit <= 'F') return digit - 'A' + 10;
    return 0xFF;
}

size_t json_unescapeString(uint8_t *dst, const uint8_t *src, size_t len)
{
    size_t i;
    size_t result = 0;
    for (i = 0; i < len; i++)
    {
        uint8_t c = src[i];
        if (c == '\\')
        {
            i++;
            if (i >= len) return false;
            c = src[i];
            switch (c)
            {
            case '"':
            case '\\':
            case '/':
                dst[result++] = (char)c;
                break;
            case 'b':
                dst[result++] = '\b';
                break;
            case 'f':
                dst[result++] = '\f';
                break;
            case 'n':
                dst[result++] = '\n';
                break;
            case 'r':
                dst[result++] = '\r';
                break;
            case 't':
                dst[result++] = '\t';
                break;
            case 'u':
            {
                uint8_t v1, v2;
                i++;
                if (i >= len - 4) return 0;
                if (src[i++] != '0') return 0;
                if (src[i++] != '0') return 0;
                v1 = prv_hexValue(src[i++]);
                if (v1 > 15) return 0;
                v2 = prv_hexValue(src[i]);
                if (v2 > 15) return 0;
                dst[result++] = (char)((v1 << 4) + v2);
                break;
            }
            default:
                /* invalid escape sequence */
                return 0;
            }
        }
        else
        {
            dst[result++] = (char)c;
        }
    }
    return result;
}


static uint8_t prv_hexDigit(uint8_t value)
{
    value = value & 0xF;
    if (value < 10) return '0' + value;
    return 'A' + value - 10;
}

size_t json_escapeString(uint8_t *dst, size_t dstLen, const uint8_t *src, size_t srcLen)
{
    size_t i;
    size_t head = 0;
    for (i = 0; i < srcLen; i++)
    {
        if (src[i] < 0x20)
        {
            if (src[i] == '\b')
            {
                if (dstLen - head < 2) return 0;
                dst[head++] = '\\';
                dst[head++] = 'b';
            }
            else if(src[i] == '\f')
            {
                if (dstLen - head < 2) return 0;
                dst[head++] = '\\';
                dst[head++] = 'f';
            }
            else if(src[i] == '\n')
            {
                if (dstLen - head < 2) return 0;
                dst[head++] = '\\';
                dst[head++] = 'n';
            }
            else if(src[i] == '\r')
            {
                if (dstLen - head < 2) return 0;
                dst[head++] = '\\';
                dst[head++] = 'r';
            }
            else if(src[i] == '\t')
            {
                if (dstLen - head < 2) return 0;
                dst[head++] = '\\';
                dst[head++] = 't';
            }
            else
            {
                if (dstLen - head < 6) return 0;
                dst[head++] = '\\';
                dst[head++] = 'u';
                dst[head++] = '0';
                dst[head++] = '0';
                dst[head++] = prv_hexDigit(src[i] >> 4);
                dst[head++] = prv_hexDigit(src[i]);
            }
        }
        else if (src[i] == '"')
        {
            if (dstLen - head < 2) return 0;
            dst[head++] = '\\';
            dst[head++] = '\"';
        }
        else if (src[i] == '\\')
        {
            if (dstLen - head < 2) return 0;
            dst[head++] = '\\';
            dst[head++] = '\\';
        }
        else
        {
            if (dstLen - head < 1) return 0;
            dst[head++] = src[i];
        }
    }

    return head;
}

lwm2m_data_t * json_extendData(lwm2m_data_t * parentP)
{
    lwm2m_data_t * newP;

    newP = lwm2m_data_new(parentP->value.asChildren.count + 1);
    if (newP == NULL) return NULL;
    if (parentP->value.asChildren.count)
    {
        memcpy(newP,
               parentP->value.asChildren.array,
               parentP->value.asChildren.count * sizeof(lwm2m_data_t));
        lwm2m_free(parentP->value.asChildren.array);     /* do not use lwm2m_data_free() to keep pointed values */
    }
    parentP->value.asChildren.array = newP;
    parentP->value.asChildren.count += 1;

    return newP + parentP->value.asChildren.count - 1;
}

int json_dataStrip(int size, lwm2m_data_t * dataP, lwm2m_data_t ** resultP)
{
    int i;
    int j;

    *resultP = lwm2m_data_new(size);
    if (*resultP == NULL) return -1;

    j = 0;
    for (i = 0 ; i < size ; i++)
    {
        memcpy((*resultP) + j, dataP + i, sizeof(lwm2m_data_t));

        switch (dataP[i].type)
        {
        case LWM2M_TYPE_OBJECT:
        case LWM2M_TYPE_OBJECT_INSTANCE:
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
        {
            int childLen;

            childLen = json_dataStrip(dataP[i].value.asChildren.count,
                                      dataP[i].value.asChildren.array,
                                      &((*resultP)[j].value.asChildren.array));
            if (childLen <= 0)
            {
                /* skip this one */
                j--;
            }
            else
            {
                (*resultP)[j].value.asChildren.count = childLen;
            }
            break;
        }
        case LWM2M_TYPE_STRING:
        case LWM2M_TYPE_OPAQUE:
        case LWM2M_TYPE_CORE_LINK:
            dataP[i].value.asBuffer.length = 0;
            dataP[i].value.asBuffer.buffer = NULL;
            break;
        default:
            /* do nothing */
            break;
        }

        j++;
    }

    return size;
}

lwm2m_data_t * json_findDataItem(lwm2m_data_t * listP, size_t count, uint16_t id)
{
    size_t i;

    // TODO: handle times

    i = 0;
    while (i < count)
    {
        if (listP[i].type != LWM2M_TYPE_UNDEFINED && listP[i].id == id)
        {
            return listP + i;
        }
        i++;
    }

    return NULL;
}

uri_depth_t json_decreaseLevel(uri_depth_t level)
{
    switch(level)
    {
    case URI_DEPTH_NONE:
        return URI_DEPTH_OBJECT;
    case URI_DEPTH_OBJECT:
        return URI_DEPTH_OBJECT_INSTANCE;
    case URI_DEPTH_OBJECT_INSTANCE:
        return URI_DEPTH_RESOURCE;
    case URI_DEPTH_RESOURCE:
        return URI_DEPTH_RESOURCE_INSTANCE;
    case URI_DEPTH_RESOURCE_INSTANCE:
        return URI_DEPTH_RESOURCE_INSTANCE;
    default:
        return URI_DEPTH_RESOURCE;
    }
}

static int prv_findAndCheckData(const lwm2m_uri_t * uriP,
                                uri_depth_t desiredLevel,
                                size_t size,
                                const lwm2m_data_t * tlvP,
                                lwm2m_data_t ** targetP,
                                uri_depth_t *targetLevelP)
{
    size_t index;
    int result;

    if (size == 0) return 0;

    if (size > 1)
    {
        if (tlvP[0].type == LWM2M_TYPE_OBJECT
         || tlvP[0].type == LWM2M_TYPE_OBJECT_INSTANCE)
        {
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].type != tlvP[0].type)
                {
                    *targetP = NULL;
                    return -1;
                }
            }
        }
        else
        {
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].type == LWM2M_TYPE_OBJECT
                 || tlvP[index].type == LWM2M_TYPE_OBJECT_INSTANCE)
                {
                    *targetP = NULL;
                    return -1;
                }
            }
        }
    }

    *targetP = NULL;
    result = -1;
    switch (desiredLevel)
    {
    case URI_DEPTH_OBJECT:
        if (tlvP[0].type == LWM2M_TYPE_OBJECT)
        {
            *targetP = (lwm2m_data_t*)tlvP;
            *targetLevelP = URI_DEPTH_OBJECT;
            result = (int)size;
        }
        break;

    case URI_DEPTH_OBJECT_INSTANCE:
        switch (tlvP[0].type)
        {
        case LWM2M_TYPE_OBJECT:
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].id == uriP->objectId)
                {
                    *targetLevelP = URI_DEPTH_OBJECT_INSTANCE;
                    return prv_findAndCheckData(uriP,
                                                desiredLevel,
                                                tlvP[index].value.asChildren.count,
                                                tlvP[index].value.asChildren.array,
                                                targetP,
                                                targetLevelP);
                }
            }
            break;
        case LWM2M_TYPE_OBJECT_INSTANCE:
            *targetP = (lwm2m_data_t*)tlvP;
            result = (int)size;
            break;
        default:
            break;
        }
        break;

    case URI_DEPTH_RESOURCE:
        switch (tlvP[0].type)
        {
        case LWM2M_TYPE_OBJECT:
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].id == uriP->objectId)
                {
                    *targetLevelP = URI_DEPTH_OBJECT_INSTANCE;
                    return prv_findAndCheckData(uriP,
                                                desiredLevel,
                                                tlvP[index].value.asChildren.count,
                                                tlvP[index].value.asChildren.array,
                                                targetP,
                                                targetLevelP);
                }
            }
            break;
        case LWM2M_TYPE_OBJECT_INSTANCE:
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].id == uriP->instanceId)
                {
                    *targetLevelP = URI_DEPTH_RESOURCE;
                    return prv_findAndCheckData(uriP,
                                                desiredLevel,
                                                tlvP[index].value.asChildren.count,
                                                tlvP[index].value.asChildren.array,
                                                targetP, targetLevelP);
                }
            }
            break;
        default:
            *targetP = (lwm2m_data_t*)tlvP;
            result = (int)size;
            break;
        }
        break;

    case URI_DEPTH_RESOURCE_INSTANCE:
        switch (tlvP[0].type)
        {
        case LWM2M_TYPE_OBJECT:
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].id == uriP->objectId)
                {
                    *targetLevelP = URI_DEPTH_OBJECT_INSTANCE;
                    return prv_findAndCheckData(uriP,
                                                desiredLevel,
                                                tlvP[index].value.asChildren.count,
                                                tlvP[index].value.asChildren.array,
                                                targetP,
                                                targetLevelP);
                }
            }
            break;
        case LWM2M_TYPE_OBJECT_INSTANCE:
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].id == uriP->instanceId)
                {
                    *targetLevelP = URI_DEPTH_RESOURCE;
                    return prv_findAndCheckData(uriP,
                                                desiredLevel,
                                                tlvP[index].value.asChildren.count,
                                                tlvP[index].value.asChildren.array,
                                                targetP,
                                                targetLevelP);
                }
            }
            break;
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            for (index = 0; index < size; index++)
            {
                if (tlvP[index].id == uriP->resourceId)
                {
                    *targetLevelP = URI_DEPTH_RESOURCE_INSTANCE;
                    return prv_findAndCheckData(uriP,
                                                desiredLevel,
                                                tlvP[index].value.asChildren.count,
                                                tlvP[index].value.asChildren.array,
                                                targetP,
                                                targetLevelP);
                }
            }
            break;
        default:
            *targetP = (lwm2m_data_t*)tlvP;
            result = (int)size;
            break;
        }
        break;

    default:
        break;
    }

    return result;
}

int json_findAndCheckData(const lwm2m_uri_t * uriP,
                          uri_depth_t baseLevel,
                          size_t size,
                          const lwm2m_data_t * tlvP,
                          lwm2m_data_t ** targetP,
                          uri_depth_t *targetLevelP)
{
    uri_depth_t desiredLevel = json_decreaseLevel(baseLevel);
    if (baseLevel < URI_DEPTH_RESOURCE)
    {
        *targetLevelP = desiredLevel;
    }
    else
    {
        *targetLevelP = baseLevel;
    }
    return prv_findAndCheckData(uriP,
                                desiredLevel,
                                size,
                                tlvP,
                                targetP,
                                targetLevelP);
}

#endif
