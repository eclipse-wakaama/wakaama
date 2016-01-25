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
 *    Fabien Fleutot - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    
 *******************************************************************************/

#include "internals.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <float.h>

#ifndef LWM2M_BIG_ENDIAN
#ifndef LWM2M_LITTLE_ENDIAN
#error Please define LWM2M_BIG_ENDIAN or LWM2M_LITTLE_ENDIAN
#endif
#endif

#define _PRV_64BIT_BUFFER_SIZE 8
#define _PRV_TLV_TYPE_MASK 0xC0

static int prv_getHeaderLength(uint16_t id,
                               size_t dataLen)
{
    int length;

    length = 2;

    if (id > 0xFF)
    {
        length += 1;
    }

    if (dataLen > 0xFFFF)
    {
        length += 3;
    }
    else if (dataLen > 0xFF)
    {
        length += 2;
    }
    else if (dataLen > 7)
    {
        length += 1;
    }

    return length;
}

static int prv_create_header(uint8_t * header,
                             lwm2m_tlv_type_t type,
                             uint16_t id,
                             size_t data_len)
{
    int header_len;
    int offset;

    header_len = prv_getHeaderLength(id, data_len);

    header[0] = 0;
    header[0] |= type&_PRV_TLV_TYPE_MASK;

    if (id > 0xFF)
    {
        header[0] |= 0x20;
        header[1] = (id >> 8) & 0XFF;
        header[2] = id & 0XFF;
        offset = 3;
    }
    else
    {
        header[1] = id;
        offset = 2;
    }
    if (data_len <= 7)
    {
        header[0] += data_len;
    }
    else if (data_len <= 0xFF)
    {
        header[0] |= 0x08;
        header[offset] = data_len;
    }
    else if (data_len <= 0xFFFF)
    {
        header[0] |= 0x10;
        header[offset] = (data_len >> 8) & 0XFF;
        header[offset + 1] = data_len & 0XFF;
    }
    else if (data_len <= 0xFFFFFF)
    {
        header[0] |= 0x18;
        header[offset] = (data_len >> 16) & 0XFF;
        header[offset + 1] = (data_len >> 8) & 0XFF;
        header[offset + 2] = data_len & 0XFF;
    }

    return header_len;
}

static void prv_copyValue(void * dst,
                          void * src,
                          size_t len)
{
#ifdef LWM2M_BIG_ENDIAN
    memcpy(dst, src, len);
#else
#ifdef LWM2M_LITTLE_ENDIAN
    int i;

    for (i = 0 ; i < len ; i++)
    {
        ((uint8_t *)dst)[i] = ((uint8_t *)src)[len - 1 - i];
    }
#endif
#endif
}

static void prv_encodeInt(int64_t data,
                          uint8_t data_buffer[_PRV_64BIT_BUFFER_SIZE],
                          size_t * lengthP)
{
    uint64_t value;
    int negative = 0;
    size_t length = 0;

    memset(data_buffer, 0, _PRV_64BIT_BUFFER_SIZE);

    if (data >= INT8_MIN && data <= INT8_MAX)
    {
        *lengthP = 1;
        data_buffer[0] = data;
    }
    else if (data >= INT16_MIN && data <= INT16_MAX)
    {
        int16_t value;

        value = data;
        *lengthP = 2;
        data_buffer[0] = (value >> 8) & 0xFF;
        data_buffer[1] = value & 0xFF;
    }
    else if (data >= INT32_MIN && data <= INT32_MAX)
    {
        int32_t value;

        value = data;
        *lengthP = 4;
        prv_copyValue(data_buffer, &value, *lengthP);
    }
    else if (data >= INT64_MIN && data <= INT64_MAX)
    {
        *lengthP = 8;
        prv_copyValue(data_buffer, &data, *lengthP);
    }
 }

int lwm2m_opaqueToTLV(lwm2m_tlv_type_t type,
                      uint8_t* dataP,
                      size_t data_len,
                      uint16_t id,
                      uint8_t * buffer,
                      size_t buffer_len)
{
    uint8_t header[LWM2M_TLV_HEADER_MAX_LENGTH];
    size_t header_len;

    header_len = prv_create_header(header, type, id, data_len);

    if (buffer_len < data_len + header_len) return 0;

    memmove(buffer, header, header_len);

    memmove(buffer + header_len, dataP, data_len);

    return header_len + data_len;
}

int lwm2m_boolToTLV(lwm2m_tlv_type_t type,
                    bool value,
                    uint16_t id,
                    uint8_t * buffer,
                    size_t buffer_len)
{
    return lwm2m_intToTLV(type, value?1:0, id, buffer, buffer_len);
}

int lwm2m_intToTLV(lwm2m_tlv_type_t type,
                   int64_t data,
                   uint16_t id,
                   uint8_t * buffer,
                   size_t buffer_len)
{
    uint8_t data_buffer[_PRV_64BIT_BUFFER_SIZE];
    size_t length = 0;

    if (type != LWM2M_TYPE_RESOURCE_INSTANCE && type != LWM2M_TYPE_RESOURCE)
        return 0;

    prv_encodeInt(data, data_buffer, &length);

    return lwm2m_opaqueToTLV(type, data_buffer + (_PRV_64BIT_BUFFER_SIZE - length), length, id, buffer, buffer_len);
}

int lwm2m_decodeTLV(uint8_t * buffer,
                    size_t buffer_len,
                    lwm2m_tlv_type_t * oType,
                    uint16_t * oID,
                    size_t * oDataIndex,
                    size_t * oDataLen)
{

    if (buffer_len < 2) return 0;

    *oDataIndex = 2;

    *oType = buffer[0]&_PRV_TLV_TYPE_MASK;

    if ((buffer[0]&0x20) == 0x20)
    {
        // id is 16 bits long
        if (buffer_len < 3) return 0;
        *oDataIndex += 1;
        *oID = (buffer[1]<<8) + buffer[2];
    }
    else
    {
        // id is 8 bits long
        *oID = buffer[1];
    }

    switch (buffer[0]&0x18)
    {
    case 0x00:
        // no length field
        *oDataLen = buffer[0]&0x07;
        break;
    case 0x08:
        // length field is 8 bits long
        if (buffer_len < *oDataIndex + 1) return 0;
        *oDataLen = buffer[*oDataIndex];
        *oDataIndex += 1;
        break;
    case 0x10:
        // length field is 16 bits long
        if (buffer_len < *oDataIndex + 2) return 0;
        *oDataLen = (buffer[*oDataIndex]<<8) + buffer[*oDataIndex+1];
        *oDataIndex += 2;
        break;
    case 0x18:
        // length field is 24 bits long
        if (buffer_len < *oDataIndex + 3) return 0;
        *oDataLen = (buffer[*oDataIndex]<<16) + (buffer[*oDataIndex+1]<<8) + buffer[*oDataIndex+2];
        *oDataIndex += 3;
        break;
    default:
        // can't happen
        return 0;
    }

    if (*oDataIndex + *oDataLen > buffer_len) return 0;

    return *oDataIndex + *oDataLen;
}

int lwm2m_opaqueToInt(uint8_t * buffer,
                      size_t buffer_len,
                      int64_t * dataP)
{
    *dataP = 0;

    switch (buffer_len)
    {
    case 1:
    {
        *dataP = (int8_t)buffer[0];

        break;
    }

    case 2:
    {
        int16_t value;

        prv_copyValue(&value, buffer, buffer_len);

        *dataP = value;
        break;
    }

    case 4:
    {
        int32_t value;

        prv_copyValue(&value, buffer, buffer_len);

        *dataP = value;
        break;
    }

    case 8:
        prv_copyValue(dataP, buffer, buffer_len);
        return buffer_len;

    default:
        return 0;
    }

    return buffer_len;
}

int lwm2m_opaqueToFloat(uint8_t * buffer,
                        size_t buffer_len,
                        double * dataP)
{
    switch (buffer_len)
    {
    case 4:
    {
        float temp;

        prv_copyValue(&temp, buffer, buffer_len);

        *dataP = temp;
    }
    return 4;

    case 8:
        prv_copyValue(dataP, buffer, buffer_len);
        return 8;

    default:
        return 0;
    }
}

lwm2m_data_t * lwm2m_data_new(int size)
{
    lwm2m_data_t * dataP;

    if (size <= 0) return NULL;

    dataP = (lwm2m_data_t *)lwm2m_malloc(size * sizeof(lwm2m_data_t));

    if (dataP != NULL)
    {
        memset(dataP, 0, size * sizeof(lwm2m_data_t));
    }

    return dataP;
}

static int prv_parseTLV(uint8_t * buffer,
                        size_t bufferLen,
                        lwm2m_data_t ** dataP)
{
    lwm2m_tlv_type_t type;
    uint16_t id;
    size_t dataIndex;
    size_t dataLen;
    int index = 0;
    int result;
    int size = 0;

    *dataP = NULL;

    while (0 != (result = lwm2m_decodeTLV((uint8_t*)buffer + index, bufferLen - index, &type, &id, &dataIndex, &dataLen)))
    {
        lwm2m_data_t * newTlvP;

        newTlvP = lwm2m_data_new(size + 1);
        if (size >= 1)
        {
            if (newTlvP == NULL)
            {
                lwm2m_data_free(size, *dataP);
                return 0;
            }
            else
            {
                memcpy(newTlvP, *dataP, size * sizeof(lwm2m_data_t));
                lwm2m_free(*dataP);
            }
        }
        *dataP = newTlvP;

        (*dataP)[size].type = type;
        (*dataP)[size].id = id;
        if (type == LWM2M_TYPE_OBJECT_INSTANCE || type == LWM2M_TYPE_MULTIPLE_RESOURCE)
        {
            (*dataP)[size].length = prv_parseTLV(buffer + index + dataIndex,
                                                 dataLen,
                                                 (lwm2m_data_t **)&((*dataP)[size].value));
            if ((*dataP)[size].length == 0)
            {
                lwm2m_data_free(size + 1, *dataP);
                return 0;
            }
        }
        else
        {
            (*dataP)[size].flags = LWM2M_TLV_FLAG_STATIC_DATA;
            (*dataP)[size].length = dataLen;
            (*dataP)[size].value = (uint8_t*)buffer + index + dataIndex;
        }
        size++;
        index += result;
    }

    return size;
}

int lwm2m_data_parse(uint8_t * buffer,
                     size_t bufferLen,
                     lwm2m_media_type_t format,
                     lwm2m_data_t ** dataP)
{
    switch (format)
    {
    case LWM2M_CONTENT_TEXT:
    case LWM2M_CONTENT_OPAQUE:
        *dataP = lwm2m_data_new(1);
        if (*dataP == NULL) return 0;
        (*dataP)->length = bufferLen;
        (*dataP)->value = buffer;
        (*dataP)->flags = LWM2M_TLV_FLAG_STATIC_DATA;
        return 1;

    case LWM2M_CONTENT_TLV:
        return prv_parseTLV(buffer, bufferLen, dataP);

#ifdef LWM2M_SUPPORT_JSON
    case LWM2M_CONTENT_JSON:
        return lwm2m_json_parse(buffer, bufferLen, dataP);
#endif

    default:
        return 0;
    }
}

static int prv_getLength(int size,
                         lwm2m_data_t * dataP)
{
    int length;
    int i;

    length = 0;

    for (i = 0 ; i < size && length != -1 ; i++)
    {
        switch (dataP[i].type)
        {
        case LWM2M_TYPE_OBJECT_INSTANCE:
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            {
                int subLength;

                subLength = prv_getLength(dataP[i].length, (lwm2m_data_t *)(dataP[i].value));
                if (subLength == -1)
                {
                    length = -1;
                }
                else
                {
                    length += prv_getHeaderLength(dataP[i].id, subLength) + subLength;
                }
            }
            break;
        case LWM2M_TYPE_RESOURCE_INSTANCE:
        case LWM2M_TYPE_RESOURCE:
            length += prv_getHeaderLength(dataP[i].id, dataP[i].length) + dataP[i].length;
            break;
        default:
            length = -1;
            break;
        }
    }

    return length;
}


static int prv_serializeTLV(int size,
                            lwm2m_data_t * dataP,
                            uint8_t ** bufferP)
{
    int length;
    int index;
    int i;

    *bufferP = NULL;
    length = prv_getLength(size, dataP);
    if (length <= 0) return length;

    *bufferP = (uint8_t *)lwm2m_malloc(length);
    if (*bufferP == NULL) return 0;

    index = 0;
    for (i = 0 ; i < size && length != 0 ; i++)
    {
        int headerLen;

        switch (dataP[i].type)
        {
        case LWM2M_TYPE_OBJECT_INSTANCE:
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            {
                uint8_t * tmpBuffer;
                int tmpLength;

                tmpLength = prv_serializeTLV(dataP[i].length, (lwm2m_data_t *)dataP[i].value, &tmpBuffer);
                if (tmpLength == 0)
                {
                    length = 0;
                }
                else
                {
                    headerLen = prv_create_header((uint8_t*)(*bufferP) + index, dataP[i].type, dataP[i].id, tmpLength);
                    index += headerLen;
                    memcpy(*bufferP + index, tmpBuffer, tmpLength);
                    index += tmpLength;
                    lwm2m_free(tmpBuffer);
                }
            }
            break;

        case LWM2M_TYPE_RESOURCE_INSTANCE:
        case LWM2M_TYPE_RESOURCE:
            {
                headerLen = prv_create_header((uint8_t*)(*bufferP) + index, dataP[i].type, dataP[i].id, dataP[i].length);
                if (headerLen == 0)
                {
                    length = 0;
                }
                else
                {
                    index += headerLen;
                    memcpy(*bufferP + index, dataP[i].value, dataP[i].length);
                    index += dataP[i].length;
                }
            }
            break;

        default:
            length = 0;
            break;
        }
    }

    if (length == 0)
    {
        lwm2m_free(*bufferP);
    }
    return length;
}

int lwm2m_data_serialize(int size,
                         lwm2m_data_t * dataP,
                         lwm2m_media_type_t * formatP,
                         uint8_t ** bufferP)
{

    // Check format
    if (*formatP == LWM2M_CONTENT_TEXT
     || *formatP == LWM2M_CONTENT_OPAQUE)
    {
        if ((size != 1)
         || (dataP->type != LWM2M_TYPE_RESOURCE))
        {
#ifdef LWM2M_SUPPORT_JSON
            *formatP = LWM2M_CONTENT_JSON;
#else
            *formatP = LWM2M_CONTENT_TLV;
#endif
        }
    }

    switch (*formatP)
    {
    case LWM2M_CONTENT_TEXT:
    case LWM2M_CONTENT_OPAQUE:
        *bufferP = (uint8_t *)lwm2m_malloc(dataP->length);
        if (*bufferP == NULL) return 0;
        memcpy(*bufferP, dataP->value, dataP->length);
        return dataP->length;

    case LWM2M_CONTENT_TLV:
        return prv_serializeTLV(size, dataP, bufferP);

#ifdef LWM2M_SUPPORT_JSON
    case LWM2M_CONTENT_JSON:
        return lwm2m_json_serialize(size, dataP, bufferP);
#endif

    default:
        return 0;
    }
}

void lwm2m_data_free(int size,
                     lwm2m_data_t * dataP)
{
    int i;

    if (size == 0 || dataP == NULL) return;

    for (i = 0 ; i < size ; i++)
    {
        if ((dataP[i].flags & LWM2M_TLV_FLAG_STATIC_DATA) == 0)
        {
            if (dataP[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE
             || dataP[i].type == LWM2M_TYPE_OBJECT_INSTANCE)
            {
                lwm2m_data_free(dataP[i].length, (lwm2m_data_t *)(dataP[i].value));
            }
            else
            {
                lwm2m_free(dataP[i].value);
            }
        }
    }
    lwm2m_free(dataP);
}

void lwm2m_data_encode_int(int64_t value,
                           lwm2m_data_t * dataP)
{
    dataP->length = 0;
    dataP->dataType = LWM2M_TYPE_INTEGER;

    if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
    {
        dataP->flags &= ~LWM2M_TLV_FLAG_STATIC_DATA;
        dataP->length = lwm2m_int64ToPlainText(value, &dataP->value);
    }
    else
    {
        uint8_t buffer[_PRV_64BIT_BUFFER_SIZE];
        size_t length = 0;

        prv_encodeInt(value, buffer, &length);

        dataP->value = (uint8_t *)lwm2m_malloc(length);
        if (dataP->value != NULL)
        {
            memcpy(dataP->value,
                   buffer,
                   length);
            dataP->flags &= ~LWM2M_TLV_FLAG_STATIC_DATA;
            dataP->length = length;
        }
    }
}

int lwm2m_data_decode_int(lwm2m_data_t * dataP,
                          int64_t * valueP)
{
    int result;

    if (dataP->length == 0) return 0;

    if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
    {
        result = lwm2m_PlainTextToInt64(dataP->value, dataP->length, valueP);
    }
    else
    {
        result = lwm2m_opaqueToInt(dataP->value, dataP->length, valueP);
        if (result == dataP->length)
        {
            result = 1;
        }
        else
        {
            result = 0;
        }
    }

    return result;
}

void lwm2m_data_encode_float(double value,
                             lwm2m_data_t * dataP)
{
    dataP->length = 0;
    dataP->dataType = LWM2M_TYPE_FLOAT;

    if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
    {
        dataP->flags &= ~LWM2M_TLV_FLAG_STATIC_DATA;
        dataP->length = lwm2m_float64ToPlainText(value, &dataP->value);
    }
    else
    {
        size_t length = 0;

        if (value > FLT_MAX || value < (0 - FLT_MAX))
        {
            length = 8;
        }
        else
        {
            length = 4;
        }

        dataP->value = (uint8_t *)lwm2m_malloc(length);
        if (dataP->value != NULL)
        {
            if (length == 4)
            {
                float temp;

                temp = value;

                prv_copyValue(dataP->value, &temp, length);
            }
            else
            {
                prv_copyValue(dataP->value, &value, length);
            }

            dataP->flags &= ~LWM2M_TLV_FLAG_STATIC_DATA;
            dataP->length = length;
        }
    }
}

int lwm2m_data_decode_float(lwm2m_data_t * dataP,
                            double * valueP)
{
    int result;

    if (dataP->length == 0) return 0;

    if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
    {
        result = lwm2m_PlainTextToFloat64(dataP->value, dataP->length, valueP);
    }
    else
    {
        result = lwm2m_opaqueToFloat(dataP->value, dataP->length, valueP);
        if (result == dataP->length)
        {
            result = 1;
        }
        else
        {
            result = 0;
        }
    }

    return result;
}

void lwm2m_data_encode_bool(bool value,
                            lwm2m_data_t * dataP)
{
    dataP->length = 0;
    dataP->dataType = LWM2M_TYPE_BOOLEAN;

    dataP->value = (uint8_t *)lwm2m_malloc(1);
    if (dataP->value != NULL)
    {
        if (value == true)
        {
            if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
            {
                dataP->value[0] = '1';
            }
            else
            {
                dataP->value[0] = 1;
            }
        }
        else
        {
            if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
            {
                dataP->value[0] = '0';
            }
            else
            {
                dataP->value[0] = 0;
            }
        }
        dataP->flags &= ~LWM2M_TLV_FLAG_STATIC_DATA;
        dataP->length = 1;
    }
}

int lwm2m_data_decode_bool(lwm2m_data_t * dataP,
                           bool * valueP)
{
    if (dataP->length != 1) return 0;

    if ((dataP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
    {
        switch (dataP->value[0])
        {
        case '0':
            *valueP = false;
            break;
        case '1':
            *valueP = true;
            break;
        default:
            return 0;
        }
    }
    else
    {
        switch (dataP->value[0])
        {
        case 0:
            *valueP = false;
            break;
        case 1:
            *valueP = true;
            break;
        default:
            return 0;
        }
    }

    return 1;
}

void lwm2m_data_include(lwm2m_data_t * subDataP,
                        size_t count,
                        lwm2m_data_t * dataP)
{
    if (subDataP == NULL || count == 0) return;

    switch(subDataP[0].type)
    {
    case LWM2M_TYPE_RESOURCE:
    case LWM2M_TYPE_MULTIPLE_RESOURCE:
        dataP->type = LWM2M_TYPE_OBJECT_INSTANCE;
        break;
    case LWM2M_TYPE_RESOURCE_INSTANCE:
        dataP->type = LWM2M_TYPE_MULTIPLE_RESOURCE;
        break;
    default:
        break;
    }
    dataP->flags = 0;
    dataP->dataType = LWM2M_TYPE_UNDEFINED;
    dataP->length = count;
    dataP->value = (uint8_t *)subDataP;
}
