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
#include "helper/log_handler.h"
#include "internals.h"
#include "tests.h"
#include <stdio.h>
#include <unistd.h>

#ifdef LWM2M_WITH_LOGS

#ifndef LWM2M_LOG_CUSTOM_HANDLER
#error These tests expect to use a custom logging handles to capture the log messages!
#endif

static void setup(void) { test_log_handler_clear_captured_message(); }

static void teardown(void) { test_log_handler_clear_captured_message(); }

static void test_log(void) {
    LOG("Test");

    const char *const log_buffer = test_log_handler_get_captured_message();
    CU_ASSERT_STRING_EQUAL(log_buffer, "DBG - [test_log] Test\n");
}

static void test_log_arg(void) {
    LOG_ARG("Hello, %s", "test");

    const char *const log_buffer = test_log_handler_get_captured_message();
    CU_ASSERT_STRING_EQUAL(log_buffer, "DBG - [test_log_arg] Hello, test\n");
}

static void test_log_uri_null(void) {
    lwm2m_uri_t *uri = NULL;
    LOG_URI(uri);

    const char *const log_buffer = test_log_handler_get_captured_message();
    CU_ASSERT_STRING_EQUAL(log_buffer, "DBG - [test_log_uri_null] NULL\n");
}

static void test_log_uri_empty(void) {
    lwm2m_uri_t uri;
    LWM2M_URI_RESET(&uri);

    LOG_URI(&uri);

    const char *const expected_log = "DBG - [test_log_uri_empty] /\n";
    const char *const log_buffer = test_log_handler_get_captured_message();
    CU_ASSERT_STRING_EQUAL(log_buffer, expected_log);
}

static void test_log_uri_obj(void) {
    lwm2m_uri_t uri;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 1;

    LOG_URI(&uri);

    const char *const log_buffer = test_log_handler_get_captured_message();
    CU_ASSERT_STRING_EQUAL(log_buffer, "DBG - [test_log_uri_obj] /1\n");
}

static void test_log_uri_obj_inst(void) {
    lwm2m_uri_t uri;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 2;
    uri.instanceId = 1;

    LOG_URI(&uri);

    const char *const log_buffer = test_log_handler_get_captured_message();
    CU_ASSERT_STRING_EQUAL(log_buffer, "DBG - [test_log_uri_obj_inst] /2/1\n");
}

static void test_log_uri_obj_inst_res(void) {
    lwm2m_uri_t uri;
    LWM2M_URI_RESET(&uri);
    uri.objectId = 3;
    uri.instanceId = 0;
    uri.resourceId = 1;

    LOG_URI(&uri);

    const char *const log_buffer = test_log_handler_get_captured_message();
    CU_ASSERT_STRING_EQUAL(log_buffer, "DBG - [test_log_uri_obj_inst_res] /3/0/1\n");
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
    const char *const expected_log = "DBG - [test_log_uri_obj_inst_res_inst] /5/1/2\n";
#else
    const char *const expected_log = "DBG - [test_log_uri_obj_inst_res_inst] /5/1/2/0\n";
#endif

    const char *const log_buffer = test_log_handler_get_captured_message();
    CU_ASSERT_STRING_EQUAL(log_buffer, expected_log);
}

static void test_log_level(void) {
    LOG_L(DBG, "debug");
    LOG_L(INFO, "info");
    LOG_L(WARN, "warning");
    LOG_L(ERR, "error");
    LOG_L(FATAL, "fatal");
    const char * const log_buffer = test_log_handler_get_captured_message();
    const char * const expected_log = "DBG - [test_log_level] debug\n"
                                      "INFO - [test_log_level] info\n"
                                      "WARN - [test_log_level] warning\n"
                                      "ERR - [test_log_level] error\n"
                                      "FATAL - [test_log_level] fatal\n";
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
    {"test_log_level", test_log_level},
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
