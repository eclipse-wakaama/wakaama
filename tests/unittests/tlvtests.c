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

#include "tests.h"
#include "CUnit/Basic.h"
#include "liblwm2m.h"
#include "memtest.h"

static void fill(uint8_t *data, int size) {
    int index;
    for (index = 0; index < size; ++index) {
        data[index] = index;
    }
}

static void test_tlv_new(void)
{
   MEMORY_TRACE_BEFORE;
   lwm2m_data_t *dataP =  lwm2m_data_new(10);
   CU_ASSERT_PTR_NOT_NULL(dataP);
   MEMORY_TRACE_AFTER(<);
}

static void test_tlv_free(void)
{
   MEMORY_TRACE_BEFORE;
   lwm2m_data_t *dataP =  lwm2m_data_new(10);
   CU_ASSERT_PTR_NOT_NULL_FATAL(dataP);
   lwm2m_data_free(10, dataP);
   MEMORY_TRACE_AFTER_EQ;
}

static void test_opaqueToTLV()
{
    MEMORY_TRACE_BEFORE;
    uint8_t data[0x200];
    uint8_t buffer[1024];
    int result;

    fill(data, sizeof(data));

    result = lwm2m_opaqueToTLV(LWM2M_TYPE_RESOURCE, data, 3, 55, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(result, 5);
    CU_ASSERT_EQUAL(buffer[0], 0xC3);
    CU_ASSERT_EQUAL(buffer[1], 55);
    CU_ASSERT(0 == memcmp(data, &buffer[2], 3));

    result = lwm2m_opaqueToTLV(LWM2M_TYPE_OBJECT_INSTANCE, data, 9, 0x0203, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(result, 13);
    CU_ASSERT_EQUAL(buffer[0], 0x28);
    CU_ASSERT_EQUAL(buffer[1], 0x02);
    CU_ASSERT_EQUAL(buffer[2], 0x03);
    CU_ASSERT_EQUAL(buffer[3], 9);
    CU_ASSERT(0 == memcmp(data, &buffer[4], 9));

    result = lwm2m_opaqueToTLV(LWM2M_TYPE_MULTIPLE_RESOURCE, data, 0x190, 33, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(result, 0x194);
    CU_ASSERT_EQUAL(buffer[0], 0x90);
    CU_ASSERT_EQUAL(buffer[1], 33);
    CU_ASSERT_EQUAL(buffer[2], 0x1);
    CU_ASSERT_EQUAL(buffer[3], 0x90);
    CU_ASSERT(0 == memcmp(data, &buffer[4], 0x190));

    MEMORY_TRACE_AFTER_EQ;
}

static void test_boolToTLV()
{
    MEMORY_TRACE_BEFORE;
    uint8_t buffer[16];
    int result;

    result = lwm2m_boolToTLV(LWM2M_TYPE_RESOURCE, 1, 55, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(result, 3);
    CU_ASSERT_EQUAL(buffer[0], 0xC1);
    CU_ASSERT_EQUAL(buffer[1], 55);
    CU_ASSERT_EQUAL(buffer[2], 1);

    result = lwm2m_boolToTLV(LWM2M_TYPE_RESOURCE, 0, 0x2255, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(result, 4);
    CU_ASSERT_EQUAL(buffer[0], 0xE1);
    CU_ASSERT_EQUAL(buffer[1], 0x22);
    CU_ASSERT_EQUAL(buffer[2], 0x55);
    CU_ASSERT_EQUAL(buffer[3], 0);

    MEMORY_TRACE_AFTER_EQ;
}

static void test_intToTLV()
{
    MEMORY_TRACE_BEFORE;
    uint8_t buffer[32];
    int result;

    result = lwm2m_intToTLV(LWM2M_TYPE_RESOURCE, 1, 55, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(result, 3);
    CU_ASSERT_EQUAL(buffer[0], 0xC1);
    CU_ASSERT_EQUAL(buffer[1], 55);
    CU_ASSERT_EQUAL(buffer[2], 1);

    result = lwm2m_intToTLV(LWM2M_TYPE_RESOURCE, 0x0102, 0x2255, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(result, 5);
    CU_ASSERT_EQUAL(buffer[0], 0xE2);
    CU_ASSERT_EQUAL(buffer[1], 0x22);
    CU_ASSERT_EQUAL(buffer[2], 0x55);
    CU_ASSERT_EQUAL(buffer[3], 0x01);
    CU_ASSERT_EQUAL(buffer[4], 0x02);

    result = lwm2m_intToTLV(LWM2M_TYPE_RESOURCE, -0x010203, 0x2255, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(result, 7);
    CU_ASSERT_EQUAL(buffer[0], 0xE4);
    CU_ASSERT_EQUAL(buffer[1], 0x22);
    CU_ASSERT_EQUAL(buffer[2], 0x55);
    CU_ASSERT_EQUAL(buffer[3], 0x80);
    CU_ASSERT_EQUAL(buffer[4], 0x01);
    CU_ASSERT_EQUAL(buffer[5], 0x02);
    CU_ASSERT_EQUAL(buffer[6], 0x03);

    result = lwm2m_intToTLV(LWM2M_TYPE_RESOURCE, -0x0102030405060708, 55, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(result, 11);
    CU_ASSERT_EQUAL(buffer[0], 0xC8);
    CU_ASSERT_EQUAL(buffer[1], 55);
    CU_ASSERT_EQUAL(buffer[2], 0x08);
    CU_ASSERT_EQUAL(buffer[3], 0x81);
    CU_ASSERT_EQUAL(buffer[4], 0x02);
    CU_ASSERT_EQUAL(buffer[5], 0x03);
    CU_ASSERT_EQUAL(buffer[6], 0x04);
    CU_ASSERT_EQUAL(buffer[7], 0x05);
    CU_ASSERT_EQUAL(buffer[8], 0x06);
    CU_ASSERT_EQUAL(buffer[9], 0x07);
    CU_ASSERT_EQUAL(buffer[10], 0x08);

    MEMORY_TRACE_AFTER_EQ;
}

static void test_decodeTLV()
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


    result = lwm2m_decodeTLV(data1, sizeof(data1) - 1, &type, &id, &index, &length);
    CU_ASSERT_EQUAL(result, 0);

    result = lwm2m_decodeTLV(data1, sizeof(data1), &type, &id, &index, &length);
    CU_ASSERT_EQUAL(result, 5);
    CU_ASSERT_EQUAL(type, LWM2M_TYPE_RESOURCE);
    CU_ASSERT_EQUAL(id, 55);
    CU_ASSERT_EQUAL(index, 2);
    CU_ASSERT_EQUAL(length, 3);

    result = lwm2m_decodeTLV(data2, sizeof(data2) - 1, &type, &id, &index, &length);
    CU_ASSERT_EQUAL(result, 0);

    result = lwm2m_decodeTLV(data2, sizeof(data2), &type, &id, &index, &length);
    CU_ASSERT_EQUAL(result, 13);
    CU_ASSERT_EQUAL(type, LWM2M_TYPE_OBJECT_INSTANCE);
    CU_ASSERT_EQUAL(id, 0x0203);
    CU_ASSERT_EQUAL(index, 4);
    CU_ASSERT_EQUAL(length, 9);

    result = lwm2m_decodeTLV(data3, sizeof(data3) - 1, &type, &id, &index, &length);
    CU_ASSERT_EQUAL(result, 0);

    result = lwm2m_decodeTLV(data3, sizeof(data3), &type, &id, &index, &length);
    CU_ASSERT_EQUAL(result, 0x194);
    CU_ASSERT_EQUAL(type, LWM2M_TYPE_MULTIPLE_RESOURCE);
    CU_ASSERT_EQUAL(id, 33);
    CU_ASSERT_EQUAL(index, 4);
    CU_ASSERT_EQUAL(length, 0x190);

    MEMORY_TRACE_AFTER_EQ;
}

static void test_opaqueToInt()
{
    MEMORY_TRACE_BEFORE;
    uint8_t data1[] = {1 };
    uint8_t data2[] = {1, 2 };
    uint8_t data3[] = {0x81, 2, 3, 4 };
    uint8_t data4[] = {0x81, 2, 3, 4, 5, 6, 7, 8 };
    int64_t value = 0;
    int result;


    result = lwm2m_opaqueToInt(data1, sizeof(data1), &value);
    CU_ASSERT_EQUAL(result, 1);
    CU_ASSERT_EQUAL(value, 1);

    result = lwm2m_opaqueToInt(data2, sizeof(data2), &value);
    CU_ASSERT_EQUAL(result, 2);
    CU_ASSERT_EQUAL(value, 0x102);

    result = lwm2m_opaqueToInt(data3, sizeof(data3), &value);
    CU_ASSERT_EQUAL(result, 4);
    CU_ASSERT_EQUAL(value, -0x1020304);

    result = lwm2m_opaqueToInt(data4, sizeof(data4), &value);
    CU_ASSERT_EQUAL(result, 8);
    CU_ASSERT_EQUAL(value, -0x102030405060708);

    MEMORY_TRACE_AFTER_EQ;
}

static void test_tlv_parse()
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

    result = lwm2m_data_parse(data1, sizeof(data1), &dataP);
    CU_ASSERT_EQUAL(result, 1);
    CU_ASSERT_PTR_NOT_NULL_FATAL(dataP);
    CU_ASSERT_EQUAL(dataP->type, LWM2M_TYPE_RESOURCE);
    CU_ASSERT_EQUAL(dataP->id, 55);
    CU_ASSERT_EQUAL(dataP->length, 3);
    CU_ASSERT(0 == memcmp(dataP->value, &data1[2], 3));
    lwm2m_data_free(result, dataP);

    result = lwm2m_data_parse(data2, sizeof(data2), &dataP);
    CU_ASSERT_EQUAL(result, 1);
    CU_ASSERT_PTR_NOT_NULL_FATAL(dataP);
    CU_ASSERT_EQUAL(dataP->type, LWM2M_TYPE_OBJECT_INSTANCE);
    CU_ASSERT_EQUAL(dataP->id, 0x203);
    CU_ASSERT_EQUAL(dataP->length, 2);
    CU_ASSERT_PTR_NOT_NULL_FATAL(dataP->value);
    tlvSubP = (lwm2m_data_t *)dataP->value;

    CU_ASSERT_EQUAL(tlvSubP[0].type, LWM2M_TYPE_RESOURCE);
    CU_ASSERT_EQUAL(tlvSubP[0].id, 55);
    CU_ASSERT_EQUAL(tlvSubP[0].length, 3);
    CU_ASSERT(0 == memcmp(tlvSubP[0].value, &data2[6], 3));

    CU_ASSERT_EQUAL(tlvSubP[1].type, LWM2M_TYPE_RESOURCE);
    CU_ASSERT_EQUAL(tlvSubP[1].id, 66);
    CU_ASSERT_EQUAL(tlvSubP[1].length, 9);
    CU_ASSERT(0 == memcmp(tlvSubP[1].value, &data2[12], 9));
    lwm2m_data_free(result, dataP);

    result = lwm2m_data_parse(data3, sizeof(data3), &dataP);
    CU_ASSERT_EQUAL(result, 1);
    CU_ASSERT_PTR_NOT_NULL_FATAL(dataP);
    CU_ASSERT_EQUAL(dataP->type, LWM2M_TYPE_OBJECT_INSTANCE);
    CU_ASSERT_EQUAL(dataP->id, 11);
    CU_ASSERT_EQUAL(dataP->length, 1);
    CU_ASSERT_PTR_NOT_NULL_FATAL(dataP->value);
    tlvSubP = (lwm2m_data_t *)dataP->value;

    CU_ASSERT_EQUAL(tlvSubP[0].type, LWM2M_TYPE_MULTIPLE_RESOURCE);
    CU_ASSERT_EQUAL(tlvSubP[0].id, 77);
    CU_ASSERT_EQUAL(tlvSubP[0].length, 2);
    CU_ASSERT_PTR_NOT_NULL_FATAL(tlvSubP[0].value);
    tlvSubP = (lwm2m_data_t *)tlvSubP[0].value;

    CU_ASSERT_EQUAL(tlvSubP[0].type, LWM2M_TYPE_RESOURCE_INSTANCE);
    CU_ASSERT_EQUAL(tlvSubP[0].id, 0);
    CU_ASSERT_EQUAL(tlvSubP[0].length, 3);
    CU_ASSERT(0 == memcmp(tlvSubP[0].value, &data3[8], 3));

    CU_ASSERT_EQUAL(tlvSubP[1].type, LWM2M_TYPE_RESOURCE_INSTANCE);
    CU_ASSERT_EQUAL(tlvSubP[1].id, 1);
    CU_ASSERT_EQUAL(tlvSubP[1].length, 160);
    CU_ASSERT(0 == memcmp(tlvSubP[1].value, &data3[14], 160));
    lwm2m_data_free(result, dataP);

    MEMORY_TRACE_AFTER_EQ;
}

static void test_tlv_serialize()
{
    MEMORY_TRACE_BEFORE;

    int result;
    lwm2m_data_t *dataP;
    lwm2m_data_t *tlvSubP;
    uint8_t data1[] = {1, 2, 3, 4};
    uint8_t data2[170] = {5, 6, 7, 8};
    uint8_t* buffer;

    dataP =  lwm2m_data_new(1);
    CU_ASSERT_PTR_NOT_NULL_FATAL(dataP);
    dataP->type = LWM2M_TYPE_OBJECT_INSTANCE;
    dataP->id = 3;
    dataP->length = 2;
    tlvSubP =  lwm2m_data_new(2);
    CU_ASSERT_PTR_NOT_NULL_FATAL(tlvSubP);
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
    CU_ASSERT_PTR_NOT_NULL_FATAL(tlvSubP[1].value);
    tlvSubP = (lwm2m_data_t *)tlvSubP[1].value;

    tlvSubP[0].type = LWM2M_TYPE_RESOURCE_INSTANCE;
    tlvSubP[0].flags = LWM2M_TLV_FLAG_STATIC_DATA;
    tlvSubP[0].id = 0;
    tlvSubP[0].length = sizeof(data2);
    tlvSubP[0].value = data2;

    result = lwm2m_data_serialize(1, dataP, &buffer);
    CU_ASSERT_EQUAL(result, sizeof(data2) + sizeof(data1) + 11);

    CU_ASSERT_EQUAL(buffer[0], 0x08);
    CU_ASSERT_EQUAL(buffer[1], 3);
    CU_ASSERT_EQUAL(buffer[2], sizeof(data2) + sizeof(data1) + 8);

    CU_ASSERT_EQUAL(buffer[3], 0xC0 + sizeof(data1));
    CU_ASSERT_EQUAL(buffer[4], 66);
    CU_ASSERT(0 == memcmp(data1, &buffer[5], sizeof(data1)));

    CU_ASSERT_EQUAL(buffer[5 + sizeof(data1)], 0x88);
    CU_ASSERT_EQUAL(buffer[6 + sizeof(data1)], 77);
    CU_ASSERT_EQUAL(buffer[7 + sizeof(data1)], sizeof(data2) + 3);

    CU_ASSERT_EQUAL(buffer[8 + sizeof(data1)], 0x48);
    CU_ASSERT_EQUAL(buffer[9 + sizeof(data1)], 0);
    CU_ASSERT_EQUAL(buffer[10 + sizeof(data1)], sizeof(data2));
    CU_ASSERT(0 == memcmp(data2, &buffer[11 + sizeof(data1)], sizeof(data2)));

    lwm2m_data_free(1, dataP);
    lwm2m_free(buffer);

    MEMORY_TRACE_AFTER_EQ;
}

static void test_tlv_encode_int(void)
{
   MEMORY_TRACE_BEFORE;
   uint8_t data1[] = { 0x92, 0x34 };
   uint8_t data2[] = { 0x7f, 0x34, 0x56, 0x78, 0x91, 0x22, 0x33, 0x44 };
   lwm2m_data_t *dataP =  lwm2m_data_new(10);
   CU_ASSERT_PTR_NOT_NULL(dataP);

   lwm2m_data_encode_int(0x12, dataP);
   dataP[0].type = LWM2M_TYPE_RESOURCE;
   CU_ASSERT_EQUAL(dataP[0].flags, 0);
   CU_ASSERT_EQUAL(dataP[0].length, 1);
   CU_ASSERT_PTR_NOT_NULL_FATAL(dataP[0].value);
   CU_ASSERT_EQUAL(dataP[0].value[0], 0x12);

   dataP[1].flags = LWM2M_TLV_FLAG_TEXT_FORMAT;
   lwm2m_data_encode_int(18, &dataP[1]);
   dataP[1].type = LWM2M_TYPE_RESOURCE;
   CU_ASSERT_EQUAL(dataP[1].length, 2);
   CU_ASSERT_PTR_NOT_NULL_FATAL(dataP[1].value);
   CU_ASSERT(0 == memcmp(dataP[1].value, "18", 2));

   lwm2m_data_encode_int(-0x1234, &dataP[2]);
   dataP[2].type = LWM2M_TYPE_RESOURCE;
   CU_ASSERT_EQUAL(dataP[2].length, 2);
   CU_ASSERT_PTR_NOT_NULL_FATAL(dataP[2].value);
   CU_ASSERT(0 == memcmp(dataP[2].value, data1, 2));

   dataP[3].flags = LWM2M_TLV_FLAG_TEXT_FORMAT;
   lwm2m_data_encode_int(-14678, &dataP[3]);
   dataP[3].type = LWM2M_TYPE_RESOURCE;
   CU_ASSERT_EQUAL(dataP[3].length, 6);
   CU_ASSERT_PTR_NOT_NULL_FATAL(dataP[3].value);
   CU_ASSERT(0 == memcmp(dataP[3].value, "-14678", 6));

   lwm2m_data_encode_int(0x7f34567891223344, &dataP[4]);
   dataP[4].type = LWM2M_TYPE_RESOURCE;
   CU_ASSERT_EQUAL(dataP[4].length, 8);
   CU_ASSERT_PTR_NOT_NULL_FATAL(dataP[4].value);
   CU_ASSERT(0 == memcmp(dataP[4].value, data2, 8));

   lwm2m_data_free(10, dataP);
   MEMORY_TRACE_AFTER_EQ;
}

static void test_tlv_decode_int(void)
{
   MEMORY_TRACE_BEFORE;
   uint8_t data1[] = { 0x12, 0x34 };
   uint8_t data2[] = { 0x92, 0x34 };
   uint8_t data3[] = { 0x7f, 0x34, 0x56, 0x78, 0x91, 0x22, 0x33, 0x44 };
   int64_t value;
   int result;
   lwm2m_data_t *dataP =  lwm2m_data_new(10);
   CU_ASSERT_PTR_NOT_NULL(dataP);

   result = lwm2m_data_decode_int(dataP, &value);
   CU_ASSERT_EQUAL(result, 0);

   dataP[0].flags = LWM2M_TLV_FLAG_STATIC_DATA;
   dataP[0].length = 1;
   dataP[0].value = data1;
   result = lwm2m_data_decode_int(dataP, &value);
   CU_ASSERT_EQUAL(result, 1);
   CU_ASSERT_EQUAL(value, 0x12);

   dataP[0].length = 2;
   dataP[0].value = data2;
   result = lwm2m_data_decode_int(dataP, &value);
   CU_ASSERT_EQUAL(result, 1);
   CU_ASSERT_EQUAL(value, -0x1234);

   dataP[0].length = 8;
   dataP[0].value = data3;
   result = lwm2m_data_decode_int(dataP, &value);
   CU_ASSERT_EQUAL(result, 1);
   CU_ASSERT_EQUAL(value, 0x7f34567891223344);

   dataP[0].length = 9;
   result = lwm2m_data_decode_int(dataP, &value);
   CU_ASSERT_EQUAL(result, 0);

   dataP[0].flags |= LWM2M_TLV_FLAG_TEXT_FORMAT;
   dataP[0].length = 2;
   dataP[0].value = (uint8_t *) "18";
   result = lwm2m_data_decode_int(dataP, &value);
   CU_ASSERT_EQUAL(result, 1);
   CU_ASSERT_EQUAL(value, 18);

   dataP[0].length = 9;
   dataP[0].value = (uint8_t *) "-56923456";
   result = lwm2m_data_decode_int(dataP, &value);
   CU_ASSERT_EQUAL(result, 1);
   CU_ASSERT_EQUAL(value, -56923456);

   lwm2m_data_free(10, dataP);
   MEMORY_TRACE_AFTER_EQ;
}

static void test_tlv_encode_bool(void)
{
   MEMORY_TRACE_BEFORE;
   lwm2m_data_t *dataP =  lwm2m_data_new(10);
   CU_ASSERT_PTR_NOT_NULL(dataP);

   lwm2m_data_encode_bool(2, dataP);
   dataP[0].type = LWM2M_TYPE_RESOURCE;
   CU_ASSERT_EQUAL(dataP[0].flags, 0);
   CU_ASSERT_EQUAL(dataP[0].length, 1);
   CU_ASSERT_PTR_NOT_NULL_FATAL(dataP[0].value);
   CU_ASSERT_EQUAL(dataP[0].value[0], 1);

   lwm2m_data_encode_bool(0, &dataP[1]);
   dataP[1].type = LWM2M_TYPE_RESOURCE;
   CU_ASSERT_EQUAL(dataP[1].flags, 0);
   CU_ASSERT_EQUAL(dataP[1].length, 1);
   CU_ASSERT_PTR_NOT_NULL_FATAL(dataP[1].value);
   CU_ASSERT_EQUAL(dataP[1].value[0], 0);

   dataP[2].flags = LWM2M_TLV_FLAG_TEXT_FORMAT;
   lwm2m_data_encode_bool(0, &dataP[2]);
   dataP[2].type = LWM2M_TYPE_RESOURCE;
   CU_ASSERT_EQUAL(dataP[2].flags, LWM2M_TLV_FLAG_TEXT_FORMAT);
   CU_ASSERT_EQUAL(dataP[2].length, 1);
   CU_ASSERT_PTR_NOT_NULL_FATAL(dataP[2].value);
   CU_ASSERT_EQUAL(dataP[2].value[0], '0');

   dataP[3].flags = LWM2M_TLV_FLAG_TEXT_FORMAT;
   lwm2m_data_encode_bool(4, &dataP[3]);
   dataP[3].type = LWM2M_TYPE_RESOURCE;
   CU_ASSERT_EQUAL(dataP[3].flags, LWM2M_TLV_FLAG_TEXT_FORMAT);
   CU_ASSERT_EQUAL(dataP[3].length, 1);
   CU_ASSERT_PTR_NOT_NULL_FATAL(dataP[3].value);
   CU_ASSERT_EQUAL(dataP[3].value[0], '1');

   lwm2m_data_free(10, dataP);
   MEMORY_TRACE_AFTER_EQ;
}

static void test_tlv_decode_bool(void)
{
   MEMORY_TRACE_BEFORE;
   uint8_t data1[] = { 0 };
   uint8_t data2[] = { 1 };
   uint8_t data3[] = { 2 };
   bool value;
   int result;
   lwm2m_data_t *dataP =  lwm2m_data_new(10);
   CU_ASSERT_PTR_NOT_NULL(dataP);

   result = lwm2m_data_decode_bool(dataP, &value);
   CU_ASSERT_EQUAL(result, 0);

   dataP[0].length = 2;
   result = lwm2m_data_decode_bool(dataP, &value);
   CU_ASSERT_EQUAL(result, 0);

   dataP[0].flags = LWM2M_TLV_FLAG_STATIC_DATA;
   dataP[0].length = 1;
   dataP[0].value = data1;
   result = lwm2m_data_decode_bool(dataP, &value);
   CU_ASSERT_EQUAL(result, 1);
   CU_ASSERT_FALSE(value);

   dataP[0].value = data2;
   result = lwm2m_data_decode_bool(dataP, &value);
   CU_ASSERT_EQUAL(result, 1);
   CU_ASSERT_TRUE(value);

   dataP[0].value = data3;
   result = lwm2m_data_decode_bool(dataP, &value);
   CU_ASSERT_EQUAL(result, 0);

   dataP[0].flags |= LWM2M_TLV_FLAG_TEXT_FORMAT;
   dataP[0].value = (uint8_t *)"0";
   result = lwm2m_data_decode_bool(dataP, &value);
   CU_ASSERT_EQUAL(result, 1);
   CU_ASSERT_FALSE(value);

   dataP[0].value = (uint8_t *)"1";
   result = lwm2m_data_decode_bool(dataP, &value);
   CU_ASSERT_EQUAL(result, 1);
   CU_ASSERT_TRUE(value);

   dataP[0].value = (uint8_t *)"2";
   result = lwm2m_data_decode_bool(dataP, &value);
   CU_ASSERT_EQUAL(result, 0);

   lwm2m_data_free(10, dataP);
   MEMORY_TRACE_AFTER_EQ;
}

static struct TestTable table[] = {
        { "test of lwm2m_data_new()", test_tlv_new },
        { "test of lwm2m_data_free()", test_tlv_free },
        { "test of lwm2m_opaqueToTLV()", test_opaqueToTLV },
        { "test of lwm2m_boolToTLV()", test_boolToTLV },
        { "test of lwm2m_intToTLV()", test_intToTLV },
        { "test of lwm2m_decodeTLV()", test_decodeTLV },
        { "test of lwm2m_opaqueToInt()", test_opaqueToInt },
        { "test of lwm2m_data_parse()", test_tlv_parse },
        { "test of lwm2m_data_serialize()", test_tlv_serialize },
        { "test of lwm2m_data_encode_int()", test_tlv_encode_int },
        { "test of lwm2m_data_decode_int()", test_tlv_decode_int },
        { "test of lwm2m_data_encode_bool()", test_tlv_encode_bool },
        { "test of lwm2m_data_decode_bool()", test_tlv_decode_bool },
        { NULL, NULL },
};

CU_ErrorCode create_tlv_suit()
{
   CU_pSuite pSuite = NULL;

   pSuite = CU_add_suite("Suite_TLV", NULL, NULL);
   if (NULL == pSuite) {
      return CU_get_error();
   }

   return add_tests(pSuite, table);
}



