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

#ifdef LWM2M_SUPPORT_SENML_CBOR

#ifndef STR_MEDIA_TYPE
#define STR_MEDIA_TYPE(M)                          \
((M) == LWM2M_CONTENT_TEXT ? "Text" :              \
((M) == LWM2M_CONTENT_LINK ? "Link" :              \
((M) == LWM2M_CONTENT_OPAQUE ? "Opaque" :          \
((M) == LWM2M_CONTENT_TLV ? "TLV" :                \
((M) == LWM2M_CONTENT_JSON ? "JSON" :              \
((M) == LWM2M_CONTENT_SENML_JSON ? "SenML JSON" :  \
((M) == LWM2M_CONTENT_SENML_CBOR ? "SenML CBOR" :  \
"Unknown")))))))
#endif

static void senml_cbor_test_data(const char * uriStr,
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

static void senml_cbor_test_data_and_compare(const char * uriStr,
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
 *        to TLV and CBOR and if applicable compares it to the original testBuf.
 * @param testBuf The input buffer.
 * @param testLen The length of the input buffer.
 * @param format The format of the testBuf. Maybe LWM2M_CONTENT_TLV or LWM2M_CONTENT_CBOR at the moment.
 * @param id The test object id for debug out.
 */
static void senml_cbor_test_raw(const char * uriStr,
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
    senml_cbor_test_data_and_compare(uriStr, format, tlvP, size, id, testBuf, testLen);

    // Serialize to the TLV format
    // the reverse is not possible as TLV format loses the data type information
    if (format == LWM2M_CONTENT_JSON)
    {
        senml_cbor_test_data(uriStr, LWM2M_CONTENT_TLV, tlvP, size, id);
    }
    else if (format == LWM2M_CONTENT_SENML_CBOR)
    {
        senml_cbor_test_data(uriStr, LWM2M_CONTENT_TLV, tlvP, size, id);
    }
    lwm2m_data_free(size, tlvP);
}

static void senml_cbor_test_raw_expected(const char * uriStr,
                              const uint8_t * testBuf,
                              size_t testLen,
                              const uint8_t * expectBuf,
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
    senml_cbor_test_data_and_compare(uriStr, format, tlvP, size, id, expectBuf, expectLen);

    // Serialize to the other format respectively.
    if (format == LWM2M_CONTENT_TLV)
        senml_cbor_test_data(uriStr, LWM2M_CONTENT_JSON, tlvP, size, id);
    else if (format == LWM2M_CONTENT_JSON)
        senml_cbor_test_data(uriStr, LWM2M_CONTENT_TLV, tlvP, size, id);
    else if (format == LWM2M_CONTENT_SENML_CBOR)
        senml_cbor_test_data(uriStr, LWM2M_CONTENT_TLV, tlvP, size, id);
    lwm2m_data_free(size, tlvP);
}

static void senml_cbor_test_raw_error(const char * uriStr,
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

static void senml_cbor_test_1(void)
{
    // From Table 7.4.4-2 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    // [{-2: "/3/0/", 0: "0", 3: "Open Mobile Alliance"},
    //  {0: "1", 3: "Lightweight M2M Client"},
    //  {0: "2", 3: "345000123"},
    //  {0: "3", 3: "1.0"},
    //  {0: "6/0", 2: 1},
    //  {0: "6/1", 2: 5},
    //  {0: "7/0", 2: 3800},
    //  {0: "7/1", 2: 5000},
    //  {0: "8/0", 2: 125},
    //  {0: "8/1", 2: 900},
    //  {0: "9", 2: 100},
    //  {0: "10", 2: 15},
    //  {0: "11/0", 2: 0},
    //  {0: "13", 2: 1367491215},
    //  {0: "14", 3: "+02:00"},
    //  {0: "16", 3: "U"} ]
    const uint8_t buffer[] = {
        0x90,
            0xa3,
                0x21, 0x65, '/', '3', '/', '0', '/',
                0x00, 0x61, '0',
                0x03, 0x74, 'O', 'p', 'e', 'n', ' ', 'M', 'o', 'b', 'i', 'l', 'e', ' ', 'A', 'l', 'l', 'i', 'a', 'n', 'c', 'e',
            0xa2,
                0x00, 0x61, '1',
                0x03, 0x76, 'L', 'i', 'g', 'h', 't', 'w', 'e', 'i', 'g', 'h', 't', ' ', 'M', '2', 'M', ' ', 'C', 'l', 'i', 'e', 'n', 't',
            0xa2,
                0x00, 0x61, '2',
                0x03, 0x69, '3', '4', '5', '0', '0', '0', '1', '2', '3',
            0xa2,
                0x00, 0x61, '3',
                0x03, 0x63, '1', '.', '0',
            0xa2,
                0x00, 0x63, '6', '/', '0',
                0x02, 0x01,
            0xa2,
                0x00, 0x63, '6', '/', '1',
                0x02, 0x05,
            0xa2,
                0x00, 0x63, '7', '/', '0',
                0x02, 0x19, 0x0e, 0xd8,
            0xa2,
                0x00, 0x63, '7', '/', '1',
                0x02, 0x19, 0x13, 0x88,
            0xa2,
                0x00, 0x63, '8', '/', '0',
                0x02, 0x18, 0x7d,
            0xa2,
                0x00, 0x63, '8', '/', '1',
                0x02, 0x19, 0x03, 0x84,
            0xa2,
                0x00, 0x61, '9',
                0x02, 0x18, 0x64,
            0xa2,
                0x00, 0x62, '1', '0',
                0x02, 0x0f,
            0xa2,
                0x00, 0x64, '1', '1', '/', '0',
                0x02, 0x00,
            0xa2,
                0x00, 0x62, '1', '3',
                0x02, 0x1a, 0x51, 0x82, 0x42, 0x8f,
            0xa2,
                0x00, 0x62, '1', '4',
                0x03, 0x66, '+', '0', '2', ':', '0', '0',
            0xa2,
                0x00, 0x62, '1', '6',
                0x03, 0x61, 'U'
    };
    senml_cbor_test_raw("/3/0", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "1");
}

static void senml_cbor_test_2(void)
{
    // From Table 7.4.4-3 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    // [{-2: "/72/", -3: 25462634, 0: "1/2", 2: 22.4, 6: -5},
    //  {0: "1/2", 2: 22.9, 6: -30},
    //  {0: "1/2", 2: 24.1, 6: -50}]
    const uint8_t buffer[] = {
        0x83,
            0xa5,
                0x21, 0x64, '/', '7', '2', '/',
                0x22, 0x1a, 0x01, 0x84, 0x87, 0x6a,
                0x00, 0x63, '1', '/', '2',
                0x02, 0xfb, 0x40, 0x36, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                0x06, 0x24,
            0xa3,
                0x00, 0x63, '1', '/', '2',
                0x02, 0xfb, 0x40, 0x36, 0xe6, 0x66, 0x66, 0x66, 0x66, 0x66,
                0x06, 0x38, 0x1d,
            0xa3,
                0x00, 0x63, '1', '/', '2',
                0x02, 0xfb, 0x40, 0x38, 0x19, 0x99, 0x99, 0x99, 0x99, 0x9a,
                0x06, 0x38, 0x31
    };
#if 0
    // TODO: This should pass once time is supported
    senml_cbor_test_raw("/72", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "2");
#else
    // [{-2: "/72/", 0: "1/2", 2: 24.1}]
    const uint8_t expect[] = {
        0x81,
            0xa3,
                0x21, 0x64, '/', '7', '2', '/',
                0x00, 0x63, '1', '/', '2',
                0x02, 0xfb, 0x40, 0x38, 0x19, 0x99, 0x99, 0x99, 0x99, 0x9a
    };
    senml_cbor_test_raw_expected("/72", buffer, sizeof(buffer), expect, sizeof(expect), LWM2M_CONTENT_SENML_CBOR, "2");
#endif
}

static void senml_cbor_test_3(void)
{
    // From Table 7.4.4-4 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    // Corrected /66/1/2 from hex to decimal
    // [{-2: "/", 0: "65/0/0/0", "vlo": "66:0"},
    //  {0: "65/0/0/1", "vlo": "66:1"},
    //  {0: "65/0/1", 3: "8613800755500"},
    //  {0: "65/0/2", 2: 1},
    //  {0: "66/0/0", 3: "myService1"},
    //  {0: "66/0/1", 3: "Internet.15.234"},
    //  {0: "66/0/2", "vlo": "67:0"},
    //  {0: "66/1/0", 3: "myService2"},
    //  {0: "66/1/1", 3: "Internet.15.235"},
    //  {0: "66/1/2", "vlo": "65535:65535"},
    //  {0: "67/0/0", 3: "85.76.76.84"},
    //  {0: "67/0/1", 3: "85.76.255.255"}]
    const uint8_t buffer[] = {
        0x8c,
            0xa3,
                0x21, 0x61, '/',
                0x00, 0x68, '6', '5', '/', '0', '/', '0', '/', '0',
                0x63, 'v', 'l', 'o', 0x64, '6', '6', ':', '0',
            0xa2,
                0x00, 0x68, '6', '5', '/', '0', '/', '0', '/', '1',
                0x63, 'v', 'l', 'o', 0x64, '6', '6', ':', '1',
            0xa2,
                0x00, 0x66, '6', '5', '/', '0', '/', '1',
                0x03, 0x6d, '8', '6', '1', '3', '8', '0', '0', '7', '5', '5', '5', '0', '0',
            0xa2,
                0x00, 0x66, '6', '5', '/', '0', '/', '2',
                0x02, 0x01,
            0xa2,
                0x00, 0x66, '6', '6', '/', '0', '/', '0',
                0x03, 0x6a, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e', '1',
            0xa2,
                0x00, 0x66, '6', '6', '/', '0', '/', '1',
                0x03, 0x6f, 'I', 'n', 't', 'e', 'r', 'n', 'e', 't', '.', '1', '5', '.', '2', '3', '4',
            0xa2,
                0x00, 0x66, '6', '6', '/', '0', '/', '2',
                0x63, 'v', 'l', 'o', 0x64, '6', '7', ':', '0',
            0xa2,
                0x00, 0x66, '6', '6', '/', '1', '/', '0',
                0x03, 0x6a, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e', '2',
            0xa2,
                0x00, 0x66, '6', '6', '/', '1', '/', '1',
                0x03, 0x6f, 'I', 'n', 't', 'e', 'r', 'n', 'e', 't', '.', '1', '5', '.', '2', '3', '5',
            0xa2,
                0x00, 0x66, '6', '6', '/', '1', '/', '2',
                0x63, 'v', 'l', 'o', 0x6b, '6', '5', '5', '3', '5', ':', '6', '5', '5', '3', '5',
            0xa2,
                0x00, 0x66, '6', '7', '/', '0', '/', '0',
                0x03, 0x6b, '8', '5', '.', '7', '6', '.', '7', '6', '.', '8', '4',
            0xa2,
                0x00, 0x66, '6', '7', '/', '0', '/', '1',
                0x03, 0x6d, '8', '5', '.', '7', '6', '.', '2', '5', '5', '.', '2', '5', '5'
    };
    senml_cbor_test_raw("/", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "3");
}

static void senml_cbor_test_4(void)
{
    // From Table 7.4.4-5 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    // Corrected for missing closing quote.
    // [{-2: "/3/0/0", 3: "Open Mobile Alliance"}]
    const uint8_t buffer[] = {
        0x81,
            0xa2,
                0x21, 0x66, '/', '3', '/', '0', '/', '0',
                0x03, 0x74, 'O', 'p', 'e', 'n', ' ', 'M', 'o', 'b', 'i', 'l', 'e', ' ', 'A', 'l', 'l', 'i', 'a', 'n', 'c', 'e'
    };
    senml_cbor_test_raw("/3/0/0", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "4");
}

static void senml_cbor_test_5(void)
{
    // From Table 7.4.4-6 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    // [{0: "/3/0/0"},
    //  {0: "/3/0/9"},
    //  {0: "/1/0/1"}]
    const uint8_t buffer[] = {
        0x83,
            0xa1,
                0x00, 0x66, '/', '3', '/', '0', '/', '0',
            0xa1,
                0x00, 0x66, '/', '3', '/', '0', '/', '9',
            0xa1,
                0x00, 0x66, '/', '1', '/', '0', '/', '1'
    };
    senml_cbor_test_raw(NULL, buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "5");
}

static void senml_cbor_test_6(void)
{
    // From Table 7.4.4-7 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    // [{0: "/3/0/0", 3: "Open Mobile Alliance"},
    //  {0: "/3/0/9", 2: 95},
    //  {0: "/1/0/1", 2: 86400}]
    const uint8_t buffer[] = {
            0x83,
                0xa2,
                    0x00, 0x66, '/', '3', '/', '0', '/', '0',
                    0x03, 0x74, 'O', 'p', 'e', 'n', ' ', 'M', 'o', 'b', 'i', 'l', 'e', ' ', 'A', 'l', 'l', 'i', 'a', 'n', 'c', 'e',
                0xa2,
                    0x00, 0x66, '/', '3', '/', '0', '/', '9',
                    0x02, 0x18, 0x5f,
                0xa2,
                    0x00, 0x66, '/', '1', '/', '0', '/', '1',
                    0x02, 0x1a, 0x00, 0x01, 0x51, 0x80
    };
    senml_cbor_test_raw(NULL, buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "6");
}

static void senml_cbor_test_7(void)
{
    // From Table 7.4.4-8 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    // [{0: "/3311/0/5850", 4: false},
    //  {0: "/3311/1/5850", 4: false},
    //  {0: "/3311/2/5851", 2: 20},
    //  {0: "/3308/0/5900", 2: 18}]
    const uint8_t buffer[] = {
        0x84,
            0xa2,
                0x00, 0x6c, '/', '3', '3', '1', '1', '/', '0', '/', '5', '8', '5', '0',
                0x04, 0xf4,
            0xa2,
                0x00, 0x6c, '/', '3', '3', '1', '1', '/', '1', '/', '5', '8', '5', '0',
                0x04, 0xf4,
            0xa2,
                0x00, 0x6c, '/', '3', '3', '1', '1', '/', '2', '/', '5', '8', '5', '1',
                0x02, 0x14,
            0xa2,
                0x00, 0x6c, '/', '3', '3', '0', '8', '/', '0', '/', '5', '9', '0', '0',
                0x02, 0x12
    };
    senml_cbor_test_raw(NULL, buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "7");
}

static void senml_cbor_test_8(void)
{
    // The ordering here isn't strictly valid, but we can allow it.
    // [{-2: "/65/543/", 0: "0", 3: "LWM2M"},
    //  {2: 2015, 0: "1"},
    //  {0: "2/0", 4: true},
    //  {4: false, 0: "2/1"}]
    const uint8_t buffer[] = {
        0x84,
            0xa3,
                0x21, 0x68, '/', '6', '5', '/', '5', '4', '3', '/',
                0x00, 0x61, '0',
                0x03, 0x65, 'L', 'W', 'M', '2', 'M',
            0xa2,
                0x02, 0x19, 0x07, 0xdf,
                0x00, 0x61, '1',
            0xa2,
                0x00, 0x63, '2', '/', '0',
                0x04, 0xf5,
            0xa2,
                0x04, 0xf4,
                0x00, 0x63, '2', '/', '1'
    };
    // We serialize in the proper order
    // [{-2: "/65/543/", 0: "0", 3: "LWM2M"},
    //  {0: "1", 2: 2015},
    //  {0: "2/0", 4: true},
    //  {0: "2/1", 4: false}]
    const uint8_t expect[] = {
        0x84,
            0xa3,
                0x21, 0x68, '/', '6', '5', '/', '5', '4', '3', '/',
                0x00, 0x61, '0',
                0x03, 0x65, 'L', 'W', 'M', '2', 'M',
            0xa2,
                0x00, 0x61, '1',
                0x02, 0x19, 0x07, 0xdf,
            0xa2,
                0x00, 0x63, '2', '/', '0',
                0x04, 0xf5,
            0xa2,
                0x00, 0x63, '2', '/', '1',
                0x04, 0xf4
    };
    senml_cbor_test_raw_expected("/65/543", buffer, sizeof(buffer), expect, sizeof(expect), LWM2M_CONTENT_SENML_CBOR, "8");
}

static void senml_cbor_test_9(void)
{
    // [{-2: "/2/3", 0: "/0", 2: 1234}]
    const uint8_t buffer[] = {
        0x81,
            0xa3,
                0x21, 0x64, '/', '2', '/', '3',
                0x00, 0x62, '/', '0',
                0x02, 0x19, 0x04, 0xd2
    };
    // [{-2: "/2/3/", 0: "0", 2: 1234}]
    const uint8_t expect[] = {
        0x81,
            0xa3,
                0x21, 0x65, '/', '2', '/', '3', '/',
                0x00, 0x61, '0',
                0x02, 0x19, 0x04, 0xd2
    };

    senml_cbor_test_raw_expected("/2/3", buffer, sizeof(buffer), expect, sizeof(expect), LWM2M_CONTENT_SENML_CBOR, "9");
}

static void senml_cbor_test_10(void)
{
    // [{0: "1", 2: 1234, -2: "/12/0/", -3: 1234567},
    //  {0: "3", 2: 56.789},
    //  {0: "2/0", 2: 0.23},
    //  {0: "2/1", 2: -52.0006}]
    const uint8_t buffer[] = {
        0x84,
            0xa4,
                0x00, 0x61, '1',
                0x02, 0x19, 0x04, 0xd2,
                0x21, 0x66, '/', '1', '2', '/', '0', '/',
                0x22, 0x1a, 0x00, 0x12, 0xd6, 0x87,
            0xa2,
                0x00, 0x61, '3',
                0x02, 0xfb, 0x40, 0x4C, 0x64, 0xFD, 0xF3, 0xB6, 0x45, 0xA2,
            0xa2,
                0x00, 0x63, '2', '/', '0',
                0x02, 0xfb, 0x3F, 0xCD, 0x70, 0xA3, 0xD7, 0x0A, 0x3D, 0x71,
            0xa2,
                0x00, 0x63, '2', '/', '1',
                0x02, 0xfb, 0xC0, 0x4A, 0x00, 0x13, 0xA9, 0x2A, 0x30, 0x55
    };

    // [{0: "/12/0/1", 2: 1234},
    //  {0: "/12/0/3", 2: 56.789},
    //  {0: "/12/0/2/0", 2: 0.23},
    //  {0: "/12/0/2/1", 2: -52.0006}]
    const uint8_t expectA[] = {
        0x84,
            0xa2,
                0x00, 0x67, '/', '1', '2', '/', '0', '/', '1',
                0x02, 0x19, 0x04, 0xd2,
            0xa2,
                0x00, 0x67, '/', '1', '2', '/', '0', '/', '3',
                0x02, 0xfb, 0x40, 0x4C, 0x64, 0xFD, 0xF3, 0xB6, 0x45, 0xA2,
            0xa2,
                0x00, 0x69, '/', '1', '2', '/', '0', '/', '2', '/', '0',
                0x02, 0xfb, 0x3F, 0xCD, 0x70, 0xA3, 0xD7, 0x0A, 0x3D, 0x71,
            0xa2,
                0x00, 0x69, '/', '1', '2', '/', '0', '/', '2', '/', '1',
                0x02, 0xfb, 0xC0, 0x4A, 0x00, 0x13, 0xA9, 0x2A, 0x30, 0x55
    };
    // [{-2: "/", 0: "12/0/1", 2: 1234},
    //  {0: "12/0/3", 2: 56.789},
    //  {0: "12/0/2/0", 2: 0.23},
    //  {0: "12/0/2/1", 2: -52.0006}]
    const uint8_t expectB[] = {
        0x84,
            0xa3,
                0x21, 0x61, '/',
                0x00, 0x66, '1', '2', '/', '0', '/', '1',
                0x02, 0x19, 0x04, 0xd2,
            0xa2,
                0x00, 0x66, '1', '2', '/', '0', '/', '3',
                0x02, 0xfb, 0x40, 0x4C, 0x64, 0xFD, 0xF3, 0xB6, 0x45, 0xA2,
            0xa2,
                0x00, 0x68, '1', '2', '/', '0', '/', '2', '/', '0',
                0x02, 0xfb, 0x3F, 0xCD, 0x70, 0xA3, 0xD7, 0x0A, 0x3D, 0x71,
            0xa2,
                0x00, 0x68, '1', '2', '/', '0', '/', '2', '/', '1',
                0x02, 0xfb, 0xC0, 0x4A, 0x00, 0x13, 0xA9, 0x2A, 0x30, 0x55
    };
    // [{-2: "/12/", 0: "0/1", 2: 1234},
    //  {0: "0/3", 2: 56.789},
    //  {0: "0/2/0", 2: 0.23},
    //  {0: "0/2/1", 2: -52.0006}]
    const uint8_t expectC[] = {
        0x84,
            0xa3,
                0x21, 0x64, '/', '1', '2', '/',
                0x00, 0x63, '0', '/', '1',
                0x02, 0x19, 0x04, 0xd2,
            0xa2,
                0x00, 0x63, '0', '/', '3',
                0x02, 0xfb, 0x40, 0x4C, 0x64, 0xFD, 0xF3, 0xB6, 0x45, 0xA2,
            0xa2,
                0x00, 0x65, '0', '/', '2', '/', '0',
                0x02, 0xfb, 0x3F, 0xCD, 0x70, 0xA3, 0xD7, 0x0A, 0x3D, 0x71,
            0xa2,
                0x00, 0x65, '0', '/', '2', '/', '1',
                0x02, 0xfb, 0xC0, 0x4A, 0x00, 0x13, 0xA9, 0x2A, 0x30, 0x55
    };
    // [{-2: "/12/0/", 0: "1", 2: 1234},
    //  {0: "3", 2: 56.789},
    //  {0: "2/0", 2:0.23},
    //  {0: "2/1", 2: -52.0006}]
    const uint8_t expectD[] = {
        0x84,
            0xa3,
                0x21, 0x66, '/', '1', '2', '/', '0', '/',
                0x00, 0x61, '1',
                0x02, 0x19, 0x04, 0xd2,
            0xa2,
                0x00, 0x61, '3',
                0x02, 0xfb, 0x40, 0x4C, 0x64, 0xFD, 0xF3, 0xB6, 0x45, 0xA2,
            0xa2,
                0x00, 0x63, '2', '/', '0',
                0x02, 0xfb, 0x3F, 0xCD, 0x70, 0xA3, 0xD7, 0x0A, 0x3D, 0x71,
            0xa2,
                0x00, 0x63, '2', '/', '1',
                0x02, 0xfb, 0xC0, 0x4A, 0x00, 0x13, 0xA9, 0x2A, 0x30, 0x55
    };
    // [{-2: "/12/0/3", 2: 56.789}]
    const uint8_t expectE[] = {
        0x81,
            0xa2,
                0x21, 0x67, '/', '1', '2', '/', '0', '/', '3',
                0x02, 0xfb, 0x40, 0x4C, 0x64, 0xFD, 0xF3, 0xB6, 0x45, 0xA2,
    };

    senml_cbor_test_raw_expected(NULL, buffer, sizeof(buffer), expectA, sizeof(expectA), LWM2M_CONTENT_SENML_CBOR, "10a");
    senml_cbor_test_raw_expected("/", buffer, sizeof(buffer), expectB, sizeof(expectB), LWM2M_CONTENT_SENML_CBOR, "10b");
    senml_cbor_test_raw_expected("/12", buffer, sizeof(buffer), expectC, sizeof(expectC), LWM2M_CONTENT_SENML_CBOR, "10c");
    senml_cbor_test_raw_expected("/12/0", buffer, sizeof(buffer), expectD, sizeof(expectD), LWM2M_CONTENT_SENML_CBOR, "10d");
    senml_cbor_test_raw_expected("/12/0/3", buffer, sizeof(buffer), expectE, sizeof(expectE), LWM2M_CONTENT_SENML_CBOR, "10e");
}

static void senml_cbor_test_11(void)
{
    // [{-2: "/", 0: "34/0/1", 3: "8613800755500"},
    //  {0: "34/0/2", 2: 1},
    //  {0: "66/0/0", 3: "myService1"},
    //  {0: "66/0/1", 3: "Internet.15.234"},
    //  {0: "66/1/0", 3: "myService2"},
    //  {0: "66/1/1", 3: "Internet.15.235"},
    //  {0: "31/0/0", 3: "85.76.76.84"},
    //  {0: "31/0/1", 3: "85.76.255.255"}]
    const uint8_t buffer[] = {
        0x88,
            0xa3,
                0x21, 0x61, '/',
                0x00, 0x66, '3', '4', '/', '0', '/', '1',
                0x03, 0x6d, '8', '6', '1', '3', '8', '0', '0', '7', '5', '5', '5', '0', '0',
            0xa2,
                0x00, 0x66, '3', '4', '/', '0', '/', '2',
                0x02, 0x01,
            0xa2,
                0x00, 0x66, '6', '6', '/', '0', '/', '0',
                0x03, 0x6a, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e', '1',
            0xa2,
                0x00, 0x66, '6', '6', '/', '0', '/', '1',
                0x03, 0x6f, 'I', 'n', 't', 'e', 'r', 'n', 'e', 't', '.', '1', '5', '.', '2', '3', '4',
            0xa2,
                0x00, 0x66, '6', '6', '/', '1', '/', '0',
                0x03, 0x6a, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e', '2',
            0xa2,
                0x00, 0x66, '6', '6', '/', '1', '/', '1',
                0x03, 0x6f, 'I', 'n', 't', 'e', 'r', 'n', 'e', 't', '.', '1', '5', '.', '2', '3', '5',
            0xa2,
                0x00, 0x66, '3', '1', '/', '0', '/', '0',
                0x03, 0x6b, '8', '5', '.', '7', '6', '.', '7', '6', '.', '8', '4',
            0xa2,
                0x00, 0x66, '3', '1', '/', '0', '/', '1',
                0x03, 0x6d, '8', '5', '.', '7', '6', '.', '2', '5', '5', '.', '2', '5', '5'
    };

    senml_cbor_test_raw("/", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "11");
}

static void senml_cbor_test_12(void)
{
    // [{-2: "/", 0: "34/0/0/0", "vlo": "66:0"},
    //  {0: "34/0/0/1", "vlo": "66:1"},
    //  {0: "34/0/1", 3: "8613800755500"},
    //  {0: "34/0/2", 2: 1},
    //  {0: "66/0/0", 3: "myService1"},
    //  {0: "66/0/1", 3: "Internet.15.234"},
    //  {0: "66/0/2", "vlo": "31:0"},
    //  {0: "66/1/0", 3: "myService2"},
    //  {0: "66/1/1", 3: "Internet.15.235"},
    //  {0: "66/1/2", "vlo": "65535:65535"},
    //  {0: "31/0/0", 3: "85.76.76.84"},
    //  {0: "31/0/1", 3: "85.76.255.255"}]
    const uint8_t buffer[] = {
        0x8c,
            0xa3,
                0x21, 0x61, '/',
                0x00, 0x68, '3', '4', '/', '0', '/', '0', '/', '0',
                0x63, 'v', 'l', 'o', 0x64, '6', '6', ':', '0',
            0xa2,
                0x00, 0x68, '3', '4', '/', '0', '/', '0', '/', '1',
                0x63, 'v', 'l', 'o', 0x64, '6', '6', ':', '1',
            0xa2,
                0x00, 0x66, '3', '4', '/', '0', '/', '1',
                0x03, 0x6d, '8', '6', '1', '3', '8', '0', '0', '7', '5', '5', '5', '0', '0',
            0xa2,
                0x00, 0x66, '3', '4', '/', '0', '/', '2',
                0x02, 0x01,
            0xa2,
                0x00, 0x66, '6', '6', '/', '0', '/', '0',
                0x03, 0x6a, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e', '1',
            0xa2,
                0x00, 0x66, '6', '6', '/', '0', '/', '1',
                0x03, 0x6f, 'I', 'n', 't', 'e', 'r', 'n', 'e', 't', '.', '1', '5', '.', '2', '3', '4',
            0xa2,
                0x00, 0x66, '6', '6', '/', '0', '/', '2',
                0x63, 'v', 'l', 'o', 0x64, '3', '1', ':', '0',
            0xa2,
                0x00, 0x66, '6', '6', '/', '1', '/', '0',
                0x03, 0x6a, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e', '2',
            0xa2,
                0x00, 0x66, '6', '6', '/', '1', '/', '1',
                0x03, 0x6f, 'I', 'n', 't', 'e', 'r', 'n', 'e', 't', '.', '1', '5', '.', '2', '3', '5',
            0xa2,
                0x00, 0x66, '6', '6', '/', '1', '/', '2',
                0x63, 'v', 'l', 'o', 0x6b, '6', '5', '5', '3', '5', ':', '6', '5', '5', '3', '5',
            0xa2,
                0x00, 0x66, '3', '1', '/', '0', '/', '0',
                0x03, 0x6b, '8', '5', '.', '7', '6', '.', '7', '6', '.', '8', '4',
            0xa2,
                0x00, 0x66, '3', '1', '/', '0', '/', '1',
                0x03, 0x6d, '8', '5', '.', '7', '6', '.', '2', '5', '5', '.', '2', '5', '5'
    };

    senml_cbor_test_raw("/", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "12");
}

static void senml_cbor_test_13(void)
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

    senml_cbor_test_data("/12/0", LWM2M_CONTENT_SENML_CBOR, data1, 17, "13");

    lwm2m_data_free(1, data1);
}

static void senml_cbor_test_14(void)
{
    // [{-2: "/5/0/1", 0: "/1", 3: "http"}]
    const uint8_t buffer[] = {
        0x81,
            0xa3,
                0x21, 0x66, '/', '5', '/', '0', '/', '1',
                0x00, 0x62, '/', '1',
                0x03, 0x64, 'h', 't', 't', 'p'
    };
    // [{-2: "/5/0/1/", 0: "1", 3: "http"}]
    const uint8_t expect[] = {
        0x81,
            0xa3,
                0x21, 0x67, '/', '5', '/', '0', '/', '1', '/',
                0x00, 0x61, '1',
                0x03, 0x64, 'h', 't', 't', 'p'
    };
    // [{-2: "/5/0/1/1", 3: "http"}]
    const uint8_t expect2[] = {
        0x81,
            0xa2,
                0x21, 0x68, '/', '5', '/', '0', '/', '1', '/', '1',
                0x03, 0x64, 'h', 't', 't', 'p'
    };

    senml_cbor_test_raw_expected("/5/0/1", buffer, sizeof(buffer), expect, sizeof(expect), LWM2M_CONTENT_SENML_CBOR, "14a");

    senml_cbor_test_raw_expected("/5/0/1/1", buffer, sizeof(buffer), expect2, sizeof(expect2), LWM2M_CONTENT_SENML_CBOR, "14b");
}

static void senml_cbor_test_15(void)
{
    // [{-2: "/5/0", 0: "/1", 3: "http"}]
    const uint8_t buffer[] = {
        0x81,
            0xa3,
                0x21, 0x64, '/', '5', '/', '0',
                0x00, 0x62, '/', '1',
                0x03, 0x64, 'h', 't', 't', 'p'
    };
    // [{-2: "/5/0/", 0: "1", 3: "http"}]
    const uint8_t expect[] = {
        0x81,
            0xa3,
                0x21, 0x65, '/', '5', '/', '0', '/',
                0x00, 0x61, '1',
                0x03, 0x64, 'h', 't', 't', 'p'
    };

    senml_cbor_test_raw_expected("/5/0", buffer, sizeof(buffer), expect, sizeof(expect), LWM2M_CONTENT_SENML_CBOR, "15");
}

static void senml_cbor_test_16(void)
{
    // [{-2: "/5/0/1", 0: "", 3: "http"}]
    const uint8_t buffer[] = {
            0x81,
                0xa3,
                    0x21, 0x66, '/', '5', '/', '0', '/', '1',
                    0x00, 0x60,
                    0x03, 0x64, 'h', 't', 't', 'p'
    };
    // [{-2: "/5/0/1", 3: "http"}]
    const uint8_t expect[] = {
            0x81,
                0xa2,
                    0x21, 0x66, '/', '5', '/', '0', '/', '1',
                    0x03, 0x64, 'h', 't', 't', 'p'
    };

    senml_cbor_test_raw_expected("/5/0/1", buffer, sizeof(buffer), expect, sizeof(expect), LWM2M_CONTENT_SENML_CBOR, "16");
}

static void senml_cbor_test_17(void)
{
    // Adapted from Table 7.4.4-4 in OMA-TS-LightweightM2M_Core-V1_1-20180710-A
    // [{-2: "/65/0/", 0: "0/0", "vlo": "66:0"},
    //  {0: "0/1", "vlo": "66:1"},
    //  {0: "1", 3: "8613800755500"},
    //  {0: "2", 2: 1 },
    //  {-2: "/66/0/", 0: "0", 3: "myService1"},
    //  {0: "1", 3: "Internet.15.234"},
    //  {0: "2", "vlo": "67:0"},
    //  {-2: "/66/1/", 0: "0", 3: "myService2"},
    //  {0: "1", 3: "Internet.15.235"},
    //  {0: "2", "vlo": "65535:65535"},
    //  {-2: "", 0: "/67/0/00", 3: "85.76.76.84"},
    //  {0: "/67/0/1", 3: "85.76.255.255"}]
    const uint8_t buffer[] = {
        0x8c,
            0xa3,
                0x21, 0x66, '/', '6', '5', '/', '0', '/',
                0x00, 0x63, '0', '/', '0',
                0x63, 'v', 'l', 'o', 0x64, '6', '6', ':', '0',
            0xa2,
                0x00, 0x63, '0', '/', '1',
                0x63, 'v', 'l', 'o', 0x64, '6', '6', ':', '1',
            0xa2,
                0x00, 0x61, '1',
                0x03, 0x6d, '8', '6', '1', '3', '8', '0', '0', '7', '5', '5', '5', '0', '0',
            0xa2,
                0x00, 0x61, '2',
                0x02, 0x01,
            0xa3,
                0x21, 0x66, '/', '6', '6', '/', '0', '/',
                0x00, 0x61, '0',
                0x03, 0x6a, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e', '1',
            0xa2,
                0x00, 0x61, '1',
                0x03, 0x6f, 'I', 'n', 't', 'e', 'r', 'n', 'e', 't', '.', '1', '5', '.', '2', '3', '4',
            0xa2,
                0x00, 0x61, '2',
                0x63, 'v', 'l', 'o', 0x64, '6', '7', ':', '0',
            0xa3,
                0x21, 0x66, '/', '6', '6', '/', '1', '/',
                0x00, 0x61, '0',
                0x03, 0x6a, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e', '2',
            0xa2,
                0x00, 0x61, '1',
                0x03, 0x6f, 'I', 'n', 't', 'e', 'r', 'n', 'e', 't', '.', '1', '5', '.', '2', '3', '5',
            0xa2,
                0x00, 0x61, '2',
                0x63, 'v', 'l', 'o', 0x6b, '6', '5', '5', '3', '5', ':', '6', '5', '5', '3', '5',
            0xa3,
                0x21, 0x60,
                0x00, 0x68, '/', '6', '7', '/', '0', '/', '0', '0',
                0x03, 0x6b, '8', '5', '.', '7', '6', '.', '7', '6', '.', '8', '4',
            0xa2,
                0x00, 0x67, '/', '6', '7', '/', '0', '/', '1',
                0x03, 0x6d, '8', '5', '.', '7', '6', '.', '2', '5', '5', '.', '2', '5', '5'
    };
    // [{0: "/65/0/0/0", "vlo": "66:0"},
    //  {0: "/65/0/0/1", "vlo": "66:1"},
    //  {0: "/65/0/1", 3: "8613800755500"},
    //  {0: "/65/0/2", 2: 1},
    //  {0: "/66/0/0", 3: "myService1"},
    //  {0: "/66/0/1", 3: "Internet.15.234"},
    //  {0: "/66/0/2", "vlo": "67:0"},
    //  {0: "/66/1/0", 3: "myService2"},
    //  {0: "/66/1/1", 3: "Internet.15.235"},
    //  {0: "/66/1/2", "vlo": "65535:65535"},
    //  {0: "/67/0/0", 3: "85.76.76.84"},
    //  {0: "/67/0/1", 3: "85.76.255.255"}]
    const uint8_t expect[] = {
        0x8c,
            0xa2,
                0x00, 0x69, '/', '6', '5', '/', '0', '/', '0', '/', '0',
                0x63, 'v', 'l', 'o', 0x64, '6', '6', ':', '0',
            0xa2,
                0x00, 0x69, '/', '6', '5', '/', '0', '/', '0', '/', '1',
                0x63, 'v', 'l', 'o', 0x64, '6', '6', ':', '1',
            0xa2,
                0x00, 0x67, '/', '6', '5', '/', '0', '/', '1',
                0x03, 0x6d, '8', '6', '1', '3', '8', '0', '0', '7', '5', '5', '5', '0', '0',
            0xa2,
                0x00, 0x67, '/', '6', '5', '/', '0', '/', '2',
                0x02, 0x01,
            0xa2,
                0x00, 0x67, '/', '6', '6', '/', '0', '/', '0',
                0x03, 0x6a, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e', '1',
            0xa2,
                0x00, 0x67, '/', '6', '6', '/', '0', '/', '1',
                0x03, 0x6f, 'I', 'n', 't', 'e', 'r', 'n', 'e', 't', '.', '1', '5', '.', '2', '3', '4',
            0xa2,
                0x00, 0x67, '/', '6', '6', '/', '0', '/', '2',
                0x63, 'v', 'l', 'o', 0x64, '6', '7', ':', '0',
            0xa2,
                0x00, 0x67, '/', '6', '6', '/', '1', '/', '0',
                0x03, 0x6a, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e', '2',
            0xa2,
                0x00, 0x67, '/', '6', '6', '/', '1', '/', '1',
                0x03, 0x6f, 'I', 'n', 't', 'e', 'r', 'n', 'e', 't', '.', '1', '5', '.', '2', '3', '5',
            0xa2,
                0x00, 0x67, '/', '6', '6', '/', '1', '/', '2',
                0x63, 'v', 'l', 'o', 0x6b, '6', '5', '5', '3', '5', ':', '6', '5', '5', '3', '5',
            0xa2,
                0x00, 0x67, '/', '6', '7', '/', '0', '/', '0',
                0x03, 0x6b, '8', '5', '.', '7', '6', '.', '7', '6', '.', '8', '4',
            0xa2,
                0x00, 0x67, '/', '6', '7', '/', '0', '/', '1',
                0x03, 0x6d, '8', '5', '.', '7', '6', '.', '2', '5', '5', '.', '2', '5', '5'
    };
    senml_cbor_test_raw_expected(NULL, buffer, sizeof(buffer), expect, sizeof(expect), LWM2M_CONTENT_SENML_CBOR, "17");
}

static void senml_cbor_test_18(void)
{
    /* Explicit supported base version */
    // [{-1: 10, 0: "/34/0/2", 2: 1}]
    const uint8_t buffer[] = {
        0x81,
            0xa3,
                0x20, 0x0a,
                0x00, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x02, 0x01
    };
    // [{0: "/34/0/2", 2: 1}]
    const uint8_t expect[] = {
        0x81,
            0xa2,
                0x00, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x02, 0x01
    };
    senml_cbor_test_raw_expected(NULL, buffer, sizeof(buffer), expect, sizeof(expect), LWM2M_CONTENT_SENML_CBOR, "18");
}

static void senml_cbor_test_19(void)
{
    /* Unsupported base version */
    // [{-1: 11, 0: "/34/0/2", 2: 1}]
    const uint8_t buffer[] = {
        0x81,
            0xa3,
                0x20, 0x0b,
                0x00, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x02, 0x01
    };
    senml_cbor_test_raw_error(NULL, buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "19");
}

static void senml_cbor_test_20(void)
{
    /* Unsupported fields not causing error */
    // [{"test": "good", 0: "/34/0/2", 2: 1}]
    const uint8_t buffer[] = {
        0x81,
            0xa3,
                0x64, 't', 'e', 's', 't', 0x64, 'g', 'o', 'o', 'd',
                0x00, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x02, 0x01
    };
    // [{-2: "/34/0/", 0: "2", 2: 1}]
    const uint8_t expect[] = {
            0x81,
                0xa3,
                    0x21, 0x66, '/', '3', '4', '/', '0', '/',
                    0x00, 0x61, '2',
                    0x02, 0x01
    };
    senml_cbor_test_raw_expected("/34/0/", buffer, sizeof(buffer), expect, sizeof(expect), LWM2M_CONTENT_SENML_CBOR, "20");
}

static void senml_cbor_test_21(void)
{
    /* Unsupported field requiring error */
    // [{-2: "/", "test_": "fail", 0: "34/0/2", 2: 1}]
    const uint8_t buffer[] = {
        0x81,
            0xa4,
                0x21, 0x61, '/',
                0x65, 't', 'e', 's', 't', '_', 0x64, 'f', 'a', 'i', 'l',
                0x00, 0x66, '3', '4', '/', '0', '/', '2',
                0x02, 0x01
    };
    senml_cbor_test_raw_error(NULL, buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "19");
}

static void senml_cbor_test_22(void)
{
    /* Base value */
    // [{-5: 1, 0: "/34/0/2", 2: 1}]
    const uint8_t buffer[] = {
        0x81,
            0xa3,
                0x24, 0x01,
                0x00, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x02, 0x01
    };
    // [{0: "/34/0/2", 2: 2}]
    const uint8_t expect[] = {
            0x81,
                0xa2,
                    0x00, 0x67, '/', '3', '4', '/', '0', '/', '2',
                    0x02, 0x02
    };
    senml_cbor_test_raw_expected(NULL, buffer, sizeof(buffer), expect, sizeof(expect), LWM2M_CONTENT_SENML_CBOR, "22");
}

static void senml_cbor_test_23(void)
{
    /* Opaque data */
    const uint8_t rawData[] = { 1, 2, 3, 4, 5 };
    lwm2m_data_t *dataP = lwm2m_data_new(1);
    CU_ASSERT_PTR_NOT_NULL_FATAL(dataP);
    lwm2m_data_encode_opaque(rawData, sizeof(rawData), dataP);
    // [{-2: "/34/0/2", 8: h'0102030405'}]
    const uint8_t buffer[] = {
        0x81,
            0xa2,
                0x21, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x08, 0x45, 0x01, 0x02, 0x03, 0x04, 0x05
    };
    senml_cbor_test_raw("/34/0/2", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "23a");
    senml_cbor_test_data_and_compare("/34/0/2",
                                     LWM2M_CONTENT_SENML_CBOR,
                                     dataP,
                                     1,
                                     "23b",
                                     buffer,
                                     sizeof(buffer));
    lwm2m_data_free(1, dataP);

    // [{-2: "/34/0/2", 8: h'30'}]
    const uint8_t buffer2[] = {
        0x81,
            0xa2,
                0x21, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x08, 0x41, 0x30
    };
    senml_cbor_test_raw("/34/0/2", buffer2, sizeof(buffer2), LWM2M_CONTENT_SENML_CBOR, "23c");
}

static void senml_cbor_test_24(void)
{
    /* Test encoding from different depths */
    lwm2m_data_t * data1 = lwm2m_data_new(1);
    lwm2m_data_t * data2 = lwm2m_data_new(1);
    lwm2m_data_t * data3 = lwm2m_data_new(1);
    lwm2m_data_t * data4 = lwm2m_data_new(1);
    data4->id = 0;
    lwm2m_data_encode_int(0, data4);
    data3->id = 1;
    lwm2m_data_encode_instances(data4, 1, data3);
    data2->id = 0;
    lwm2m_data_include(data3, 1, data2);
    data1->id = 4;
    lwm2m_data_include(data2, 1, data1);
    // [{-2: "/4/", 0: "0/1/0", 2: 0}]
    const uint8_t expect1[] = {
        0x81,
            0xa3,
                0x21, 0x63, '/', '4', '/',
                0x00, 0x65, '0', '/', '1', '/', '0',
                0x02, 0x00
    };
    // [{-2:"/4/0/",0:"1/0",2:0}]
    const uint8_t expect2[] = {
        0x81,
            0xa3,
                0x21, 0x65, '/', '4', '/', '0', '/',
                0x00, 0x63, '1', '/', '0',
                0x02, 0x00
    };
    // [{-2:"/4/0/1/",0:"0",2:0}]
    const uint8_t expect3[] = {
        0x81,
            0xa3,
                0x21, 0x67, '/', '4', '/', '0', '/', '1', '/',
                0x00, 0x61, '0',
                0x02, 0x00
    };
    // [{-2:"/4/0/1/0",2:0}]
    const uint8_t expect4[] = {
        0x81,
            0xa2,
                0x21, 0x68, '/', '4', '/', '0', '/', '1', '/', '0',
                0x02, 0x00
    };

    senml_cbor_test_data_and_compare("/4", LWM2M_CONTENT_SENML_CBOR, data1, 1, "24a", (const uint8_t *)expect1, sizeof(expect1));
    senml_cbor_test_data_and_compare("/4/0", LWM2M_CONTENT_SENML_CBOR, data1, 1, "24b", (const uint8_t *)expect2, sizeof(expect2));
    senml_cbor_test_data_and_compare("/4/0", LWM2M_CONTENT_SENML_CBOR, data2, 1, "24c", (const uint8_t *)expect2, sizeof(expect2));
    senml_cbor_test_data_and_compare("/4/0/1", LWM2M_CONTENT_SENML_CBOR, data1, 1, "24d", (const uint8_t *)expect3, sizeof(expect3));
    senml_cbor_test_data_and_compare("/4/0/1", LWM2M_CONTENT_SENML_CBOR, data2, 1, "24e", (const uint8_t *)expect3, sizeof(expect3));
    senml_cbor_test_data_and_compare("/4/0/1", LWM2M_CONTENT_SENML_CBOR, data3, 1, "24f", (const uint8_t *)expect3, sizeof(expect3));
    senml_cbor_test_data_and_compare("/4/0/1/0", LWM2M_CONTENT_SENML_CBOR, data1, 1, "24g", (const uint8_t *)expect4, sizeof(expect4));
    senml_cbor_test_data_and_compare("/4/0/1/0", LWM2M_CONTENT_SENML_CBOR, data2, 1, "24h", (const uint8_t *)expect4, sizeof(expect4));
    senml_cbor_test_data_and_compare("/4/0/1/0", LWM2M_CONTENT_SENML_CBOR, data3, 1, "24i", (const uint8_t *)expect4, sizeof(expect4));
    senml_cbor_test_data_and_compare("/4/0/1/0", LWM2M_CONTENT_SENML_CBOR, data4, 1, "24j", (const uint8_t *)expect4, sizeof(expect4));

    lwm2m_data_free(1, data1);
}

static void senml_cbor_test_25(void)
{
    /* Test empty strings and opaques */
    // [{-2: "/34/0/2", 3: ""}]
    const uint8_t buffer[] = {
        0x81,
            0xa2,
                0x21, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x03, 0x60
    };
    // [{-2: "/34/0/2", 8: h''}]
    const uint8_t buffer2[] = {
        0x81,
            0xa2,
                0x21, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x08, 0x40
    };
    senml_cbor_test_raw("/34/0/2", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "25a");
    senml_cbor_test_raw("/34/0/2", buffer2, sizeof(buffer2), LWM2M_CONTENT_SENML_CBOR, "25b");
}

static void senml_cbor_test_26(void)
{
    /* Test missing array */
    // {-2: "/34/0/2", 3: ""}
    const uint8_t buffer[] = {
        0xa2,
            0x21, 0x67, '/', '3', '4', '/', '0', '/', '2',
            0x03, 0x60
    };
    senml_cbor_test_raw_error("/34/0/2", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "26");
}

static void senml_cbor_test_27(void)
{
    /* Test extra array */
    // [{-2: "/34/0/2", 3: ""}][{-2: "/34/0/2", 3: ""}]
    const uint8_t buffer[] = {
        0x81,
            0xa2,
                0x21, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x03, 0x60,
        0x81,
            0xa2,
                0x21, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x03, 0x60
    };
    senml_cbor_test_raw_error("/34/0/2", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "27");
}

static void senml_cbor_test_28(void)
{
    /* Test missing record */
    // [{-2: "/34/0/2", 3: ""},]
    const uint8_t buffer[] = {
        0x82,
            0xa2,
                0x21, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x03, 0x60
    };
    senml_cbor_test_raw_error("/34/0/2", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "28");
}

static void senml_cbor_test_29(void)
{
    /* Test extra record */
    // [{-2: "/34/0/2", 3: ""}]{-2: "/34/0/2", 3: ""}
    const uint8_t buffer[] = {
        0x81,
            0xa2,
                0x21, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x03, 0x60,
        0xa2,
            0x21, 0x67, '/', '3', '4', '/', '0', '/', '2',
            0x03, 0x60
    };
    senml_cbor_test_raw_error("/34/0/2", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "29");
}

static void senml_cbor_test_30(void)
{
    /* Test indefinite size array */
    // [_{-2: "/34/0/2", 3: ""},]
    const uint8_t buffer[] = {
        0x9f,
            0xa2,
                0x21, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x03, 0x60,
            0xff
    };
    senml_cbor_test_raw_error("/34/0/2", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "30");
}

static void senml_cbor_test_31(void)
{
    /* Test indefinite size record */
    // [{_-2: "/34/0/2", 3: ""},]
    const uint8_t buffer[] = {
        0x81,
            0xbf,
                0x21, 0x67, '/', '3', '4', '/', '0', '/', '2',
                0x03, 0x60,
                0xff
    };
    senml_cbor_test_raw_error("/34/0/2", buffer, sizeof(buffer), LWM2M_CONTENT_SENML_CBOR, "31");
}

static struct TestTable table[] = {
        { "test of senml_cbor_test_1()", senml_cbor_test_1 },
        { "test of senml_cbor_test_2()", senml_cbor_test_2 },
        { "test of senml_cbor_test_3()", senml_cbor_test_3 },
        { "test of senml_cbor_test_4()", senml_cbor_test_4 },
        { "test of senml_cbor_test_5()", senml_cbor_test_5 },
        { "test of senml_cbor_test_6()", senml_cbor_test_6 },
        { "test of senml_cbor_test_7()", senml_cbor_test_7 },
        { "test of senml_cbor_test_8()", senml_cbor_test_8 },
        { "test of senml_cbor_test_9()", senml_cbor_test_9 },
        { "test of senml_cbor_test_10()", senml_cbor_test_10 },
        { "test of senml_cbor_test_11()", senml_cbor_test_11 },
        { "test of senml_cbor_test_12()", senml_cbor_test_12 },
        { "test of senml_cbor_test_13()", senml_cbor_test_13 },
        { "test of senml_cbor_test_14()", senml_cbor_test_14 },
        { "test of senml_cbor_test_15()", senml_cbor_test_15 },
        { "test of senml_cbor_test_16()", senml_cbor_test_16 },
        { "test of senml_cbor_test_17()", senml_cbor_test_17 },
        { "test of senml_cbor_test_18()", senml_cbor_test_18 },
        { "test of senml_cbor_test_19()", senml_cbor_test_19 },
        { "test of senml_cbor_test_20()", senml_cbor_test_20 },
        { "test of senml_cbor_test_21()", senml_cbor_test_21 },
        { "test of senml_cbor_test_22()", senml_cbor_test_22 },
        { "test of senml_cbor_test_23()", senml_cbor_test_23 },
        { "test of senml_cbor_test_24()", senml_cbor_test_24 },
        { "test of senml_cbor_test_25()", senml_cbor_test_25 },
        { "test of senml_cbor_test_26()", senml_cbor_test_26 },
        { "test of senml_cbor_test_27()", senml_cbor_test_27 },
        { "test of senml_cbor_test_28()", senml_cbor_test_28 },
        { "test of senml_cbor_test_29()", senml_cbor_test_29 },
        { "test of senml_cbor_test_30()", senml_cbor_test_30 },
        { "test of senml_cbor_test_31()", senml_cbor_test_31 },
        { NULL, NULL },
};

CU_ErrorCode create_senml_cbor_suit()
{
   CU_pSuite pSuite = NULL;

   pSuite = CU_add_suite("Suite_SenML_CBOR", NULL, NULL);
   if (NULL == pSuite) {
      return CU_get_error();
   }

   return add_tests(pSuite, table);
}

#endif
