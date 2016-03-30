/*******************************************************************************
 *
 * Copyright (c) 2015 Bosch Software Innovations GmbH, Germany.
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
 *    Bosch Software Innovations GmbH - Please refer to git log
 *
 *******************************************************************************/

#include <gtest/gtest.h>

#include "internals.h"
#include "liblwm2m.h"
#include "memtest.h"

void fill(uint8_t *data, int size) {
    int index;
    for (index = 0; index < size; ++index) {
        data[index] = index;
    }
}

TEST(tlvtests, test_tlv_new)
{
   MEMORY_TRACE_BEFORE;
   lwm2m_data_t *dataP =  lwm2m_data_new(10);
   EXPECT_TRUE(dataP);
   MEMORY_TRACE_AFTER(<);
}

TEST(tlvtests, test_tlv_free)
{
   MEMORY_TRACE_BEFORE;
   lwm2m_data_t *dataP =  lwm2m_data_new(10);
   ASSERT_TRUE(dataP);
   lwm2m_data_free(10, dataP);
   MEMORY_TRACE_AFTER_EQ;
}

TEST(tlvtests, test_decodeTLV)
{
    MEMORY_TRACE_BEFORE;
    uint8_t data1[] = {0xC3, 55, 1, 2, 3};
    uint8_t data2[] = {0x28, 2, 3, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    uint8_t data3[0x194] = {0x90, 33, 1, 0x90 };
    lwm2m_tlv_type_t type;
    uint16_t id = 0;
    size_t   index = 0;
    size_t   length = 0;
    int result;


    result = lwm2m_decode_TLV(data1, sizeof(data1) - 1, &type, &id, &index, &length);
    ASSERT_EQ(result, 0);

    result = lwm2m_decode_TLV(data1, sizeof(data1), &type, &id, &index, &length);
    ASSERT_EQ(result, 5);
    ASSERT_EQ(type, LWM2M_TYPE_RESOURCE);
    ASSERT_EQ(id, 55);
    ASSERT_EQ(index, 2);
    ASSERT_EQ(length, 3);

    result = lwm2m_decode_TLV(data2, sizeof(data2) - 1, &type, &id, &index, &length);
    ASSERT_EQ(result, 0);

    result = lwm2m_decode_TLV(data2, sizeof(data2), &type, &id, &index, &length);
    ASSERT_EQ(result, 13);
    ASSERT_EQ(type, LWM2M_TYPE_OBJECT_INSTANCE);
    ASSERT_EQ(id, 0x0203);
    ASSERT_EQ(index, 4);
    ASSERT_EQ(length, 9);

    result = lwm2m_decode_TLV(data3, sizeof(data3) - 1, &type, &id, &index, &length);
    ASSERT_EQ(result, 0);

    result = lwm2m_decode_TLV(data3, sizeof(data3), &type, &id, &index, &length);
    ASSERT_EQ(result, 0x194);
    ASSERT_EQ(type, LWM2M_TYPE_MULTIPLE_RESOURCE);
    ASSERT_EQ(id, 33);
    ASSERT_EQ(index, 4);
    ASSERT_EQ(length, 0x190);

    MEMORY_TRACE_AFTER_EQ;
}

TEST(tlvtests, test_opaqueToInt)
{
    MEMORY_TRACE_BEFORE;
    // Data represented in big endian, two'2 complement
    uint8_t data1[] = {1 };
    uint8_t data2[] = {1, 2 };
    uint8_t data3[] = {0xfe, 0xfd, 0xfc, 0xfc};
    uint8_t data4[] = {0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8, 0xf8};
    // Result in platform endianess
    int64_t value = 0;
    int length;


    length = utils_opaqueToInt(data1, sizeof(data1), &value);
    ASSERT_EQ(length, 1);
    ASSERT_EQ(value, 1);

    length = utils_opaqueToInt(data2, sizeof(data2), &value);
    ASSERT_EQ(length, 2);
    ASSERT_EQ(value, 0x102);

    length = utils_opaqueToInt(data3, sizeof(data3), &value);
    ASSERT_EQ(length, 4);
    ASSERT_EQ(value, -0x1020304);

    length = utils_opaqueToInt(data4, sizeof(data4), &value);
    ASSERT_EQ(length, 8);
    ASSERT_EQ(value, -0x102030405060708);

    MEMORY_TRACE_AFTER_EQ;
}

TEST(tlvtests, test_tlv_parse)
{
    MEMORY_TRACE_BEFORE;
    // Resource 55 {1, 2, 3}
    uint8_t data1[] = {0xC3, 55, 1, 2, 3};
    // Instance 0x203 {Resource 55 {1, 2, 3}, Resource 66 {4, 5, 6, 7, 8, 9, 10, 11, 12 } }
    uint8_t data2[] = {0x28, 2, 3, 17, 0xC3, 55, 1, 2, 3, 0xC8, 66, 9, 4, 5, 6, 7, 8, 9, 10, 11, 12, };
    // Instance 11 {MultiResource 11 {ResourceInstance 0 {1, 2, 3}, ResourceInstance 1 {4, 5, 6, 7, 8, 9, ... } }
    uint8_t data3[174] = {0x08, 11, 171, 0x88, 77, 168, 0x43, 0, 1, 2, 3, 0x48, 1, 160, 4, 5, 6, 7, 8, 9};
    int result;
    lwm2m_data_t *dataP;
    lwm2m_data_t *tlvSubP;

    result = lwm2m_data_parse(NULL, data1, sizeof(data1), LWM2M_CONTENT_TLV, &dataP);
    ASSERT_EQ(result, 1);
    ASSERT_TRUE(dataP);
    ASSERT_EQ(dataP->type, LWM2M_TYPE_RESOURCE);
    ASSERT_EQ(dataP->id, 55);
    ASSERT_EQ(dataP->length, 3);
    ASSERT_TRUE(0 == memcmp(dataP->value, &data1[2], 3));
    lwm2m_data_free(result, dataP);

    result = lwm2m_data_parse(NULL, data2, sizeof(data2), LWM2M_CONTENT_TLV, &dataP);
    ASSERT_EQ(result, 1);
    ASSERT_TRUE(dataP);
    ASSERT_EQ(dataP->type, LWM2M_TYPE_OBJECT_INSTANCE);
    ASSERT_EQ(dataP->id, 0x203);
    ASSERT_EQ(dataP->length, 2);
    ASSERT_TRUE(dataP->value);
    tlvSubP = (lwm2m_data_t *)dataP->value;

    ASSERT_EQ(tlvSubP[0].type, LWM2M_TYPE_RESOURCE);
    ASSERT_EQ(tlvSubP[0].id, 55);
    ASSERT_EQ(tlvSubP[0].length, 3);
    ASSERT_TRUE(0 == memcmp(tlvSubP[0].value, &data2[6], 3));

    ASSERT_EQ(tlvSubP[1].type, LWM2M_TYPE_RESOURCE);
    ASSERT_EQ(tlvSubP[1].id, 66);
    ASSERT_EQ(tlvSubP[1].length, 9);
    ASSERT_TRUE(0 == memcmp(tlvSubP[1].value, &data2[12], 9));
    lwm2m_data_free(result, dataP);

    result = lwm2m_data_parse(NULL, data3, sizeof(data3), LWM2M_CONTENT_TLV, &dataP);
    ASSERT_EQ(result, 1);
    ASSERT_TRUE(dataP);
    ASSERT_EQ(dataP->type, LWM2M_TYPE_OBJECT_INSTANCE);
    ASSERT_EQ(dataP->id, 11);
    ASSERT_EQ(dataP->length, 1);
    ASSERT_TRUE(dataP->value);
    tlvSubP = (lwm2m_data_t *)dataP->value;

    ASSERT_EQ(tlvSubP[0].type, LWM2M_TYPE_MULTIPLE_RESOURCE);
    ASSERT_EQ(tlvSubP[0].id, 77);
    ASSERT_EQ(tlvSubP[0].length, 2);
    ASSERT_TRUE(tlvSubP[0].value);
    tlvSubP = (lwm2m_data_t *)tlvSubP[0].value;

    ASSERT_EQ(tlvSubP[0].type, LWM2M_TYPE_RESOURCE_INSTANCE);
    ASSERT_EQ(tlvSubP[0].id, 0);
    ASSERT_EQ(tlvSubP[0].length, 3);
    ASSERT_TRUE(0 == memcmp(tlvSubP[0].value, &data3[8], 3));

    ASSERT_EQ(tlvSubP[1].type, LWM2M_TYPE_RESOURCE_INSTANCE);
    ASSERT_EQ(tlvSubP[1].id, 1);
    ASSERT_EQ(tlvSubP[1].length, 160);
    ASSERT_TRUE(0 == memcmp(tlvSubP[1].value, &data3[14], 160));
    lwm2m_data_free(result, dataP);

    MEMORY_TRACE_AFTER_EQ;
}

TEST(tlvtests, test_tlv_serialize)
{
    MEMORY_TRACE_BEFORE;

    int result;
    lwm2m_data_t *dataP;
    lwm2m_data_t *tlvSubP;
    uint8_t data1[] = {1, 2, 3, 4};
    uint8_t data2[170] = {5, 6, 7, 8};
    uint8_t* buffer;

    dataP =  lwm2m_data_new(1);
    ASSERT_TRUE(dataP);
    dataP->type = LWM2M_TYPE_OBJECT_INSTANCE;
    dataP->id = 3;
    dataP->length = 2;
    tlvSubP =  lwm2m_data_new(2);
    ASSERT_TRUE(tlvSubP);
    dataP->value = (uint8_t *) tlvSubP;

    tlvSubP[0].type = LWM2M_TYPE_RESOURCE;
    tlvSubP[0].flags = LWM2M_TLV_FLAG_STATIC_DATA;
    tlvSubP[0].id = 66;
    tlvSubP[0].length = sizeof(data1);
    tlvSubP[0].value = data1;

    tlvSubP[1].type = LWM2M_TYPE_MULTIPLE_RESOURCE;
    tlvSubP[1].id = 77;
    tlvSubP[1].length = 1;
    tlvSubP[1].value = (uint8_t *) lwm2m_data_new(1);
    ASSERT_TRUE(tlvSubP[1].value);
    tlvSubP = (lwm2m_data_t *)tlvSubP[1].value;

    tlvSubP[0].type = LWM2M_TYPE_RESOURCE_INSTANCE;
    tlvSubP[0].flags = LWM2M_TLV_FLAG_STATIC_DATA;
    tlvSubP[0].id = 0;
    tlvSubP[0].length = sizeof(data2);
    tlvSubP[0].value = data2;

    lwm2m_media_type_t media_type = LWM2M_CONTENT_TLV;
    result = lwm2m_data_serialize(NULL, 1, dataP, &media_type, &buffer);
    ASSERT_EQ(result, sizeof(data2) + sizeof(data1) + 11);

    ASSERT_EQ(buffer[0], 0x08);
    ASSERT_EQ(buffer[1], 3);
    ASSERT_EQ(buffer[2], sizeof(data2) + sizeof(data1) + 8);

    ASSERT_EQ(buffer[3], 0xC0 + sizeof(data1));
    ASSERT_EQ(buffer[4], 66);
    ASSERT_TRUE(0 == memcmp(data1, &buffer[5], sizeof(data1)));

    ASSERT_EQ(buffer[5 + sizeof(data1)], 0x88);
    ASSERT_EQ(buffer[6 + sizeof(data1)], 77);
    ASSERT_EQ(buffer[7 + sizeof(data1)], sizeof(data2) + 3);

    ASSERT_EQ(buffer[8 + sizeof(data1)], 0x48);
    ASSERT_EQ(buffer[9 + sizeof(data1)], 0);
    ASSERT_EQ(buffer[10 + sizeof(data1)], sizeof(data2));
    ASSERT_TRUE(0 == memcmp(data2, &buffer[11 + sizeof(data1)], sizeof(data2)));

    lwm2m_data_free(1, dataP);
    lwm2m_free(buffer);

    MEMORY_TRACE_AFTER_EQ;
}

TEST(tlvtests, test_tlv_encode_int)
{
   MEMORY_TRACE_BEFORE;
   lwm2m_data_t *dataP =  lwm2m_data_new(10);
   EXPECT_TRUE(dataP);

   lwm2m_data_encode_int(0x12, dataP);
   dataP[0].type = LWM2M_TYPE_RESOURCE;
   ASSERT_EQ(dataP[0].flags, 0);
   ASSERT_EQ(dataP[0].length, 1);
   ASSERT_TRUE(dataP[0].value);
   ASSERT_EQ(dataP[0].value[0], 0x12);

   dataP[1].flags = LWM2M_TLV_FLAG_TEXT_FORMAT;
   lwm2m_data_encode_int(18, &dataP[1]);
   dataP[1].type = LWM2M_TYPE_RESOURCE;
   ASSERT_EQ(dataP[1].length, 2);
   ASSERT_TRUE(dataP[1].value);
   ASSERT_TRUE(0 == memcmp(dataP[1].value, "18", 2));

   uint8_t data1[] = { 0xed, 0xcc };
   lwm2m_data_encode_int(-0x1234, &dataP[2]);
   dataP[2].type = LWM2M_TYPE_RESOURCE;
   ASSERT_EQ(dataP[2].length, 2);
   ASSERT_TRUE(dataP[2].value);
   ASSERT_TRUE(0 == memcmp(dataP[2].value, data1, 2));

   dataP[3].flags = LWM2M_TLV_FLAG_TEXT_FORMAT;
   lwm2m_data_encode_int(-14678, &dataP[3]);
   dataP[3].type = LWM2M_TYPE_RESOURCE;
   ASSERT_EQ(dataP[3].length, 6);
   ASSERT_TRUE(dataP[3].value);
   ASSERT_TRUE(0 == memcmp(dataP[3].value, "-14678", 6));

   uint8_t data2[] = { 0x7f, 0x34, 0x56, 0x78, 0x91, 0x22, 0x33, 0x44 };
   lwm2m_data_encode_int(0x7f34567891223344, &dataP[4]);
   dataP[4].type = LWM2M_TYPE_RESOURCE;
   ASSERT_EQ(dataP[4].length, 8);
   ASSERT_TRUE(dataP[4].value);
   ASSERT_TRUE(0 == memcmp(dataP[4].value, data2, 8));

   lwm2m_data_free(10, dataP);
   MEMORY_TRACE_AFTER_EQ;
}

TEST(tlvtests, test_tlv_decode_int)
{
   MEMORY_TRACE_BEFORE;
   uint8_t data1[] = { 0x12, 0x34 };
   uint8_t data2[] = { 0xed, 0xcc };
   uint8_t data3[] = { 0x7f, 0x34, 0x56, 0x78, 0x91, 0x22, 0x33, 0x44 };
   int64_t value;
   int result;
   lwm2m_data_t *dataP =  lwm2m_data_new(10);
   EXPECT_TRUE(dataP);

   result = lwm2m_data_decode_int(dataP, &value);
   ASSERT_EQ(result, 0);

   dataP[0].flags = LWM2M_TLV_FLAG_STATIC_DATA;
   dataP[0].length = 1;
   dataP[0].value = data1;
   result = lwm2m_data_decode_int(dataP, &value);
   ASSERT_EQ(result, 1);
   ASSERT_EQ(value, 0x12);

   dataP[0].length = 2;
   dataP[0].value = data2;
   result = lwm2m_data_decode_int(dataP, &value);
   ASSERT_EQ(result, 1);
   ASSERT_EQ(value, -0x1234);

   dataP[0].length = 8;
   dataP[0].value = data3;
   result = lwm2m_data_decode_int(dataP, &value);
   ASSERT_EQ(result, 1);
   ASSERT_EQ(value, 0x7f34567891223344);

   dataP[0].length = 9;
   result = lwm2m_data_decode_int(dataP, &value);
   ASSERT_EQ(result, 0);

   dataP[0].flags |= LWM2M_TLV_FLAG_TEXT_FORMAT;
   dataP[0].length = 2;
   dataP[0].value = (uint8_t *) "18";
   result = lwm2m_data_decode_int(dataP, &value);
   ASSERT_EQ(result, 1);
   ASSERT_EQ(value, 18);

   dataP[0].length = 9;
   dataP[0].value = (uint8_t *) "-56923456";
   result = lwm2m_data_decode_int(dataP, &value);
   ASSERT_EQ(result, 1);
   ASSERT_EQ(value, -56923456);

   lwm2m_data_free(10, dataP);
   MEMORY_TRACE_AFTER_EQ;
}

TEST(tlvtests, test_tlv_encode_bool)
{
   MEMORY_TRACE_BEFORE;
   lwm2m_data_t *dataP =  lwm2m_data_new(10);
   EXPECT_TRUE(dataP);

   lwm2m_data_encode_bool(2, dataP);
   dataP[0].type = LWM2M_TYPE_RESOURCE;
   ASSERT_EQ(dataP[0].flags, 0);
   ASSERT_EQ(dataP[0].length, 1);
   ASSERT_TRUE(dataP[0].value);
   ASSERT_EQ(dataP[0].value[0], 1);

   lwm2m_data_encode_bool(0, &dataP[1]);
   dataP[1].type = LWM2M_TYPE_RESOURCE;
   ASSERT_EQ(dataP[1].flags, 0);
   ASSERT_EQ(dataP[1].length, 1);
   ASSERT_TRUE(dataP[1].value);
   ASSERT_EQ(dataP[1].value[0], 0);

   dataP[2].flags = LWM2M_TLV_FLAG_TEXT_FORMAT;
   lwm2m_data_encode_bool(0, &dataP[2]);
   dataP[2].type = LWM2M_TYPE_RESOURCE;
   ASSERT_EQ(dataP[2].flags, LWM2M_TLV_FLAG_TEXT_FORMAT);
   ASSERT_EQ(dataP[2].length, 1);
   ASSERT_TRUE(dataP[2].value);
   ASSERT_EQ(dataP[2].value[0], '0');

   dataP[3].flags = LWM2M_TLV_FLAG_TEXT_FORMAT;
   lwm2m_data_encode_bool(4, &dataP[3]);
   dataP[3].type = LWM2M_TYPE_RESOURCE;
   ASSERT_EQ(dataP[3].flags, LWM2M_TLV_FLAG_TEXT_FORMAT);
   ASSERT_EQ(dataP[3].length, 1);
   ASSERT_TRUE(dataP[3].value);
   ASSERT_EQ(dataP[3].value[0], '1');

   lwm2m_data_free(10, dataP);
   MEMORY_TRACE_AFTER_EQ;
}

TEST(tlvtests, test_tlv_decode_bool)
{
   MEMORY_TRACE_BEFORE;
   uint8_t data1[] = { 0 };
   uint8_t data2[] = { 1 };
   uint8_t data3[] = { 2 };
   bool value;
   int result;
   lwm2m_data_t *dataP =  lwm2m_data_new(10);
   EXPECT_TRUE(dataP);

   result = lwm2m_data_decode_bool(dataP, &value);
   ASSERT_EQ(result, 0);

   dataP[0].length = 2;
   result = lwm2m_data_decode_bool(dataP, &value);
   ASSERT_EQ(result, 0);

   dataP[0].flags = LWM2M_TLV_FLAG_STATIC_DATA;
   dataP[0].length = 1;
   dataP[0].value = data1;
   result = lwm2m_data_decode_bool(dataP, &value);
   ASSERT_EQ(result, 1);
   ASSERT_FALSE(value);

   dataP[0].value = data2;
   result = lwm2m_data_decode_bool(dataP, &value);
   ASSERT_EQ(result, 1);
   ASSERT_TRUE(value);

   dataP[0].value = data3;
   result = lwm2m_data_decode_bool(dataP, &value);
   ASSERT_EQ(result, 0);

   dataP[0].flags |= LWM2M_TLV_FLAG_TEXT_FORMAT;
   dataP[0].value = (uint8_t *)"0";
   result = lwm2m_data_decode_bool(dataP, &value);
   ASSERT_EQ(result, 1);
   ASSERT_FALSE(value);

   dataP[0].value = (uint8_t *)"1";
   result = lwm2m_data_decode_bool(dataP, &value);
   ASSERT_EQ(result, 1);
   ASSERT_TRUE(value);

   dataP[0].value = (uint8_t *)"2";
   result = lwm2m_data_decode_bool(dataP, &value);
   ASSERT_EQ(result, 0);

   lwm2m_data_free(10, dataP);
   MEMORY_TRACE_AFTER_EQ;
}
