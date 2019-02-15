/*******************************************************************************
 *
 * Copyright (c) 2013, 2014, 2015 Intel Corporation and others.
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
 *    David Gräff - Convert to test case
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *    
 *******************************************************************************/

#include "liblwm2m.h"
#include "internals.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#include "commandline.h"

#include "tests.h"
#include "CUnit/Basic.h"

#ifdef LWM2M_SUPPORT_SENML_JSON

#ifndef STR_MEDIA_TYPE
#define STR_MEDIA_TYPE(M)                          \
((M) == LWM2M_CONTENT_TEXT ? "Text" :              \
((M) == LWM2M_CONTENT_LINK ? "Link" :              \
((M) == LWM2M_CONTENT_OPAQUE ? "Opaque" :          \
((M) == LWM2M_CONTENT_TLV ? "TLV" :                \
((M) == LWM2M_CONTENT_JSON ? "JSON" :              \
((M) == LWM2M_CONTENT_SENML_JSON ? "SenML JSON" :  \
"Unknown"))))))
#endif

static void senml_json_test_data(const char * uriStr,
                        lwm2m_media_type_t format,
                        lwm2m_data_t * tlvP,
                        int size,
                        const char * id)
{
    lwm2m_uri_t uri;
    uint8_t * buffer;
    int length;
    
    if (uriStr != NULL)
    {
        lwm2m_stringToUri(uriStr, strlen(uriStr), &uri);
    }

    if (tlvP[0].type != LWM2M_TYPE_OBJECT)
    {
        length = lwm2m_data_serialize((uriStr != NULL) ? &uri : NULL, size, tlvP, &format, &buffer);
        if (length <= 0)
        {
            printf("(Serialize lwm2m_data_t %s to %s failed.)\t", id, STR_MEDIA_TYPE(format));
            //dump_data_t(stdout, size, tlvP, 0);
            CU_TEST_FATAL(CU_FALSE);
        }
        else
        {
            lwm2m_free(buffer);
        }
    }
}

static void senml_json_test_data_and_compare(const char * uriStr,
                        lwm2m_media_type_t format,
                        lwm2m_data_t * tlvP,
                        int size,
                        const char * id,
                        const uint8_t* original_buffer,
                        size_t original_length)
{
    lwm2m_uri_t uri;
    uint8_t * buffer;
    int length;
    
    if (uriStr != NULL)
    {
        lwm2m_stringToUri(uriStr, strlen(uriStr), &uri);
    }

    length = lwm2m_data_serialize((uriStr != NULL) ? &uri : NULL, size, tlvP, &format, &buffer);
    if (length <= 0)
    {
        printf("(Serialize lwm2m_data_t %s to %s failed.)\t", id, STR_MEDIA_TYPE(format));
        //dump_data_t(stdout, size, tlvP, 0);
        CU_TEST_FATAL(CU_FALSE);
        return;
    }

    if (format != LWM2M_CONTENT_JSON && format != LWM2M_CONTENT_SENML_JSON)
    {
        CU_ASSERT_EQUAL(original_length, length);

        if ((original_length != (size_t)length) ||
            (memcmp(original_buffer, buffer, length) != 0))
        {
            printf("Comparing buffer after parse/serialize failed for %s:\n", id);
            output_buffer(stdout, buffer, length, 0);
            printf("\ninstead of:\n");
            output_buffer(stdout, original_buffer, original_length, 0);
            CU_TEST_FATAL(CU_FALSE);
        }
    }
    else
    {
        /* Compare ignoring all whitespace and 0s.
         *
         * Ideally whitespace within quotes would be considered, but that would
         * complicate the comparison.
         *
         * 0s are ignored as they may be extra trailing 0s in floating point
         * values.
         */
#define IGNORED(x) ((x) == '0' || isspace(x))
        size_t i = 0;
        int j = 0;
        while (i < original_length && IGNORED(original_buffer[i])) i++;
        while (j < length && IGNORED(buffer[j])) j++;
        while (i < original_length && j < length)
        {
            if(original_buffer[i] != buffer[j])
            {
                printf("Comparing buffer after parse/serialize failed for %s:\n", id);
                fprintf(stdout, "%.*s\n", length, buffer);
                printf("\ninstead of:\n");
                fprintf(stdout, "%.*s\n", (int)original_length, original_buffer);
                CU_FAIL("Comparing buffer after parse/serialize failed");
                break;
            }
            i++;
            j++;
            while (i < original_length && IGNORED(original_buffer[i])) i++;
            while (j < length && IGNORED(buffer[j])) j++;
        }
#undef IGNORED
    }

    lwm2m_free(buffer);
}

/**
 * @brief Parses the testBuf to an array of lwm2m_data_t objects and serializes the result
 *        to TLV and JSON and if applicable compares it to the original testBuf.
 * @param testBuf The input buffer.
 * @param testLen The length of the input buffer.
 * @param format The format of the testBuf. Maybe LWM2M_CONTENT_TLV or LWM2M_CONTENT_JSON at the moment.
 * @param id The test object id for debug out.
 */
static void senml_json_test_raw(const char * uriStr,
                     const uint8_t * testBuf,
                     size_t testLen,
                     lwm2m_media_type_t format,
                     const char * id)
{
    lwm2m_data_t * tlvP;
    lwm2m_uri_t uri;
    int size;

    if (uriStr != NULL)
    {
        lwm2m_stringToUri(uriStr, strlen(uriStr), &uri);
    }

    size = lwm2m_data_parse((uriStr != NULL) ? &uri : NULL, testBuf, testLen, format, &tlvP);
    if (size < 0)
    {
        printf("(Parsing %s from %s failed.)\t", id, STR_MEDIA_TYPE(format));
    }
    CU_ASSERT_TRUE_FATAL(size>0);

    // Serialize to the same format and compare to the input buffer
    senml_json_test_data_and_compare(uriStr, format, tlvP, size, id, testBuf, testLen);

    // Serialize to the TLV format
    // the reverse is not possible as TLV format loses the data type information
    if (format == LWM2M_CONTENT_JSON)
    {
        senml_json_test_data(uriStr, LWM2M_CONTENT_TLV, tlvP, size, id);
    }
    else if (format == LWM2M_CONTENT_SENML_JSON)
    {
        senml_json_test_data(uriStr, LWM2M_CONTENT_TLV, tlvP, size, id);
    }
}

static void senml_json_test_raw_expected(const char * uriStr,
                              const uint8_t * testBuf,
                              size_t testLen,
                              const char * expectBuf,
                              size_t expectLen,
                              lwm2m_media_type_t format,
                              const char * id)
{
    lwm2m_data_t * tlvP;
    lwm2m_uri_t uri;
    int size;
    
    if (uriStr != NULL)
    {
        lwm2m_stringToUri(uriStr, strlen(uriStr), &uri);
    }

    size = lwm2m_data_parse((uriStr != NULL) ? &uri : NULL, testBuf, testLen, format, &tlvP);
    CU_ASSERT_TRUE_FATAL(size>0);

    // Serialize to the same format and compare to the input buffer
    senml_json_test_data_and_compare(uriStr, format, tlvP, size, id, (uint8_t*)expectBuf, expectLen);

    // Serialize to the other format respectively.
    if (format == LWM2M_CONTENT_TLV)
        senml_json_test_data(uriStr, LWM2M_CONTENT_JSON, tlvP, size, id);
    else if (format == LWM2M_CONTENT_JSON)
        senml_json_test_data(uriStr, LWM2M_CONTENT_TLV, tlvP, size, id);
    else if (format == LWM2M_CONTENT_SENML_JSON)
        senml_json_test_data(uriStr, LWM2M_CONTENT_TLV, tlvP, size, id);
}

static void senml_json_test_raw_error(const char * uriStr,
                                      const uint8_t * testBuf,
                                      size_t testLen,
                                      lwm2m_media_type_t format,
                                      const char * id)
{
    lwm2m_data_t * tlvP;
    lwm2m_uri_t uri;
    int size;

    if (uriStr != NULL)
    {
        lwm2m_stringToUri(uriStr, strlen(uriStr), &uri);
    }

    size = lwm2m_data_parse((uriStr != NULL) ? &uri : NULL, testBuf, testLen, format, &tlvP);
    if (size > 0)
    {
        printf("(Parsing %s from %s succeeded but should have failed.)\t", id, STR_MEDIA_TYPE(format));
    }
    CU_ASSERT_TRUE_FATAL(size<0);
}

static void senml_json_test_1(void)
{
    // From Table 7.4.4-2 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    const char * buffer = "[{\"bn\":\"/3/0/\",\"n\":\"0\",\"vs\":\"Open Mobile Alliance\"}," \
                          "{\"n\":\"1\",\"vs\":\"Lightweight M2M Client\"}," \
                          "{\"n\":\"2\",\"vs\":\"345000123\"}," \
                          "{\"n\":\"3\",\"vs\":\"1.0\"}," \
                          "{\"n\":\"6/0\",\"v\":1}," \
                          "{\"n\":\"6/1\",\"v\":5}," \
                          "{\"n\":\"7/0\",\"v\":3800}," \
                          "{\"n\":\"7/1\",\"v\":5000}," \
                          "{\"n\":\"8/0\",\"v\":125}," \
                          "{\"n\":\"8/1\",\"v\":900}," \
                          "{\"n\":\"9\",\"v\":100}," \
                          "{\"n\":\"10\",\"v\":15}," \
                          "{\"n\":\"11/0\",\"v\":0}," \
                          "{\"n\":\"13\",\"v\":1367491215}," \
                          "{\"n\":\"14\",\"vs\":\"+02:00\"}," \
                          "{\"n\":\"16\",\"vs\":\"U\"}]";
    senml_json_test_raw("/3/0", (uint8_t *)buffer, strlen(buffer), LWM2M_CONTENT_SENML_JSON, "1");
}

static void senml_json_test_2(void)
{
    // From Table 7.4.4-3 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    const char * buffer = "[{\"bn\":\"/72/\",\"bt\":25462634,\"n\":\"1/2\",\"v\":22.4,\"t\":-5}," \
                          "{\"n\":\"1/2\",\"v\":22.9,\"t\":-30}," \
                          "{\"n\":\"1/2\",\"v\":24.1,\"t\":-50}]";
#if 0
    // TODO: This should pass once time is supported
    senml_json_test_raw("/72", (uint8_t *)buffer, strlen(buffer), LWM2M_CONTENT_SENML_JSON, "2");
#else
    const char *expect = "[{\"bn\":\"/72/\",\"n\":\"1/2\",\"v\":24.1}]";
    senml_json_test_raw_expected("/72", (uint8_t *)buffer, strlen(buffer), expect, strlen(expect), LWM2M_CONTENT_SENML_JSON, "2");
#endif
}

static void senml_json_test_3(void)
{
    // From Table 7.4.4-4 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    // Corrected /66/1/2 from hex to decimal
    const char * buffer = "[{\"bn\":\"/\",\"n\":\"65/0/0/0\",\"vlo\":\"66:0\"}," \
                          "{\"n\":\"65/0/0/1\",\"vlo\":\"66:1\"}," \
                          "{\"n\":\"65/0/1\",\"vs\":\"8613800755500\"}," \
                          "{\"n\":\"65/0/2\",\"v\":1}," \
                          "{\"n\":\"66/0/0\",\"vs\":\"myService1\"}," \
                          "{\"n\":\"66/0/1\",\"vs\":\"Internet.15.234\"}," \
                          "{\"n\":\"66/0/2\",\"vlo\":\"67:0\"}," \
                          "{\"n\":\"66/1/0\",\"vs\":\"myService2\"}," \
                          "{\"n\":\"66/1/1\",\"vs\":\"Internet.15.235\"}," \
                          "{\"n\":\"66/1/2\",\"vlo\":\"65535:65535\"}," \
                          "{\"n\":\"67/0/0\",\"vs\":\"85.76.76.84\"}," \
                          "{\"n\":\"67/0/1\",\"vs\":\"85.76.255.255\"}]";
    senml_json_test_raw("/", (uint8_t *)buffer, strlen(buffer), LWM2M_CONTENT_SENML_JSON, "3");
}

static void senml_json_test_4(void)
{
    // From Table 7.4.4-5 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    // Corrected for missing closing quote.
    const char * buffer = "[{\"bn\":\"/3/0/0\",\"vs\":\"Open Mobile Alliance\"}]";
    senml_json_test_raw("/3/0/0", (uint8_t *)buffer, strlen(buffer), LWM2M_CONTENT_SENML_JSON, "4");
}

static void senml_json_test_5(void)
{
    // From Table 7.4.4-6 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    const char * buffer = "[{\"n\":\"/3/0/0\"}," \
                          "{\"n\":\"/3/0/9\"}," \
                          "{\"n\":\"/1/0/1\"}]";
    senml_json_test_raw(NULL, (uint8_t *)buffer, strlen(buffer), LWM2M_CONTENT_SENML_JSON, "5");
}

static void senml_json_test_6(void)
{
    // From Table 7.4.4-7 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    const char * buffer = "[{\"n\":\"/3/0/0\", \"vs\":\"Open Mobile Alliance\"}," \
                          "{\"n\" :\"/3/0/9\", \"v\":95}," \
                          "{\"n\":\"/1/0/1\", \"v\":86400}]";
    senml_json_test_raw(NULL, (uint8_t *)buffer, strlen(buffer), LWM2M_CONTENT_SENML_JSON, "6");
}

static void senml_json_test_7(void)
{
    // From Table 7.4.4-8 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    const char * buffer = "[{\"n\":\"/3311/0/5850\", \"vb\":false}," \
                          "{\"n\":\"/3311/1/5850\", \"vb\":false}," \
                          "{\"n\":\"/3311/2/5851\", \"v\":20}," \
                          "{\"n\":\"/3308/0/5900\", \"v\":18}]";
    senml_json_test_raw(NULL, (uint8_t *)buffer, strlen(buffer), LWM2M_CONTENT_SENML_JSON, "7");
}

static void senml_json_test_8(void)
{
    const char * buffer = "[{\"bn\" : \"/65/543/\",                \
                             \"n\" :   \"0\",              \
                            \"vs\":\"LWM2M\"},              \
                         {\"v\":2015, \"n\":\"1\"},         \
                         {\"n\":\"2/0\",    \"vb\":true},   \
                         {\"vb\": false, \"n\":\"2/1\"}]    \
                      ";
    // We do a string comparison, therefore first n and then v in this order have to appear,
    // because that is the order that serialize will output.
    const char * expect = "[{\"bn\":\"/65/543/\",\"n\":\"0\",\"vs\":\"LWM2M\"}," \
                          "{\"n\":\"1\",\"v\":2015},"                            \
                          "{\"n\":\"2/0\",\"vb\":true},"                         \
                          "{\"n\":\"2/1\",\"vb\":false}]";
    senml_json_test_raw_expected("/65/543", (uint8_t *)buffer, strlen(buffer), expect, strlen(expect), LWM2M_CONTENT_SENML_JSON, "8");
}

static void senml_json_test_9(void)
{
    const char * buffer = "[{\"bn\" : \"/2/3\", \
                            \"n\":\"/0\",\"v\":1234}]";
    const char * expect = "[{\"bn\":\"/2/3/\",\"n\":\"0\",\"v\":1234}]";

    senml_json_test_raw_expected("/2/3", (uint8_t *)buffer, strlen(buffer), expect, strlen(expect), LWM2M_CONTENT_SENML_JSON, "9");
}

static void senml_json_test_10(void)
{
    const char * buffer = "[{\"n\":\"1\",\"v\":1234,\"bn\" : \"/12/0/\",\"bt\" : 1234567},          \
                         {\"n\":\"3\",\"v\":56.789},        \
                         {\"n\":\"2/0\",\"v\":0.23},        \
                         {\"n\":\"2/1\",\"v\":-52.0006}]";

    // We do a string comparison. Because parsing+serialization changes double value
    // precision, we expect a slightly different output than input.
    const char * expectA = "[{\"n\":\"/12/0/1\",\"v\":1234},{\"n\":\"/12/0/3\",\"v\":56.789},{\"n\":\"/12/0/2/0\",\"v\":0.23},{\"n\":\"/12/0/2/1\",\"v\":-52.0006}]";
    const char * expectB = "[{\"bn\":\"/\",\"n\":\"12/0/1\",\"v\":1234},{\"n\":\"12/0/3\",\"v\":56.789},{\"n\":\"12/0/2/0\",\"v\":0.23},{\"n\":\"12/0/2/1\",\"v\":-52.0006}]";
    const char * expectC = "[{\"bn\":\"/12/\",\"n\":\"0/1\",\"v\":1234},{\"n\":\"0/3\",\"v\":56.789},{\"n\":\"0/2/0\",\"v\":0.23},{\"n\":\"0/2/1\",\"v\":-52.0006}]";
    const char * expectD = "[{\"bn\":\"/12/0/\",\"n\":\"1\",\"v\":1234},{\"n\":\"3\",\"v\":56.789},{\"n\":\"2/0\",\"v\":0.23},{\"n\":\"2/1\",\"v\":-52.0006}]";
    const char * expectE = "[{\"bn\":\"/12/0/3\",\"v\":56.789}]";

    senml_json_test_raw_expected(NULL, (uint8_t *)buffer, strlen(buffer), expectA, strlen(expectA), LWM2M_CONTENT_SENML_JSON, "10a");
    senml_json_test_raw_expected("/", (uint8_t *)buffer, strlen(buffer), expectB, strlen(expectB), LWM2M_CONTENT_SENML_JSON, "10b");
    senml_json_test_raw_expected("/12", (uint8_t *)buffer, strlen(buffer), expectC, strlen(expectC), LWM2M_CONTENT_SENML_JSON, "10c");
    senml_json_test_raw_expected("/12/0", (uint8_t *)buffer, strlen(buffer), expectD, strlen(expectD), LWM2M_CONTENT_SENML_JSON, "10d");
    senml_json_test_raw_expected("/12/0/3", (uint8_t *)buffer, strlen(buffer), expectE, strlen(expectE), LWM2M_CONTENT_SENML_JSON, "10e");
}

static void senml_json_test_11(void)
{
    const char * buffer = "[{ \"bn\":\"/\",\"n\":\"34/0/1\",\"vs\":\"8613800755500\"},      \
                          {\"n\":\"34/0/2\",\"v\":1},                       \
                          {\"n\":\"66/0/0\",\"vs\":\"myService1\"},         \
                          {\"n\":\"66/0/1\",\"vs\":\"Internet.15.234\"},    \
                          {\"n\":\"66/1/0\",\"vs\":\"myService2\"},         \
                          {\"n\":\"66/1/1\",\"vs\":\"Internet.15.235\"},    \
                          {\"n\":\"31/0/0\",\"vs\":\"85.76.76.84\"},        \
                          {\"n\":\"31/0/1\",\"vs\":\"85.76.255.255\"}]";

    senml_json_test_raw("/", (uint8_t *)buffer, strlen(buffer), LWM2M_CONTENT_SENML_JSON, "11");
}

static void senml_json_test_12(void)
{
    const char * buffer = "[{ \"bn\":\"/\",\"n\":\"34/0/0/0\",\"vlo\":\"66:0\"},             \
                          {\"n\":\"34/0/0/1\",\"vlo\":\"66:1\"},             \
                          {\"n\":\"34/0/1\",\"vs\":\"8613800755500\"},      \
                          {\"n\":\"34/0/2\",\"v\":1},                       \
                          {\"n\":\"66/0/0\",\"vs\":\"myService1\"},         \
                          {\"n\":\"66/0/1\",\"vs\":\"Internet.15.234\"},    \
                          {\"n\":\"66/0/2\",\"vlo\":\"31:0\"},               \
                          {\"n\":\"66/1/0\",\"vs\":\"myService2\"},         \
                          {\"n\":\"66/1/1\",\"vs\":\"Internet.15.235\"},    \
                          {\"n\":\"66/1/2\",\"vlo\":\"65535:65535\"},        \
                          {\"n\":\"31/0/0\",\"vs\":\"85.76.76.84\"},        \
                          {\"n\":\"31/0/1\",\"vs\":\"85.76.255.255\"}]";

    senml_json_test_raw("/", (uint8_t *)buffer, strlen(buffer), LWM2M_CONTENT_SENML_JSON, "12");
}

static void senml_json_test_13(void)
{
    lwm2m_data_t * data1 = lwm2m_data_new(17);
    int i;

    for (i = 0; i < 17; i++)
    {
        data1[i].id = i;
    }

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

    senml_json_test_data("/12/0", LWM2M_CONTENT_SENML_JSON, data1, 17, "13");
}

static void senml_json_test_14(void)
{
    const char * buffer = "[{\"bn\":\"/5/0/1\",              \
                           \"n\":\"/1\",\"vs\":\"http\"}]";
#if 1
    const char * expect = "[{\"bn\":\"/5/0/1/\",\"n\":\"1\",\"vs\":\"http\"}]";

    senml_json_test_raw_expected("/5/0/1", (uint8_t *)buffer, strlen(buffer), expect, strlen(expect), LWM2M_CONTENT_SENML_JSON, "14");
#else
    const char * expect = "[{\"bn\":\"/5/0/1/1\",\"vs\":\"http\"}]";

    senml_json_test_raw_expected("/5/0/1/1", (uint8_t *)buffer, strlen(buffer), expect, strlen(expect), LWM2M_CONTENT_SENML_JSON, "14");
#endif
}

static void senml_json_test_15(void)
{
    const char * buffer = "[{\"bn\":\"/5/0\",                \
                           \"n\":\"/1\",\"vs\":\"http\"}]";
    const char * expect = "[{\"bn\":\"/5/0/\",\"n\":\"1\",\"vs\":\"http\"}]";

    senml_json_test_raw_expected("/5/0", (uint8_t *)buffer, strlen(buffer), expect, strlen(expect), LWM2M_CONTENT_SENML_JSON, "15");
}

static void senml_json_test_16(void)
{
    const char * buffer = "[{\"bn\":\"/5/0/1\",                \
                           \"n\":\"\",\"vs\":\"http\"}]";
    const char * expect = "[{\"bn\":\"/5/0/1\",\"vs\":\"http\"}]";

    senml_json_test_raw_expected("/5/0/1", (uint8_t *)buffer, strlen(buffer), expect, strlen(expect), LWM2M_CONTENT_SENML_JSON, "16");
}

static void senml_json_test_17(void)
{
    // Adapted from Table 7.4.4-4 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    const char * buffer = "[{\"bn\":\"/65/0/\",\"n\":\"0/0\",\"vlo\":\"66:0\"}," \
                          "{\"n\":\"0/1\",\"vlo\":\"66:1\"}," \
                          "{\"n\":\"1\",\"vs\":\"8613800755500\"}," \
                          "{\"n\":\"2\",\"v\":1}," \
                          "{\"bn\":\"/66/0/\",\"n\":\"0\",\"vs\":\"myService1\"}," \
                          "{\"n\":\"1\",\"vs\":\"Internet.15.234\"}," \
                          "{\"n\":\"2\",\"vlo\":\"67:0\"}," \
                          "{\"bn\":\"/66/1/\",\"n\":\"0\",\"vs\":\"myService2\"}," \
                          "{\"n\":\"1\",\"vs\":\"Internet.15.235\"}," \
                          "{\"n\":\"2\",\"vlo\":\"65535:65535\"}," \
                          "{\"bn\":\"\",\"n\":\"/67/0/00\",\"vs\":\"85.76.76.84\"}," \
                          "{\"n\":\"/67/0/1\",\"vs\":\"85.76.255.255\"}]";
    // We do a string comparison, therefore first n and then v in this order have to appear,
    // because that is the order that serialize will output.
    const char * expect = "[{\"n\":\"/65/0/0/0\",\"vlo\":\"66:0\"}," \
                          "{\"n\":\"/65/0/0/1\",\"vlo\":\"66:1\"}," \
                          "{\"n\":\"/65/0/1\",\"vs\":\"8613800755500\"}," \
                          "{\"n\":\"/65/0/2\",\"v\":1}," \
                          "{\"n\":\"/66/0/0\",\"vs\":\"myService1\"}," \
                          "{\"n\":\"/66/0/1\",\"vs\":\"Internet.15.234\"}," \
                          "{\"n\":\"/66/0/2\",\"vlo\":\"67:0\"}," \
                          "{\"n\":\"/66/1/0\",\"vs\":\"myService2\"}," \
                          "{\"n\":\"/66/1/1\",\"vs\":\"Internet.15.235\"}," \
                          "{\"n\":\"/66/1/2\",\"vlo\":\"65535:65535\"}," \
                          "{\"n\":\"/67/0/0\",\"vs\":\"85.76.76.84\"}," \
                          "{\"n\":\"/67/0/1\",\"vs\":\"85.76.255.255\"}]";
    senml_json_test_raw_expected(NULL, (uint8_t *)buffer, strlen(buffer), expect, strlen(expect), LWM2M_CONTENT_SENML_JSON, "17");
}

static void senml_json_test_18(void)
{
    /* Explicit supported base version */
    const char * buffer = "[{\"bver\":10,\"n\":\"/34/0/2\",\"v\":1}]";
    const char * expect = "[{\"n\":\"/34/0/2\",\"v\":1}]";
    senml_json_test_raw_expected(NULL, (uint8_t *)buffer, strlen(buffer), expect, strlen(expect), LWM2M_CONTENT_SENML_JSON, "18");
}

static void senml_json_test_19(void)
{
    /* Unsupported base version */
    const char * buffer = "[{\"bver\":11,\"n\":\"/34/0/2\",\"v\":1}]";
    senml_json_test_raw_error(NULL, (uint8_t *)buffer, strlen(buffer), LWM2M_CONTENT_SENML_JSON, "19");
}

static void senml_json_test_20(void)
{
    /* Unsupported fields not causing error */
    const char * buffer = "[{\"test\":\"good\",\"n\":\"/34/0/2\",\"v\":1}]";
    const char * expect = "[{\"bn\":\"/34/0/\",\"n\":\"2\",\"v\":1}]";
    senml_json_test_raw_expected("/34/0/", (uint8_t *)buffer, strlen(buffer), expect, strlen(expect), LWM2M_CONTENT_SENML_JSON, "20");
}

static void senml_json_test_21(void)
{
    /* Unsupported field requiring error */
    const char * buffer = "[{ \"bn\":\"/\",\"test_\":\"fail\",\"n\":\"34/0/2\",\"v\":1}]";
    senml_json_test_raw_error(NULL, (uint8_t *)buffer, strlen(buffer), LWM2M_CONTENT_SENML_JSON, "19");
}

static void senml_json_test_22(void)
{
    /* Base value */
    const char * buffer = "[{ \"bv\":1,\"n\":\"/34/0/2\",\"v\":1}]";
    const char * expect = "[{ \"n\":\"/34/0/2\",\"v\":2}]";
    senml_json_test_raw_expected(NULL, (uint8_t *)buffer, strlen(buffer), expect, strlen(expect), LWM2M_CONTENT_SENML_JSON, "22");
}

static void senml_json_test_23(void)
{
    /* Opaque data */
    const uint8_t rawData[] = { 1, 2, 3, 4, 5 };
    lwm2m_data_t *dataP = lwm2m_data_new(1);
    CU_ASSERT_PTR_NOT_NULL_FATAL(dataP);
    lwm2m_data_encode_opaque(rawData, sizeof(rawData), dataP);
    const char * buffer = "[{\"bn\":\"/34/0/2\",\"vd\":\"AQIDBAU=\"}]";
    senml_json_test_raw("/34/0/2", (uint8_t *)buffer, strlen(buffer), LWM2M_CONTENT_SENML_JSON, "23a");
    senml_json_test_data_and_compare("/34/0/2",
                                     LWM2M_CONTENT_SENML_JSON,
                                     dataP,
                                     1,
                                     "23b",
                                     (uint8_t *)buffer,
                                     strlen(buffer));
    lwm2m_data_free(1, dataP);

    /* This test is added for https://github.com/eclipse/wakaama/issues/274 */
    const char * buffer2 = "[{\"bn\":\"/34/0/2\",\"vd\":\"MA==\"}]";
    senml_json_test_raw("/34/0/2", (uint8_t *)buffer2, strlen(buffer2), LWM2M_CONTENT_SENML_JSON, "23c");
}

static void senml_json_test_24(void)
{
    /* JSON string escaping */
    lwm2m_data_t *dataP = lwm2m_data_new(1);
    char string[33 + 8]; /* As array to allow embedded NULL */
    int i;
    for(i=0; i<33; i++) string[i] = i;
    string[i++] = '"';
    string[i++] = '\\';
    string[i++] = '/';
    string[i++] = '\b';
    string[i++] = '\f';
    string[i++] = '\n';
    string[i++] = '\r';
    string[i++] = '\t';
    lwm2m_data_encode_nstring(string, sizeof(string), dataP);
    const char * buffer = "[{\"bn\":\"/34/0/2\",\"vs\":\"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007\\u0008\\u0009\\u000A\\u000B\\u000C\\u000D\\u000E\\u000F\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017\\u0018\\u0019\\u001a\\u001b\\u001c\\u001d\\u001e\\u001f\\u0020\\\"\\\\\\/\\b\\f\\n\\r\\t\"}]";
    const char * expect = "[{\"bn\":\"/34/0/2\",\"vs\":\"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007\\b\\t\\n\\u000B\\f\\r\\u000E\\u000F\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017\\u0018\\u0019\\u001A\\u001B\\u001C\\u001D\\u001E\\u001F \\\"\\\\/\\b\\f\\n\\r\\t\"}]";
    senml_json_test_raw_expected("/34/0/2", (const uint8_t *)buffer, strlen(buffer), expect, strlen(expect), LWM2M_CONTENT_SENML_JSON, "24a");
    senml_json_test_data_and_compare("/34/0/2",
                                     LWM2M_CONTENT_SENML_JSON,
                                     dataP,
                                     1,
                                     "24b",
                                     (const uint8_t *)expect,
                                     strlen(expect));
    lwm2m_data_free(1, dataP);
}

static struct TestTable table[] = {
        { "test of senml_json_test_1()", senml_json_test_1 },
        { "test of senml_json_test_2()", senml_json_test_2 },
        { "test of senml_json_test_3()", senml_json_test_3 },
        { "test of senml_json_test_4()", senml_json_test_4 },
        { "test of senml_json_test_5()", senml_json_test_5 },
        { "test of senml_json_test_6()", senml_json_test_6 },
        { "test of senml_json_test_7()", senml_json_test_7 },
        { "test of senml_json_test_8()", senml_json_test_8 },
        { "test of senml_json_test_9()", senml_json_test_9 },
        { "test of senml_json_test_10()", senml_json_test_10 },
        { "test of senml_json_test_11()", senml_json_test_11 },
        { "test of senml_json_test_12()", senml_json_test_12 },
        { "test of senml_json_test_13()", senml_json_test_13 },
        { "test of senml_json_test_14()", senml_json_test_14 },
        { "test of senml_json_test_15()", senml_json_test_15 },
        { "test of senml_json_test_16()", senml_json_test_16 },
        { "test of senml_json_test_17()", senml_json_test_17 },
        { "test of senml_json_test_18()", senml_json_test_18 },
        { "test of senml_json_test_19()", senml_json_test_19 },
        { "test of senml_json_test_20()", senml_json_test_20 },
        { "test of senml_json_test_21()", senml_json_test_21 },
        { "test of senml_json_test_22()", senml_json_test_22 },
        { "test of senml_json_test_23()", senml_json_test_23 },
        { "test of senml_json_test_24()", senml_json_test_24 },
        { NULL, NULL },
};

CU_ErrorCode create_senml_json_suit()
{
   CU_pSuite pSuite = NULL;

   pSuite = CU_add_suite("Suite_SenML_JSON", NULL, NULL);
   if (NULL == pSuite) {
      return CU_get_error();
   }

   return add_tests(pSuite, table);
}

#endif
