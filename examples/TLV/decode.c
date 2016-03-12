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
#include "internals.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "commandline.h"

static void print_indent(int num)
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
        output_buffer(stdout, buffer + length + dataIndex, dataLen, 0);
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

static void dump_json(uint8_t * buffer,
                      int length)
{
    int i;

    printf("JSON length: %d\n", length);
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
    lwm2m_data_t * tlvP;
    int size;
    int length;
    uint8_t * buffer;
    lwm2m_media_type_t format = LWM2M_CONTENT_JSON;

    size = lwm2m_data_parse((uint8_t *)testBuf, testLen, format, &tlvP);
    if (size <= 0)
    {
        printf("\n\nJSON buffer %s decoding failed !\n", id);
        return;
    }
    else
        printf("\n\nJSON buffer %s decoding:\n", id);

    dump_tlv(stdout, size, tlvP, 0);
    length = lwm2m_data_serialize(size, tlvP, &format, &buffer);
    if (length <= 0)
    {
        printf("\n\nSerialize Buffer %s to JSON failed.\n", id);
    }
    else
    {
        dump_json(buffer, length);
        lwm2m_free(buffer);
    }
    lwm2m_data_free(size, tlvP);
}

static void test_TLV(char * testBuf,
                     size_t testLen,
                     char * id)
{
    lwm2m_data_t * tlvP;
    int size;
    int length;
    uint8_t * buffer;
    lwm2m_media_type_t format = LWM2M_CONTENT_TLV;

    printf("Buffer %s:\n", id);
    decode(testBuf, testLen, 0);
    printf("\n\nBuffer %s using lwm2m_data_t:\n", id);
    size = lwm2m_data_parse((uint8_t *)testBuf, testLen, format, &tlvP);
    dump_tlv(stdout, size, tlvP, 0);
    length = lwm2m_data_serialize(size, tlvP, &format, &buffer);
    if (length != testLen)
    {
        printf("\n\nSerialize Buffer %s to TLV failed: %d bytes instead of %d\n", id, length, testLen);
    }
    else if (memcmp(buffer, testBuf, length) != 0)
    {
        printf("\n\nSerialize Buffer %s to TLV failed:\n", id);
        output_buffer(stdout, buffer, length, 0);
        printf("\ninstead of:\n");
        output_buffer(stdout, testBuf, length, 0);
    }
    else
    {
        printf("\n\nSerialize Buffer %s to TLV OK\n", id);
    }
    lwm2m_free(buffer);
    format = LWM2M_CONTENT_JSON;
    length = lwm2m_data_serialize(size, tlvP, &format, &buffer);
    if (length <= 0)
    {
        printf("\n\nSerialize Buffer %s to JSON failed.\n", id);
    }
    else
    {
        dump_json(buffer, length);
        lwm2m_free(buffer);
    }
    lwm2m_data_free(size, tlvP);
}

static void test_data(lwm2m_data_t * tlvP,
                      int size,
                      char * id)
{
    int length;
    uint8_t * buffer;
    lwm2m_media_type_t format = LWM2M_CONTENT_TLV;

    printf("lwm2m_data_t %s:\n", id);
    dump_tlv(stdout, size, tlvP, 0);
    length = lwm2m_data_serialize(size, tlvP, &format, &buffer);
    if (length <= 0)
    {
        printf("\n\nSerialize lwm2m_data_t %s to TLV failed.\n", id);
    }
    else
    {
        printf("\n\nSerialize lwm2m_data_t %s to TLV:\n", id);
        output_buffer(stdout, buffer, length, 0);
        lwm2m_free(buffer);
    }
    format = LWM2M_CONTENT_JSON;
    length = lwm2m_data_serialize(size, tlvP, &format, &buffer);
    if (length <= 0)
    {
        printf("\n\nSerialize lwm2m_data_t %s to JSON failed.\n", id);
    }
    else
    {
        printf("\n\nSerialize lwm2m_data_t %s to JSON:\n", id);
        dump_json(buffer, length);
        lwm2m_free(buffer);
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
    char * buffer4 = "{\"e\":[                                      \
                         {\"n\":\"0\",\"sv\":\"a \\\"test\\\"\"},   \
                         {\"n\":\"1\",\"v\":2015},                  \
                         {\"n\":\"2/0\",\"bv\":true},               \
                         {\"n\":\"2/1\",\"bv\":false}]              \
                       }";
    char * buffer5 = "{\"e\":[                              \
                         {    \"n\" :   \"0\",              \
                            \"sv\":\"LWM2M\"},              \
                         {\"v\":2015, \"n\":\"1\"},         \
                         {\"n\":\"2/0\",    \"bv\":true},   \
                         {\"bv\": false, \"n\":\"2/1\"}]    \
                       }";
    char * buffer6 = "{\"e\":[                              \
                         {\"n\":\"0\",\"v\":1234},          \
                         {\"n\":\"1\",\"v\":56.789},        \
                         {\"n\":\"2/0\",\"v\":0.23},        \
                         {\"n\":\"2/1\",\"v\":-52.0006}]    \
                       }";

    lwm2m_data_t data1[] = {                                                                \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 0, 0, NULL},    \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 1, 0, NULL},    \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 2, 0, NULL},    \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 3, 0, NULL},    \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 4, 0, NULL},    \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 5, 0, NULL},    \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 6, 0, NULL},    \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 7, 0, NULL},    \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 8, 0, NULL},    \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 9, 0, NULL},    \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 10, 0, NULL},   \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 11, 0, NULL},   \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 12, 0, NULL},   \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 13, 0, NULL},   \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 14, 0, NULL},   \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 15, 0, NULL},   \
                             {0, LWM2M_TYPE_RESOURCE, LWM2M_TYPE_UNDEFINED, 16, 0, NULL},   \
                           };
    lwm2m_data_encode_int(12, data1);
    lwm2m_data_encode_int(-12, data1 + 1);
    lwm2m_data_encode_int(255, data1 + 2);
    lwm2m_data_encode_int(1000, data1 + 3);
    lwm2m_data_encode_int(-1000, data1 + 4);
    lwm2m_data_encode_int(65535, data1 + 5);
    lwm2m_data_encode_int(3000000, data1 + 6);
    lwm2m_data_encode_int(-3000000, data1 + 7);
    lwm2m_data_encode_int(4294967295, data1 + 8);
    lwm2m_data_encode_int(7000000000, data1 + 9);
    lwm2m_data_encode_int(-7000000000, data1 + 10);
    lwm2m_data_encode_float(3.14, data1 + 11);
    lwm2m_data_encode_float(-3.14, data1 + 12);
    lwm2m_data_encode_float(4E+38, data1 + 13);
    lwm2m_data_encode_float(-4E+38, data1 + 14);
    lwm2m_data_encode_bool(true, data1 + 15);
    lwm2m_data_encode_bool(false, data1 + 16);


    test_TLV(buffer1, sizeof(buffer1), "1");
    printf("\n\n============\n\n");
    test_TLV(buffer2, sizeof(buffer2), "2");
    printf("\n\n============\n\n");
    test_JSON(buffer3, strlen(buffer3), "3");
    printf("\n\n============\n\n");
    test_JSON(buffer4, strlen(buffer4), "4");
    printf("\n\n============\n\n");
    test_JSON(buffer5, strlen(buffer5), "5");
    printf("\n\n============\n\n");
    test_JSON(buffer6, strlen(buffer6), "6");
    printf("\n\n============\n\n");
    test_data(data1, sizeof(data1)/sizeof(lwm2m_data_t), "1");
    printf("\n\n");
}

