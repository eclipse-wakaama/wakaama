/*******************************************************************************
 *
 * Copyright (c) 2023 GARDENA GmbH
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

#include "CUnit/CUnit.h"
#include "internals.h"
#include "tests.h"
#include <stdio.h>
#include <unistd.h>

#ifdef LWM2M_WITH_LOGS

/* In this test suite we capture `stderr` to see if the log is reported properly. */

/* Save `stderr` to restore after capturing writes to it. */
static int stderr_bak;
/* Pipe used to read from `stderr`. */
static int pipe_fd[2];

/* Buffer for captured `stderr. */
#define CAPTURE_BUF_SIZE 100

static char capture_buf[CAPTURE_BUF_SIZE + 1];
static char *get_captured_stderr(void) {
    fflush(stderr);

    memset(capture_buf, 0, sizeof(capture_buf));
    read(pipe_fd[0], capture_buf, CAPTURE_BUF_SIZE);
    /* NULL terminate just to be safe. */
    capture_buf[100] = 0;

    return capture_buf;
}

static void setup(void) {
    stderr_bak = dup(fileno(stderr));
    pipe(pipe_fd);

    /* Stderr will now go to the pipe */
    dup2(pipe_fd[1], fileno(stderr));
}

static void teardown(void) {
    close(pipe_fd[1]);
    /* Restore `stderr`. */
    dup2(stderr_bak, fileno(stderr));
}

static void test_log(void) {
    LOG("Test");

    const char *const log_buffer = get_captured_stderr();
    CU_ASSERT_STRING_EQUAL(log_buffer, "DBG - [test_log:64] Test\n");
}

static void test_log_arg(void) {
    LOG_ARG("Hello, %s", "test");

    const char *const log_buffer = get_captured_stderr();
    CU_ASSERT_STRING_EQUAL(log_buffer, "DBG - [test_log_arg:71] Hello, test\n");
}

static void test_log_uri_null(void) {
    lwm2m_uri_t *uri = NULL;
    LOG_URI(uri);

    const char *const log_buffer = get_captured_stderr();
    CU_ASSERT_STRING_EQUAL(log_buffer, "DBG - [test_log_uri_null:79] NULL\n");
}

static void test_log_uri_empty(void) {
    lwm2m_uri_t uri;
    LWM2M_URI_RESET(&uri);

    LOG_URI(&uri);

    const char *const expected_log = "DBG - [test_log_uri_empty:89] /\n";

    const char *const log_buffer = get_captured_stderr();
    CU_ASSERT_STRING_EQUAL(log_buffer, expected_log);
}

static void test_log_uri_obj(void) {
    lwm2m_uri_t uri;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;

    LOG_URI(&uri);

    const char *const log_buffer = get_captured_stderr();
    CU_ASSERT_STRING_EQUAL(log_buffer, "DBG - [test_log_uri_obj:102] /1\n");
}

static void test_log_uri_obj_inst(void) {
    lwm2m_uri_t uri;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 2;
    uri.instanceId = 1;

    LOG_URI(&uri);

    const char *const log_buffer = get_captured_stderr();
    CU_ASSERT_STRING_EQUAL(log_buffer, "DBG - [test_log_uri_obj_inst:114] /2/1\n");
}

static void test_log_uri_obj_inst_res(void) {
    lwm2m_uri_t uri;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 3;
    uri.instanceId = 0;
    uri.resourceId = 1;

    LOG_URI(&uri);

    const char *const log_buffer = get_captured_stderr();
    CU_ASSERT_STRING_EQUAL(log_buffer, "DBG - [test_log_uri_obj_inst_res:127] /3/0/1\n");
}

static void test_log_uri_obj_inst_res_inst(void) {
    lwm2m_uri_t uri;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 5;
    uri.instanceId = 1;
    uri.resourceId = 2;
#ifndef LWM2M_VERSION_1_0
    uri.resourceInstanceId = 0;
#endif

    LOG_URI(&uri);

#ifdef LWM2M_VERSION_1_0
    const char *const expected_log = "DBG - [test_log_uri_obj_inst_res_inst:143] /5/1/2\n";
#else
    const char *const expected_log = "DBG - [test_log_uri_obj_inst_res_inst:143] /5/1/2/0\n";
#endif

    const char *const log_buffer = get_captured_stderr();
    CU_ASSERT_STRING_EQUAL(log_buffer, expected_log);
}

static struct TestTable table[] = {
    {"test_log", test_log},
    {"test_log_arg", test_log_arg},
    {"test_log_uri_null", test_log_uri_null},
    {"test_log_uri_empty", test_log_uri_empty},
    {"test_log_uri_obj", test_log_uri_obj},
    {"test_log_uri_obj_inst", test_log_uri_obj_inst},
    {"test_log_uri_obj_inst_res", test_log_uri_obj_inst_res},
    {"test_log_uri_obj_inst_res_inst", test_log_uri_obj_inst_res_inst},
    {NULL, NULL},
};

CU_ErrorCode create_logging_test_suit(void) {
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite_with_setup_and_teardown("Suite_logging", NULL, NULL, setup, teardown);
    if (NULL == pSuite) {
        return CU_get_error();
    }

    return add_tests(pSuite, table);
}

#endif /* LWM2M_WITH_LOGS */
