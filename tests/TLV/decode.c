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

#include "core/liblwm2m.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

static void prv_output_buffer(uint8_t * buffer,
                              int length)
{
    int i;
    uint8_t array[16];

    if (length == 0 || length > 16) printf("\n");

    i = 0;
    while (i < length)
    {
        int j;
        printf("  ");

        memcpy(array, buffer+i, 16);

        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            printf("%02X ", array[j]);
        }
        if (length > 16)
        {
            while (j < 16)
            {
                printf("   ");
                j++;
            }
        }
        printf("  ");
        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            if (isprint(array[j]))
                printf("%c ", array[j]);
            else
                printf(". ");
        }
        printf("\n");

        i += 16;
    }
}

void print_indent(int num)
{
    int i;

    for ( i = 0 ; i< num ; i++)
        printf("\t");
}

void decode(char * buffer,
            size_t buffer_len,
            int indent)
{
    lwm2m_tlv_type_t type;
    uint16_t id;
    size_t dataIndex;
    size_t dataLen;
    int length = 0;
    int result;

    while (0 != (result = lwm2m_decodeTLV(buffer + length, buffer_len - length, &type, &id, &dataIndex, &dataLen)))
    {
        print_indent(indent);
        printf("type: ");
        switch (type)
        {
        case TLV_OBJECT_INSTANCE:
            printf("TLV_OBJECT_INSTANCE\r\n");
            break;
        case TLV_RESSOURCE_INSTANCE:
            printf("TLV_RESSOURCE_INSTANCE\r\n");
            break;
        case TLV_MULTIPLE_INSTANCE:
            printf("TLV_MULTIPLE_INSTANCE\r\n");
            break;
        case TLV_RESSOURCE:
            printf("TLV_RESSOURCE\r\n");
            break;
        default:
            printf("unknown (%d)\r\n", (int)type);
            break;
        }
        print_indent(indent);
        printf("id: %d\r\n", id);
        print_indent(indent);
        printf("data (%d bytes): ", dataLen);
        prv_output_buffer(buffer + length + dataIndex, dataLen);
        if (type == TLV_OBJECT_INSTANCE || type == TLV_MULTIPLE_INSTANCE)
        {
            decode(buffer + length + dataIndex, dataLen, indent+1);
        }
        else if (dataLen <= 8)
        {
            int64_t value;
            if (0 != lwm2m_opaqueToInt(buffer + length + dataIndex, dataLen, &value))
            {
                print_indent(indent);
                printf("  as int: %ld\r\n", value);
            }
        }

        length += result;
    }
}

int main(int argc, char *argv[])
{
    char buffer1[] = {0x03, 0x0A, 0xC1, 0x01, 0x14, 0x03, 0x0B, 0xC1, 0x01, 0x15, 0x03, 0x0C, 0xC1, 0x01, 0x16};
    char buffer2[] = {0xC8, 0x00, 0x14, 0x4F, 0x70, 0x65, 0x6E, 0x20, 0x4D, 0x6F, 0x62, 0x69, 0x6C, 0x65, 0x20,
                      0x41, 0x6C, 0x6C, 0x69, 0x61, 0x6E, 0x63, 0x65, 0xC8, 0x01, 0x16, 0x4C, 0x69, 0x67, 0x68,
                      0x74, 0x77 , 0x65, 0x69, 0x67, 0x68, 0x74, 0x20, 0x4D, 0x32, 0x4D, 0x20, 0x43, 0x6C, 0x69,
                      0x65, 0x6E, 0x74 , 0xC8, 0x02, 0x09, 0x33, 0x34, 0x35, 0x30, 0x30, 0x30, 0x31, 0x32, 0x33,
                      0xC3, 0x03, 0x31, 0x2E , 0x30, 0x86, 0x06, 0x41, 0x00, 0x01, 0x41, 0x01, 0x05, 0x88, 0x07,
                      0x08, 0x42, 0x00, 0x0E, 0xD8 , 0x42, 0x01, 0x13, 0x88, 0x87, 0x08, 0x41, 0x00, 0x7D, 0x42,
                      0x01, 0x03, 0x84, 0xC1, 0x09, 0x64 , 0xC1, 0x0A, 0x0F, 0x83, 0x0B, 0x41, 0x00, 0x00, 0xC4,
                      0x0D, 0x51, 0x82, 0x42, 0x8F, 0xC6, 0x0E, 0x2B, 0x30, 0x32, 0x3A, 0x30, 0x30, 0xC1, 0x0F, 0x55};

    printf("Buffer 1: \r\r\n");
    decode(buffer1, sizeof(buffer1), 0);
    printf("\r\rBuffer 2: \r\r\n");
    decode(buffer2, sizeof(buffer2), 0);

}

