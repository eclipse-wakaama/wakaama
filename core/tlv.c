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
 *    
 *******************************************************************************/

#include "internals.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#define _PRV_64BIT_BUFFER_SIZE 8

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

    header_len = prv_getHeaderLength(id, data_len);

    header[0] = 0;
    switch (type)
    {
    case TLV_OBJECT_INSTANCE:
        // do nothing
        break;
    case TLV_RESSOURCE_INSTANCE:
        header[0] |= 0x40;
        break;
    case TLV_MULTIPLE_INSTANCE:
        header[0] |= 0x80;
        break;
    case TLV_RESSOURCE:
        header[0] |= 0xC0;
        break;
    default:
        return 0;
    }
    if (id > 0xFF)
    {
        header[0] |= 0x20;
        header[1] = (id >> 8) & 0XFF;
        header[2] = id & 0XFF;
    }
    else
    {
        header[1] = id;
    }
    if (data_len <= 7)
    {
        header[0] += data_len;
    }
    else if (data_len <= 0xFF)
    {
        header[0] |= 0x08;
        header[2] = data_len;
    }
    else if (data_len <= 0xFFFF)
    {
        header[0] |= 0x10;
        header[2] = (data_len >> 8) & 0XFF;
        header[3] = data_len & 0XFF;
    }
    else if (data_len <= 0xFFFFFF)
    {
        header[0] |= 0x18;
        header[2] = (data_len >> 16) & 0XFF;
        header[3] = (data_len >> 8) & 0XFF;
        header[4] = data_len & 0XFF;
    }

    return header_len;
}

int lwm2m_opaqueToTLV(lwm2m_tlv_type_t type,
                      uint8_t* dataP,
                      size_t data_len,
                      uint16_t id,
                      char * buffer,
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
                    char * buffer,
                    size_t buffer_len)
{
    return lwm2m_intToTLV(type, value?1:0, id, buffer, buffer_len);
}

int lwm2m_intToTLV(lwm2m_tlv_type_t type,
                   int64_t data,
                   uint16_t id,
                   char * buffer,
                   size_t buffer_len)
{
    uint8_t data_buffer[_PRV_64BIT_BUFFER_SIZE];
    size_t length = 0;
    uint64_t value;
    int negative = 0;

    if (type != TLV_RESSOURCE_INSTANCE && type != TLV_RESSOURCE)
        return 0;

    memset(data_buffer, 0, 8);

    if (data < 0)
    {
        negative = 1;
        value = 0 - data;
    }
    else
    {
        value = data;
    }

    do
    {
        length++;
        data_buffer[_PRV_64BIT_BUFFER_SIZE - length] = (value >> (8*(length-1))) & 0xFF;
    } while (value > (((uint64_t)1 << ((8 * length)-1)) - 1));


    if (1 == negative)
    {
        data_buffer[_PRV_64BIT_BUFFER_SIZE - length] |= 0x80;
    }

    return lwm2m_opaqueToTLV(type, data_buffer + (_PRV_64BIT_BUFFER_SIZE - length), length, id, buffer, buffer_len);
}

int lwm2m_decodeTLV(char * buffer,
                    size_t buffer_len,
                    lwm2m_tlv_type_t * oType,
                    uint16_t * oID,
                    size_t * oDataIndex,
                    size_t * oDataLen)
{

    if (buffer_len < 2) return 0;

    *oDataIndex = 2;

    switch (buffer[0]&0xC0)
    {
    case 0x00:
        *oType = TLV_OBJECT_INSTANCE;
        break;
    case 0x40:
        *oType = TLV_RESSOURCE_INSTANCE;
        break;
    case 0x80:
        *oType = TLV_MULTIPLE_INSTANCE;
        break;
    case 0xC0:
        *oType = TLV_RESSOURCE;
        break;
    default:
        // can't happen
        return 0;
    }

    if ((uint8_t)(buffer[0]&0x20) == 0x20)
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

int lwm2m_opaqueToInt(char * buffer,
                      size_t buffer_len,
                      int64_t * dataP)
{
    int i;

    if (buffer_len == 0 || buffer_len > 8) return 0;

    // first bit is the sign
    *dataP = buffer[0]&0x7F;

    for (i = 1 ; i < buffer_len ; i++)
    {
        *dataP = (*dataP << 8) + (buffer[i]&0xFF);
    }

    // first bit is the sign
    if ((uint8_t)(buffer[0]&0x80) == 0x80)
    {
        *dataP = 0 - *dataP;
    }

    return i;
}

lwm2m_tlv_t * lwm2m_tlv_new(int size)
{
    lwm2m_tlv_t * tlvP;

    if (size <= 0) return NULL;

    tlvP = (lwm2m_tlv_t *)lwm2m_malloc(size * sizeof(lwm2m_tlv_t));

    if (tlvP != NULL)
    {
        memset(tlvP, 0, size * sizeof(lwm2m_tlv_t));
    }

    return tlvP;
}

int lwm2m_tlv_parse(char * buffer,
                    size_t bufferLen,
                    lwm2m_tlv_t ** dataP)
{
    lwm2m_tlv_type_t type;
    uint16_t id;
    size_t dataIndex;
    size_t dataLen;
    int length = 0;
    int result;
    int size = 0;

    *dataP = NULL;

    while (0 != (result = lwm2m_decodeTLV(buffer + length, bufferLen - length, &type, &id, &dataIndex, &dataLen)))
    {
        lwm2m_tlv_t * newTlvP;

        newTlvP = lwm2m_tlv_new(size + 1);
        if (size >= 1)
        {
            if (newTlvP == NULL)
            {
                lwm2m_tlv_free(size, *dataP);
                return 0;
            }
            else
            {
                memcpy(newTlvP, *dataP, size * sizeof(lwm2m_tlv_t));
                lwm2m_free(*dataP);
            }
        }
        *dataP = newTlvP;

        switch (type)
        {
        case TLV_OBJECT_INSTANCE:
            (*dataP)[size].type = LWM2M_TYPE_OBJECT_INSTANCE;
            break;
        case TLV_RESSOURCE_INSTANCE:
            (*dataP)[size].type = LWM2M_TYPE_RESSOURCE_INSTANCE;
            break;
        case TLV_MULTIPLE_INSTANCE:
            (*dataP)[size].type = LWM2M_TYPE_MULTIPLE_RESSOURCE;
            break;
        case TLV_RESSOURCE:
            (*dataP)[size].type = LWM2M_TYPE_RESSOURCE;
            break;
        default:
            lwm2m_tlv_free(size, *dataP);
            return 0;
        }

        (*dataP)[size].id = id;
        if (type == TLV_OBJECT_INSTANCE || type == TLV_MULTIPLE_INSTANCE)
        {
            (*dataP)[size].length = lwm2m_tlv_parse(buffer + length + dataIndex,
                                                    dataLen,
                                                    (lwm2m_tlv_t **)&((*dataP)[size].value));
            if ((*dataP)[size].length == 0)
            {
                lwm2m_tlv_free(size + 1, *dataP);
                return 0;
            }
        }
        else
        {
            (*dataP)[size].flags = LWM2M_TLV_FLAG_STATIC_DATA;
            (*dataP)[size].length = dataLen;
            (*dataP)[size].value = buffer + length + dataIndex;
        }
        size++;
        length += result;
    }

    return size;
}

static int prv_getLength(int size,
                         lwm2m_tlv_t * tlvP)
{
    int length;
    int i;

    length = 0;

    for (i = 0 ; i < size && length != -1 ; i++)
    {
        switch (tlvP[i].type)
        {
        case LWM2M_TYPE_OBJECT_INSTANCE:
        case LWM2M_TYPE_MULTIPLE_RESSOURCE:
            {
                int subLength;

                subLength = prv_getLength(tlvP[i].length, (lwm2m_tlv_t *)(tlvP[i].value));
                if (subLength == -1)
                {
                    length = -1;
                }
                else
                {
                    length += prv_getHeaderLength(tlvP[i].id, subLength) + subLength;
                }
            }
            break;
        case LWM2M_TYPE_RESSOURCE_INSTANCE:
        case LWM2M_TYPE_RESSOURCE:
            length += prv_getHeaderLength(tlvP[i].id, tlvP[i].length) + tlvP[i].length;
            break;
        default:
            length = -1;
            break;
        }
    }

    return length;
}


int lwm2m_tlv_serialize(int size,
                        lwm2m_tlv_t * tlvP,
                        char ** bufferP)
{
    int length;
    int index;
    int i;

    *bufferP = NULL;
    length = prv_getLength(size, tlvP);
    if (length <= 0) return length;

    *bufferP = (char *)lwm2m_malloc(length);
    if (*bufferP == NULL) return 0;

    index = 0;
    for (i = 0 ; i < size && length != 0 ; i++)
    {
        int headerLen;

        switch (tlvP[i].type)
        {
        case LWM2M_TYPE_OBJECT_INSTANCE:
        case LWM2M_TYPE_MULTIPLE_RESSOURCE:
            {
                char * tmpBuffer;
                int tmpLength;

                tmpLength = lwm2m_tlv_serialize(tlvP[i].length, (lwm2m_tlv_t *)tlvP[i].value, &tmpBuffer);
                if (tmpLength == 0)
                {
                    length = 0;
                }
                else
                {
                    headerLen = prv_create_header(*bufferP + index, tlvP[i].type, tlvP[i].id, tmpLength);
                    index += headerLen;
                    memcpy(*bufferP + index, tmpBuffer, tmpLength);
                    index += tmpLength;
                    lwm2m_free(tmpBuffer);
                }
            }
            break;

        case LWM2M_TYPE_RESSOURCE_INSTANCE:
        case LWM2M_TYPE_RESSOURCE:
            {
                headerLen = prv_create_header(*bufferP + index, tlvP[i].type, tlvP[i].id, tlvP[i].length);
                if (headerLen == 0)
                {
                    length = 0;
                }
                else
                {
                    index += headerLen;
                    memcpy(*bufferP + index, tlvP[i].value, tlvP[i].length);
                    index += tlvP[i].length;
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

void lwm2m_tlv_free(int size,
                    lwm2m_tlv_t * tlvP)
{
    int i;

    if (size == 0 || tlvP == NULL) return;

    for (i = 0 ; i < size ; i++)
    {
        if ((tlvP[i].flags & LWM2M_TLV_FLAG_STATIC_DATA) == 0)
        {
            if (tlvP[i].type == LWM2M_TYPE_MULTIPLE_RESSOURCE
             || tlvP[i].type == TLV_OBJECT_INSTANCE)
            {
                lwm2m_tlv_free(tlvP[i].length, (lwm2m_tlv_t *)(tlvP[i].value));
            }
            else
            {
                lwm2m_free(tlvP[i].value);
            }
        }
    }
    lwm2m_free(tlvP);
}

void lwm2m_tlv_encode_int(int64_t data,
                          lwm2m_tlv_t * tlvP)
{
    tlvP->length = 0;

    if ((tlvP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
    {
        char string[32];
        int length;

        length = snprintf(string, 32, "%" PRId64, data);
        if (length > 0)
        {
            tlvP->value = (uint8_t *)malloc(length);
            if (tlvP->value != NULL)
            {
                strncpy(tlvP->value, string, length);
                tlvP->flags &= ~LWM2M_TLV_FLAG_STATIC_DATA;
                tlvP->length = length;
            }
        }
    }
    else
    {
        uint8_t buffer[_PRV_64BIT_BUFFER_SIZE];
        size_t length = 0;
        uint64_t value;
        int negative = 0;

        memset(buffer, 0, _PRV_64BIT_BUFFER_SIZE);

        if (data < 0)
        {
            negative = 1;
            value = 0 - data;
        }
        else
        {
            value = data;
        }

        do
        {
            length++;
            buffer[_PRV_64BIT_BUFFER_SIZE - length] = (value >> (8*(length-1))) & 0xFF;
        } while (value > (((uint64_t)1 << ((8 * length)-1)) - 1));


        if (1 == negative)
        {
            buffer[_PRV_64BIT_BUFFER_SIZE - length] |= 0x80;
        }

        tlvP->value = (uint8_t *)malloc(length);
        if (tlvP->value != NULL)
        {
            memcpy(tlvP->value,
                   buffer + (_PRV_64BIT_BUFFER_SIZE - length),
                   length);
            tlvP->flags &= ~LWM2M_TLV_FLAG_STATIC_DATA;
            tlvP->length = length;
        }
    }
}

int lwm2m_tlv_decode_int(lwm2m_tlv_t * tlvP,
                         int64_t * dataP)
{
    int i;

    if (tlvP->length == 0) return 0;

    if ((tlvP->flags & LWM2M_TLV_FLAG_TEXT_FORMAT) != 0)
    {
        char string[32];
        int result;

        // int64 is 20 digit max
        if (tlvP->length > 32) return 0;

        memcpy(string, tlvP->value, tlvP->length);
        string[tlvP->length] = 0;
        result = sscanf(string, "%" PRId64, dataP);
        if (result != 1) return 0;
    }
    else
    {
        if (tlvP->length > 8) return 0;

        // first bit is the sign
        *dataP = tlvP->value[0]&0x7F;

        for (i = 1 ; i < tlvP->length ; i++)
        {
            *dataP = (*dataP << 8) + tlvP->value[i];
        }

        // first bit is the sign
        if ((tlvP->value[0]&0x80) == 0x80)
        {
            *dataP = 0 - *dataP;
        }
    }

    return 1;
}
