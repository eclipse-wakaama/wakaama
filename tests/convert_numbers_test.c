/*******************************************************************************
 *
 * Copyright (c) 2015 Intel Corporation and others.
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
 *    David Graeff - Make this a test suite
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *    
 *******************************************************************************/

#include "internals.h"
#include "tests.h"
#include "CUnit/Basic.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>
#include <float.h>

int64_t ints[]={12, -114 , 1 , 134 , 43243 , 0, -215025, INT64_MIN, INT64_MAX};
const char* ints_text[] = {"12","-114","1", "134", "43243","0","-215025", "-9223372036854775808", "9223372036854775807"};
uint64_t uints[]={12, 1 , 134 , 43243 , 0, UINT64_MAX};
const char* uints_text[] = {"12","1", "134", "43243","0","18446744073709551615"};
double floats[]={12, -114 , -30 , 1.02 , 134.000235 , 0.43243 , 0, -21.5025, -0.0925, 0.98765, 6.667e-11, 56.789, -52.0006, FLT_MIN, FLT_MAX, DBL_MIN, DBL_MAX};
const char* floats_text[] = {"12.0","-114.0","-30.0", "1.02", "134.000235","0.43243","0.0","-21.5025","-0.0925","0.98765", "0.00000000006667", "56.789", "-52.0006", "0.00000000000000000000000000000000000001175494", "340282346638528859811704183484516925440.0", "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002225073858", "179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0"};
const char* floats_exponential[] = {"12.0","-114.0","-30.0", "1.02", "134.000235","0.43243","0.0","-21.5025","-0.0925","0.98765", "6.667e-11", "56.789", "-52.0006", "1.17549435082229e-38", "3.40282346638529e38", "2.2250738585072e-308", "1.7976931348623e308"};

static void test_utils_textToInt(void)
{
    size_t i;

    for (i = 0 ; i < sizeof(ints)/sizeof(ints[0]) ; i++)
    {
        int64_t res;
        int converted;

        converted = utils_textToInt((uint8_t*)ints_text[i], strlen(ints_text[i]), &res);

        CU_ASSERT(converted);
        if (converted)
        {
            CU_ASSERT_EQUAL(res, ints[i]);
            if (res != ints[i])
                printf("%zu \"%s\" -> fail (%" PRId64 ")\n", i, ints_text[i], res);
        }
        else
        {
            printf("%zu \"%s\" -> fail\n", i, ints_text[i]);
        }
    }
}

static void test_utils_textToUInt(void)
{
    size_t i;

    for (i = 0 ; i < sizeof(uints)/sizeof(uints[0]) ; i++)
    {
        uint64_t res;
        int converted;

        converted = utils_textToUInt((uint8_t*)uints_text[i], strlen(uints_text[i]), &res);

        CU_ASSERT(converted);
        if (converted)
        {
            CU_ASSERT_EQUAL(res, uints[i]);
            if (res != uints[i])
                printf("%zu \"%s\" -> fail (%" PRIu64 ")\n", i, uints_text[i], res);
        }
        else
        {
            printf("%zu \"%s\" -> fail\n", i, uints_text[i]);
        }
    }
}

static void test_utils_textToFloat(void)
{
    size_t i;

    for (i = 0 ; i < sizeof(floats)/sizeof(floats[0]) ; i++)
    {
        double res;
        int converted;

        converted = utils_textToFloat((uint8_t*)floats_text[i], strlen(floats_text[i]), &res, false);

        CU_ASSERT(converted);
        if (converted)
        {
            CU_ASSERT_DOUBLE_EQUAL(res, floats[i], floats[i]/1000000.0);
            if(fabs(res - floats[i]) > fabs(floats[i]/1000000.0))
                printf("%zu \"%s\" -> fail (%f)\n", i, floats_text[i], res);
        }
        else
        {
            printf("%zu \"%s\" -> fail\n", i, floats_text[i]);
        }
    }
}

static void test_utils_textToFloatExponential(void)
{
    size_t i;

    for (i = 0 ; i < sizeof(floats)/sizeof(floats[0]) ; i++)
    {
        double res;
        int converted;

        converted = utils_textToFloat((uint8_t*)floats_exponential[i], strlen(floats_exponential[i]), &res, true);

        CU_ASSERT(converted);
        if (converted)
        {
            CU_ASSERT_DOUBLE_EQUAL(res, floats[i], floats[i]/1000000.0);
            if(fabs(res - floats[i]) > fabs(floats[i]/1000000.0))
                printf("%zu \"%s\" -> fail (%f)\n", i, floats_exponential[i], res);
        }
        else
        {
            printf("%zu \"%s\" -> fail\n", i, floats_exponential[i]);
        }
    }
}

static void test_utils_textToObjLink(void)
{
    uint16_t objectId;
    uint16_t objectInstanceId;
    CU_ASSERT_EQUAL(utils_textToObjLink(NULL, 0, &objectId, &objectInstanceId), 0);
    CU_ASSERT_EQUAL(utils_textToObjLink((uint8_t*)"", 0, &objectId, &objectInstanceId), 0);
    CU_ASSERT_EQUAL(utils_textToObjLink((uint8_t*)":", 1, &objectId, &objectInstanceId), 0);
    CU_ASSERT_EQUAL(utils_textToObjLink((uint8_t*)"0:", 2, &objectId, &objectInstanceId), 0);
    CU_ASSERT_EQUAL(utils_textToObjLink((uint8_t*)":0", 2, &objectId, &objectInstanceId), 0);
    CU_ASSERT_EQUAL(utils_textToObjLink((uint8_t*)"0:0", 3, &objectId, &objectInstanceId), 1);
    CU_ASSERT_EQUAL(objectId, 0);
    CU_ASSERT_EQUAL(objectInstanceId, 0);
    CU_ASSERT_EQUAL(utils_textToObjLink((uint8_t*)"1:2", 3, &objectId, &objectInstanceId), 1);
    CU_ASSERT_EQUAL(objectId, 1);
    CU_ASSERT_EQUAL(objectInstanceId, 2);
    CU_ASSERT_EQUAL(utils_textToObjLink((uint8_t*)"65535:65535", 11, &objectId, &objectInstanceId), 1);
    CU_ASSERT_EQUAL(objectId, 65535);
    CU_ASSERT_EQUAL(objectInstanceId, 65535);
    CU_ASSERT_EQUAL(utils_textToObjLink((uint8_t*)"65536:65535", 11, &objectId, &objectInstanceId), 0);
    CU_ASSERT_EQUAL(utils_textToObjLink((uint8_t*)"65535:65536", 11, &objectId, &objectInstanceId), 0);
}

static void test_utils_intToText(void)
{
    size_t i;

    for (i = 0 ; i < sizeof(ints)/sizeof(ints[0]); i++)
    {
        char res[24];
        int len;

        len = utils_intToText(ints[i], (uint8_t*)res, sizeof(res));

        CU_ASSERT(len);
        CU_ASSERT_NSTRING_EQUAL(res, ints_text[i], strlen(ints_text[i]));
        if (!len)
            printf("%zu \"%" PRId64 "\" -> fail\n", i, ints[i]);
        else if (strncmp(res, ints_text[i], strlen(ints_text[i])))
            printf("%zu \"%" PRId64 "\" -> fail (%s)\n", i, ints[i], res);
    }
}

static void test_utils_uintToText(void)
{
    size_t i;

    for (i = 0 ; i < sizeof(uints)/sizeof(uints[0]); i++)
    {
        char res[24];
        int len;

        len = utils_uintToText(uints[i], (uint8_t*)res, sizeof(res));

        CU_ASSERT(len);
        CU_ASSERT_NSTRING_EQUAL(res, uints_text[i], strlen(uints_text[i]));
        if (!len)
            printf("%zu \"%" PRIu64 "\" -> fail\n", i, uints[i]);
        else if (strncmp(res, uints_text[i], strlen(uints_text[i])))
            printf("%zu \"%" PRIu64 "\" -> fail (%s)\n", i, uints[i], res);
    }
}

static void test_utils_floatToText(void)
{
    size_t i;
    char res[330];
    int len;
    int compareLen;

    for (i = 0 ; i < sizeof(floats)/sizeof(floats[0]); i++)
    {
        len = utils_floatToText(floats[i], (uint8_t*)res, sizeof(res), false);

        CU_ASSERT(len);
        if (len)
        {
            compareLen = (int)strlen(floats_text[i]);
            if (compareLen > DBL_DIG - 1
                && floats_text[i][compareLen-2] == '.'
                && len > compareLen - 2
                && res[compareLen-2] == '.')
            {
                compareLen = DBL_DIG - 1;
            }
            CU_ASSERT_NSTRING_EQUAL(res, floats_text[i], compareLen);
            if (strncmp(res, floats_text[i], compareLen))
                printf("%zu \"%g\" -> fail (%*s)\n", i, floats[i], len, res);
        }
        else
        {
            printf("%zu \"%g\" -> fail\n", i, floats[i]);
        }
    }

    /* Test when no significant digits fit */
    double val = 1e-9;
    len = utils_floatToText(val, (uint8_t*)res, 6, false);
    CU_ASSERT(len == 3);
    if(len)
    {
        CU_ASSERT_NSTRING_EQUAL(res, "0.0", len);
        if (strncmp(res, "0.0", len))
            printf("%zu \"%g\" -> fail (%*s)\n", i, val, len, res);
    }
    else
    {
        printf("%zu \"%g\" -> fail\n", i, val);
    }
    i++;

    /* Test when only some significant digits fit */
    val = 0.11111111111111111;
    len = utils_floatToText(val, (uint8_t*)res, 6, false);
    CU_ASSERT(len);
    if(len)
    {
        CU_ASSERT_NSTRING_EQUAL(res, "0.1111", len);
        if (strncmp(res, "0.1111", len))
            printf("%zu \"%g\" -> fail (%*s)\n", i, val, len, res);
    }
    else
    {
        printf("%zu \"%g\" -> fail\n", i, val);
    }
}

static void test_utils_floatToTextExponential(void)
{
    size_t i;
    char res[24];
    int len;
    int compareLen;

    for (i = 0 ; i < sizeof(floats)/sizeof(floats[0]); i++)
    {
        compareLen = (int)strlen(floats_exponential[i]);
        len = utils_floatToText(floats[i], (uint8_t*)res, sizeof(res), true);

        CU_ASSERT(len >= compareLen);
        if (len)
        {
            CU_ASSERT_NSTRING_EQUAL(res, floats_exponential[i], compareLen);
            if (strncmp(res, floats_exponential[i], compareLen))
                printf("%zu \"%g\" -> fail (%*s)\n", i, floats[i], len, res);
        }
        else
        {
            printf("%zu \"%g\" -> fail\n", i, floats[i]);
        }
    }

    /* Test when no significant digits */
    double val = DBL_MIN/FLT_RADIX;
    CU_ASSERT_NOT_EQUAL(val, 0);
    len = utils_floatToText(val, (uint8_t*)res, 6, true);
    CU_ASSERT(len == 3);
    if(len)
    {
        CU_ASSERT_NSTRING_EQUAL(res, "0.0", 3);
        if (strncmp(res, "0.0", 3))
            printf("%zu \"%g\" -> fail (%.*s)\n", i, val, len, res);
    }
    else
    {
        printf("%zu \"%g\" -> fail\n", i, val);
    }
    i++;

    /* Tests when only some significant digits fit */
    val = 0.11111111111111111;
    len = utils_floatToText(val, (uint8_t*)res, 6, true);
    CU_ASSERT(len == 6);
    if(len)
    {
        CU_ASSERT_NSTRING_EQUAL(res, "0.1111", 6);
        if (strncmp(res, "0.1111", 6))
            printf("%zu \"%g\" -> fail (%.*s)\n", i, val, len, res);
    }
    else
    {
        printf("%zu \"%g\" -> fail\n", i, val);
    }
    i++;

    val = 1.1111111111111111e-6;
    len = utils_floatToText(val, (uint8_t*)res, 6, true);
    CU_ASSERT(len == 6);
    if(len)
    {
        CU_ASSERT_NSTRING_EQUAL(res, "1.1e-6", 6);
        if (strncmp(res, "1.1e-6", 6))
            printf("%zu \"%g\" -> fail (%.*s)\n", i, val, len, res);
    }
    else
    {
        printf("%zu \"%g\" -> fail\n", i, val);
    }
    i++;

    val = 1.99e-6;
    len = utils_floatToText(val, (uint8_t*)res, 6, true);
    CU_ASSERT(len == 6);
    if(len)
    {
        CU_ASSERT_NSTRING_EQUAL(res, "2.0e-6", 6);
        if (strncmp(res, "2.0e-6", 6))
            printf("%zu \"%g\" -> fail (%.*s)\n", i, val, len, res);
    }
    else
    {
        printf("%zu \"%g\" -> fail\n", i, val);
    }
}

static void test_utils_objLinkToText(void)
{
    uint8_t text[12];
    CU_ASSERT_EQUAL(utils_objLinkToText(0, 0, text, sizeof(text)), 3);
    CU_ASSERT_NSTRING_EQUAL(text, "0:0", 3);
    CU_ASSERT_EQUAL(utils_objLinkToText(0, 0, text, 3), 3);
    CU_ASSERT_NSTRING_EQUAL(text, "0:0", 3);
    CU_ASSERT_EQUAL(utils_objLinkToText(10, 20, text, sizeof(text)), 5);
    CU_ASSERT_NSTRING_EQUAL(text, "10:20", 5);
    CU_ASSERT_EQUAL(utils_objLinkToText(65535, 65535, text, sizeof(text)), 11);
    CU_ASSERT_NSTRING_EQUAL(text, "65535:65535", 11);
    CU_ASSERT_EQUAL(utils_objLinkToText(0, 0, NULL, 0), 0);
    CU_ASSERT_EQUAL(utils_objLinkToText(0, 0, text, 0), 0);
    CU_ASSERT_EQUAL(utils_objLinkToText(0, 0, text, 1), 0);
    CU_ASSERT_EQUAL(utils_objLinkToText(0, 0, text, 2), 0);
    CU_ASSERT_EQUAL(utils_objLinkToText(0, 0, text, 3), 3);
}

static void test_utils_base64_1(const uint8_t *binary, size_t binaryLength, const char *base64)
{
    uint8_t encodeBuffer[8];
    uint8_t decodeBuffer[6];
    size_t base64Length = strlen(base64);
    memset(encodeBuffer, 0, sizeof(encodeBuffer));
    memset(decodeBuffer, 0xFF, sizeof(decodeBuffer));
    CU_ASSERT_EQUAL(utils_base64GetSize(binaryLength), base64Length);
    CU_ASSERT_EQUAL(utils_base64GetDecodedSize(base64, base64Length), binaryLength);
    CU_ASSERT_EQUAL(utils_base64Encode(binary, binaryLength, encodeBuffer, sizeof(encodeBuffer)), base64Length);
    CU_ASSERT_NSTRING_EQUAL(encodeBuffer, base64, base64Length);
    CU_ASSERT_EQUAL(utils_base64Decode(base64, base64Length, decodeBuffer, sizeof(decodeBuffer)), binaryLength);
    CU_ASSERT_EQUAL(memcmp(decodeBuffer, binary, binaryLength), 0);
}

static void test_utils_base64(void)
{
    uint8_t binary[] = { 0, 1, 2, 3, 4, 5 };
    const char * base64[] = { "AA==", "AAE=", "AAEC", "AAECAw==", "AAECAwQ=", "AAECAwQF" };
    size_t i;
    for (i = 0; i < sizeof(binary); i++)
    {
        test_utils_base64_1(binary, i+1, base64[i]);
    }
}

static struct TestTable table[] = {
        { "test of utils_textToInt()", test_utils_textToInt },
        { "test of utils_textToUInt()", test_utils_textToUInt },
        { "test of utils_textToFloat()", test_utils_textToFloat },
        { "test of utils_textToFloat(exponential)", test_utils_textToFloatExponential },
        { "test of utils_textToObjLink()", test_utils_textToObjLink },
        { "test of utils_intToText()", test_utils_intToText },
        { "test of utils_uintToText()", test_utils_uintToText },
        { "test of utils_floatToText()", test_utils_floatToText },
        { "test of utils_floatToText(exponential)", test_utils_floatToTextExponential },
        { "test of utils_objLinkToText()", test_utils_objLinkToText },
        { "test of base64 functions", test_utils_base64 },
        { NULL, NULL },
};

CU_ErrorCode create_convert_numbers_suit()
{
   CU_pSuite pSuite = NULL;

   pSuite = CU_add_suite("Suite_ConvertNumbers", NULL, NULL);
   if (NULL == pSuite) {
      return CU_get_error();
   }

   return add_tests(pSuite, table);
}


