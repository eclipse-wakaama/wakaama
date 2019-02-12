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
double floats[]={12, -114 , -30 , 1.02 , 134.000235 , 0.43243 , 0, -21.5025, -0.0925, 0.98765, 6.667e-11, FLT_MIN, FLT_MAX, DBL_MIN, DBL_MAX};
const char* floats_text[] = {"12.0","-114.0","-30.0", "1.02", "134.000235","0.43243","0.0","-21.5025","-0.0925","0.98765", "0.00000000006667", "0.00000000000000000000000000000000000001175494", "340282346638528859811704183484516925440.0", "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002225073858", "179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0"};

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

        converted = utils_textToFloat((uint8_t*)floats_text[i], strlen(floats_text[i]), &res);

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
        len = utils_floatToText(floats[i], (uint8_t*)res, sizeof(res));

        CU_ASSERT(len);
        if (len)
        {
            compareLen = (int)strlen(floats_text[i]);
            if (compareLen > DBL_DIG
                && floats_text[i][compareLen-2] == '.'
                && len > compareLen - 2
                && res[compareLen-2] == '.')
            {
                compareLen = DBL_DIG;
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
    len = utils_floatToText(val, (uint8_t*)res, 6);
    CU_ASSERT(len);
    if(len)
    {
        CU_ASSERT_NSTRING_EQUAL(res, "0.0000", len);
        if (strncmp(res, "0.0000", len))
            printf("%zu \"%g\" -> fail (%*s)\n", i, val, len, res);
    }
    else
    {
        printf("%zu \"%g\" -> fail\n", i, val);
    }
    i++;

    /* Test when only some significant digits fit */
    val = 0.11111111111111111;
    len = utils_floatToText(val, (uint8_t*)res, 6);
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

static struct TestTable table[] = {
        { "test of utils_textToInt()", test_utils_textToInt },
        { "test of utils_textToUInt()", test_utils_textToUInt },
        { "test of utils_textToFloat()", test_utils_textToFloat },
        { "test of utils_intToText()", test_utils_intToText },
        { "test of utils_uintToText()", test_utils_uintToText },
        { "test of utils_floatToText()", test_utils_floatToText },
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


