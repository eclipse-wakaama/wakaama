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

#include "internals.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define _PRV_64BIT_BUFFER_SIZE 8


static int prv_create_header(uint8_t * header,
                             lwm2m_tlv_type_t type,
                             uint16_t id,
                             size_t data_len)
{
    int header_len = 2;

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
        header_len += 1;
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
        header[header_len] = data_len;
        header_len += 1;
    }
    else if (data_len <= 0xFFFF)
    {
        header[0] |= 0x10;
        header[header_len] = (data_len >> 8) & 0XFF;
        header[header_len+1] = data_len & 0XFF;
        header_len += 2;
    }
    else if (data_len <= 0xFFFFFF)
    {
        header[0] |= 0x18;
        header[header_len] = (data_len >> 16) & 0XFF;
        header[header_len+1] = (data_len >> 8) & 0XFF;
        header[header_len+2] = data_len & 0XFF;
        header_len += 3;
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
        *dataP = (*dataP << 8) + buffer[i];
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
            (*dataP)[size].type = LWM2M_STATIC_DATA | LWM2M_TYPE_RESSOURCE_INSTANCE;
            break;
        case TLV_MULTIPLE_INSTANCE:
            (*dataP)[size].type = LWM2M_TYPE_MULTIPLE_RESSOURCE;
            break;
        case TLV_RESSOURCE:
            (*dataP)[size].type = LWM2M_STATIC_DATA | LWM2M_TYPE_RESSOURCE;
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
            (*dataP)[size].length = dataLen;
            (*dataP)[size].value = buffer + length + dataIndex;
        }
        size++;
        length += result;
    }

    return size;
}

void lwm2m_tlv_free(int size,
                    lwm2m_tlv_t * tlvP)
{
    int i;

    for (i = 0 ; i < size ; i++)
    {
        if (!LWM2M_IS_STATIC(tlvP[i].type))
        {
            if (LWM2M_TLV_TYPE(tlvP[i].type) == LWM2M_TYPE_MULTIPLE_RESSOURCE
             || LWM2M_TLV_TYPE(tlvP[i].type) == TLV_OBJECT_INSTANCE)
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
