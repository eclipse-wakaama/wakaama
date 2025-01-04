/*******************************************************************************
 *
 * Copyright (c) 2025 GARDENA GmbH
 *
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
 *   Lukas Woodtli, GARDENA GmbH - Please refer to git log
 *
 *******************************************************************************/
#include "CUnit/Basic.h"
#include "tests.h"
#include "utils.h"

void test_strnlen_null(void) {
    size_t len = utils_strnlen(NULL, 10);
    CU_ASSERT_EQUAL(len, 0);
}

void test_strnlen_0(void) {
    const char *string = "";
    size_t len = utils_strnlen(string, 10);
    CU_ASSERT_EQUAL(len, 0);
}

void test_strnlen_1(void) {
    const char *string = "a";
    size_t len = utils_strnlen(string, 10);
    CU_ASSERT_EQUAL(len, 1);
}

void test_strnlen_2(void) {
    const char *string = "ab";
    size_t len = utils_strnlen(string, 10);
    CU_ASSERT_EQUAL(len, 2);
}

void test_strnlen_max_len_0(void) {
    const char *string = "abc";
    size_t len = utils_strnlen(string, 0);
    CU_ASSERT_EQUAL(len, 0);
}

void test_strnlen_max_len_1(void) {
    const char *string = "abc";
    size_t len = utils_strnlen(string, 1);
    CU_ASSERT_EQUAL(len, 1);
}

void test_strnlen_max_len_2(void) {
    const char *string = "abc";
    size_t len = utils_strnlen(string, 2);
    CU_ASSERT_EQUAL(len, 2);
}

void test_strnlen_max_len_3(void) {
    const char *string = "abc";
    size_t len = utils_strnlen(string, 3);
    CU_ASSERT_EQUAL(len, 3);
}

void test_strnlen_max_len_4(void) {
    const char *string = "abc";
    size_t len = utils_strnlen(string, 4);
    CU_ASSERT_EQUAL(len, 3);
}

void test_strnlen_max_len_5(void) {
    const char *string = "abc";
    size_t len = utils_strnlen(string, 5);
    CU_ASSERT_EQUAL(len, 3);
}

static struct TestTable table[] = {
    {"test_strnlen_null()", test_strnlen_null},
    {"test_strnlen_0()", test_strnlen_0},
    {"test_strnlen_1()", test_strnlen_1},
    {"test_strnlen_2()", test_strnlen_2},
    {"test_strnlen_max_len_0()", test_strnlen_max_len_0},
    {"test_strnlen_max_len_1()", test_strnlen_max_len_1},
    {"test_strnlen_max_len_2()", test_strnlen_max_len_2},
    {"test_strnlen_max_len_3()", test_strnlen_max_len_3},
    {"test_strnlen_max_len_4()", test_strnlen_max_len_4},
    {"test_strnlen_max_len_5()", test_strnlen_max_len_5},
    {NULL, NULL},
};

CU_ErrorCode create_utils_suit(void) {
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("Suite_utils", NULL, NULL);
    if (NULL == pSuite) {
        return CU_get_error();
    }

    return add_tests(pSuite, table);
}
