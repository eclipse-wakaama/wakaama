/*******************************************************************************
 *
 * Copyright (c) 2024 GARDENA GmbH
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
 *   Marc Lasch, GARDENA GmbH - Please refer to git log
 *
 *******************************************************************************/

#include <unistd.h>

#include "CUnit/CUnit.h"
#include "tests.h"

#include <internals.h>
#include <liblwm2m.h>
#include <message_dedup.h>

/**
 * Test insertion of duplication tracking into the dedup message list
 */
static void test_message_deduplication_tracking_entry(void) {
    int session = 0xcaffee;
    uint8_t coap_response_code = NO_ERROR;
    lwm2m_context_t context;
    context.message_dedup = NULL;

    coap_check_message_duplication(&context.message_dedup, 1, &session, &coap_response_code);
    CU_ASSERT_PTR_NOT_NULL(context.message_dedup);
    CU_ASSERT_PTR_NULL(context.message_dedup->next);
    CU_ASSERT_EQUAL(context.message_dedup->mid, 1);
    CU_ASSERT_EQUAL(*(int *)context.message_dedup->session, 0xcaffee);
    CU_ASSERT_EQUAL(context.message_dedup->coap_response_code, NO_ERROR);

    coap_deduplication_free(&context);
    CU_ASSERT_PTR_NULL(context.message_dedup);
}

/**
 * Test setting the response code for replies to duplicate messages
 */
static void test_message_deduplication_set_response_code(void) {
    int session = 0xcaffee;
    uint8_t coap_response_code = NO_ERROR;
    uint16_t mid = 123;
    lwm2m_context_t context;
    context.message_dedup = NULL;

    coap_check_message_duplication(&context.message_dedup, mid, &session, &coap_response_code);

    coap_deduplication_set_response_code(&context.message_dedup, mid, &session, COAP_205_CONTENT);

    CU_ASSERT_EQUAL(context.message_dedup->coap_response_code, COAP_205_CONTENT);

    coap_deduplication_free(&context);
    CU_ASSERT_PTR_NULL(context.message_dedup);
}

/**
 * Test duplication step
 *
 * TODO: Extend test once there is an infrastructure to manipulate time in the tests.
 */
static void test_message_deduplication_step(void) {
    int session = 0xcaffee;
    uint8_t coap_response_code = NO_ERROR;
    lwm2m_context_t context;
    context.message_dedup = NULL;

    coap_check_message_duplication(&context.message_dedup, 1, &session, &coap_response_code);

    time_t timeoutP = 10;
    coap_cleanup_message_deduplication_step(&context.message_dedup, lwm2m_gettime(), &timeoutP);
    CU_ASSERT_PTR_NOT_NULL(context.message_dedup);
    CU_ASSERT_PTR_NULL(context.message_dedup->next);
    CU_ASSERT_EQUAL(context.message_dedup->mid, 1);
    CU_ASSERT_EQUAL(*(int *)context.message_dedup->session, 0xcaffee);
    CU_ASSERT_EQUAL(context.message_dedup->coap_response_code, NO_ERROR);

    coap_deduplication_free(&context);
    CU_ASSERT_PTR_NULL(context.message_dedup);
}

static struct TestTable table[] = {
    {"test_message_deduplication_tracking_entry", test_message_deduplication_tracking_entry},
    {"test_message_deduplication_set_response_code", test_message_deduplication_set_response_code},
    {"test_message_deduplication_step", test_message_deduplication_step},
    {NULL, NULL},
};

CU_ErrorCode create_message_deduplication_suit(void) {
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("Suite_list", NULL, NULL);
    if (NULL == pSuite) {
        return CU_get_error();
    }

    return add_tests(pSuite, table);
}
