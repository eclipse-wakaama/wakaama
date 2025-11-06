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

void test_strncpy_null(void) {
    char dest[] = {'a', 'b', 'c'};
    size_t len = utils_strncpy(dest, sizeof(dest), NULL, 1);
    CU_ASSERT_EQUAL(len, 0);
    CU_ASSERT_NSTRING_EQUAL(dest, "abc", sizeof(dest));

    len = utils_strncpy(NULL, 99, "xyz", 1);
    CU_ASSERT_EQUAL(len, 0);
    len = utils_strncpy(NULL, 5, NULL, 3);
    CU_ASSERT_EQUAL(len, 0);
}

void test_strncpy_dest_0_src_0(void) {
    char dst[] = {'a', 'b', 'c'};
    const char *src = "xyz";
    const size_t len = utils_strncpy(dst, 0, src, 0);
    CU_ASSERT_EQUAL(len, 0);
    CU_ASSERT_NSTRING_EQUAL(dst, "abc", sizeof(dst));
}

void test_strncpy_dest_1_src_0(void) {
    char dst[] = {'a', 'b', 'c'};
    const char *src = "xyz";
    const size_t len = utils_strncpy(dst, 1, src, 0);
    CU_ASSERT_EQUAL(dst[0], '\0');
    CU_ASSERT_EQUAL(len, 0);
    CU_ASSERT_EQUAL(utils_strnlen(dst, sizeof(dst)), 0);
}

void test_strncpy_dest_sizeof_src_0(void) {
    char dst[] = {'a', 'b', 'c'};
    const char *src = "xyz";
    const size_t len = utils_strncpy(dst, sizeof(dst), src, 0);
    CU_ASSERT_EQUAL(dst[0], '\0');
    CU_ASSERT_EQUAL(len, 0);
    CU_ASSERT_EQUAL(utils_strnlen(dst, sizeof(dst)), 0);
}

void test_strncpy_dest_sizeof_src_1(void) {
    char dst[] = {'a', 'b', 'c'};
    const char *src = "xyz";
    const size_t len = utils_strncpy(dst, sizeof(dst), src, 1);
    CU_ASSERT_NSTRING_EQUAL(dst, "x", 1);
    CU_ASSERT_EQUAL(dst[1], '\0');
    CU_ASSERT_EQUAL(len, 1);
    CU_ASSERT_EQUAL(utils_strnlen(dst, sizeof(dst)), 1);
}

void test_strncpy_dest_sizeof_src_2(void) {
    char dst[] = {'a', 'b', 'c'};
    const char *src = "xyz";
    const size_t len = utils_strncpy(dst, sizeof(dst), src, 2);
    CU_ASSERT_NSTRING_EQUAL(dst, "xy", 2);
    CU_ASSERT_EQUAL(dst[2], '\0');
    CU_ASSERT_EQUAL(len, 2);
    CU_ASSERT_EQUAL(utils_strnlen(dst, sizeof(dst)), 2);
}

void test_strncpy_dest_sizeof_src_3(void) {
    char dst[] = {'a', 'b', 'c'};
    const char *src = "xyz";
    const size_t len = utils_strncpy(dst, sizeof(dst), src, 3);
    // only 2 characters and NULL has space in dst
    CU_ASSERT_NSTRING_EQUAL(dst, "xy", 2);
    CU_ASSERT_EQUAL(dst[2], '\0');
    CU_ASSERT_EQUAL(len, 2);
    CU_ASSERT_EQUAL(utils_strnlen(dst, sizeof(dst)), 2);
}

void test_strncpy_dest_sizeof_src_4(void) {
    char dst[] = {'a', 'b', 'c'};
    char src[10];
    memset(src, '\0', sizeof(src));
    src[0] = 'u';
    src[1] = 'v';
    src[2] = 'w';
    const size_t src_len = sizeof(src);
    CU_ASSERT_EQUAL(src_len, 10);
    const size_t len = utils_strncpy(dst, sizeof(dst), src, src_len);
    // only 2 characters and NULL has space in dst
    CU_ASSERT_NSTRING_EQUAL(dst, "uv", 2);
    CU_ASSERT_EQUAL(dst[2], '\0');
    CU_ASSERT_EQUAL(len, 2);
    CU_ASSERT_EQUAL(utils_strnlen(dst, sizeof(dst)), 2);
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
    {"test_strncpy_null()", test_strncpy_null},
    {"test_strncpy_dest_0_src_0()", test_strncpy_dest_0_src_0},
    {"test_strncpy_dest_1_src_0()", test_strncpy_dest_1_src_0},
    {"test_strncpy_dest_sizeof_src_0()", test_strncpy_dest_sizeof_src_0},
    {"test_strncpy_dest_sizeof_src_1()", test_strncpy_dest_sizeof_src_1},
    {"test_strncpy_dest_sizeof_src_3()", test_strncpy_dest_sizeof_src_3},
    {"test_strncpy_dest_sizeof_src_4()", test_strncpy_dest_sizeof_src_4},
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
