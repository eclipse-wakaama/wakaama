/*******************************************************************************
 *
 * Copyright (c) 2013, 2014, 2015 Intel Corporation and others.
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

#include "liblwm2m.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define _JSON_BUFFER_SIZE   1024

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

void dump_tlv(int size,
              lwm2m_tlv_t * tlvP,
              int indent)
{
    int i;

    for(i= 0 ; i < size ; i++)
    {
        print_indent(indent);
        printf("type: ");
        switch (tlvP[i].type)
        {
        case LWM2M_TYPE_OBJECT_INSTANCE:
            printf("LWM2M_TYPE_OBJECT_INSTANCE\r\n");
            break;
        case LWM2M_TYPE_RESOURCE_INSTANCE:
            printf("LWM2M_TYPE_RESOURCE_INSTANCE\r\n");
            break;
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            printf("LWM2M_TYPE_MULTIPLE_RESOURCE\r\n");
            break;
        case LWM2M_TYPE_RESOURCE:
            printf("LWM2M_TYPE_RESOURCE\r\n");
            break;
        default:
            printf("unknown (%d)\r\n", (int)tlvP[i].type);
            break;
        }
        print_indent(indent);
        printf("id: %d\r\n", tlvP[i].id);
        print_indent(indent);
        printf("data (%d bytes): ", tlvP[i].length);
        prv_output_buffer(tlvP[i].value, tlvP[i].length);
        if (tlvP[i].type == LWM2M_TYPE_OBJECT_INSTANCE
         || tlvP[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
        {
            dump_tlv(tlvP[i].length, (lwm2m_tlv_t *)(tlvP[i].value), indent+1);
        }
        else if (tlvP[i].length <= 8
              && (tlvP[i].flags & LWM2M_TLV_FLAG_TEXT_FORMAT) == 0)
        {
            int64_t value;
            if (0 != lwm2m_opaqueToInt(tlvP[i].value, tlvP[i].length, &value))
            {
                print_indent(indent);
                printf("  as int: %ld\r\n", value);
            }
        }
    }
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
        case LWM2M_TYPE_OBJECT_INSTANCE:
            printf("LWM2M_TYPE_OBJECT_INSTANCE\r\n");
            break;
        case LWM2M_TYPE_RESOURCE_INSTANCE:
            printf("LWM2M_TYPE_RESOURCE_INSTANCE\r\n");
            break;
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            printf("LWM2M_TYPE_MULTIPLE_RESOURCE\r\n");
            break;
        case LWM2M_TYPE_RESOURCE:
            printf("LWM2M_TYPE_RESOURCE\r\n");
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
        if (type == LWM2M_TYPE_OBJECT_INSTANCE || type == LWM2M_TYPE_MULTIPLE_RESOURCE)
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

static void dump_json(char * buffer,
                      int length)
{
    int i;

    for (i = 0 ; i < length ; i++)
    {
        printf("%c", buffer[i]);
    }
    printf("\n");
}

static void test_JSON(char * testBuf,
                      size_t testLen,
                      char * id)
{
    lwm2m_tlv_t * tlvP;
    int size;
    int length;
    char bufferJSON[_JSON_BUFFER_SIZE];

    size = lwm2m_tlv_parse_json(testBuf, testLen, &tlvP);
    if (size <= 0)
    {
        printf("\n\nJSON buffer %s decoding failed !\n", id);
        return;
    }
    else
        printf("\n\nJSON buffer %s decoding:\n", id);

    dump_tlv(size, tlvP, 0);
    length = lwm2m_tlv_serialize_json(size, tlvP, bufferJSON, _JSON_BUFFER_SIZE);
    dump_json(bufferJSON, length);
    lwm2m_tlv_free(size, tlvP);
}

static void test_TLV(char * testBuf,
                     size_t testLen,
                     char * id)
{
    lwm2m_tlv_t * tlvP;
    int size;
    int length;
    char * buffer;
    char bufferJSON[_JSON_BUFFER_SIZE];

    printf("Buffer %s:\n", id);
    decode(testBuf, testLen, 0);
    printf("\n\nBuffer %s using lwm2m_tlv_t:\n", id);
    size = lwm2m_tlv_parse(testBuf, testLen, &tlvP);
    dump_tlv(size, tlvP, 0);
    length = lwm2m_tlv_serialize(size, tlvP, &buffer);
    if (length != testLen)
    {
        printf("\n\nSerialize Buffer %s to TLV failed: %d bytes instead of %d\n", id, length, testLen);
    }
    else if (memcmp(buffer, testBuf, length) != 0)
    {
        printf("\n\nSerialize Buffer %s to TLV failed:\n", id);
        prv_output_buffer(buffer, length);
        printf("\ninstead of:\n");
        prv_output_buffer(testBuf, length);
    }
    else
    {
        printf("\n\nSerialize Buffer %s to TLV OK\n", id);
    }
    lwm2m_free(buffer);
    length = lwm2m_tlv_serialize_json(size, tlvP, bufferJSON, _JSON_BUFFER_SIZE);
    if (length <= 0)
    {
        printf("\n\nSerialize Buffer %s to JSON failed.\n", id);
    }
    else
    {
        dump_json(bufferJSON, length);
    }
    lwm2m_tlv_free(size, tlvP);
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
    char * buffer3 = "{\"e\":[                                              \
                         {\"n\":\"0\",\"sv\":\"Open Mobile Alliance\"},      \
                         {\"n\":\"1\",\"sv\":\"Lightweight M2M Client\"},    \
                         {\"n\":\"2\",\"sv\":\"345000123\"},                 \
                         {\"n\":\"3\",\"sv\":\"1.0\"},                       \
                         {\"n\":\"6/0\",\"v\":1},                            \
                         {\"n\":\"6/1\",\"v\":5},                            \
                         {\"n\":\"7/0\",\"v\":3800},                         \
                         {\"n\":\"7/1\",\"v\":5000},                         \
                         {\"n\":\"8/0\",\"v\":125},                          \
                         {\"n\":\"8/1\",\"v\":900},                          \
                         {\"n\":\"9\",\"v\":100},                            \
                         {\"n\":\"10\",\"v\":15},                            \
                         {\"n\":\"11/0\",\"v\":0},                           \
                         {\"n\":\"13\",\"v\":1367491215},                    \
                         {\"n\":\"14\",\"sv\":\"+02:00\"},                   \
                         {\"n\":\"15\",\"sv\":\"U\"}]                        \
                       }";
    char * buffer4 = "{\"e\":[                                \
                         {\"n\":\"0\",\"sv\":\"a \\\"test\\\"\"},      \
                         {\"n\":\"1\",\"v\":2015},            \
                         {\"n\":\"2/0\",\"bv\":true},         \
                         {\"n\":\"2/1\",\"bv\":false}]        \
                       }";
    char * buffer5 = "{\"e\":[                                \
                         {    \"n\" :   \"0\",                \
                            \"sv\":\"LWM2M\"},               \
                         {\"v\":2015, \"n\":\"1\"},           \
                         {\"n\":\"2/0\",    \"bv\":true},         \
                         {\"bv\": false, \"n\":\"2/1\"}]        \
                       }";


    test_TLV(buffer1, sizeof(buffer1), "1");
    printf("\n\n============\n\n");
    test_TLV(buffer2, sizeof(buffer2), "2");
    printf("\n\n============\n\n");
    test_JSON(buffer3, strlen(buffer3), "3");
    printf("\n\n============\n\n");
    test_JSON(buffer4, strlen(buffer3), "4");
    printf("\n\n============\n\n");
    test_JSON(buffer5, strlen(buffer3), "5");
}

