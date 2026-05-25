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
    lwm2m_context_t context;
    context.message_dedup = NULL;
    uint8_t *msg_buffer;
    size_t msg_buffer_len;

    coap_check_message_duplication(&context.message_dedup, 1, &session, &msg_buffer, &msg_buffer_len);
    CU_ASSERT_PTR_NOT_NULL(context.message_dedup);
    CU_ASSERT_PTR_NULL(context.message_dedup->next);
    CU_ASSERT_EQUAL(context.message_dedup->mid, 1);
    CU_ASSERT_EQUAL(*(int *)context.message_dedup->session, 0xcaffee);
    CU_ASSERT_PTR_NULL(context.message_dedup->full_response);
    CU_ASSERT_EQUAL(context.message_dedup->full_response_len, 0);

    coap_deduplication_free(&context);
    CU_ASSERT_PTR_NULL(context.message_dedup);
}

static void create_test_response(coap_packet_t *packet) {
    coap_init_message(packet, COAP_TYPE_ACK, COAP_205_CONTENT, 0xbeef);
    uint8_t token[] = {'t', 'o', 'k', 'e', 'n'};
    coap_set_header_token(packet, token, sizeof(token));
    coap_set_header_uri_path(packet, "/999/888");
    coap_set_header_content_type(packet, LWM2M_CONTENT_TEXT);

    char const *payload = "deadbeef";
    coap_set_payload(packet, payload, strlen(payload)); // NOSONAR
}

/**
 * Test setting the response code for replies to duplicate messages
 */
static void test_message_deduplication_set_response_code(void) {
    int session = 0xcaffee;
    uint16_t mid = 123;
    lwm2m_context_t context;
    context.message_dedup = NULL;
    uint8_t *msg_buffer;
    size_t msg_buffer_len;
    coap_packet_t response;

    create_test_response(&response);

    coap_check_message_duplication(&context.message_dedup, mid, &session, &msg_buffer, &msg_buffer_len);

    coap_deduplication_set_response(&context.message_dedup, mid, &session, &response);

    CU_ASSERT_PTR_NOT_NULL(context.message_dedup->full_response);
    CU_ASSERT_EQUAL(context.message_dedup->full_response_len, 27);

    coap_packet_t parsed_response;
    memset(&parsed_response, 0, sizeof(parsed_response));
    coap_status_t status = coap_parse_message(&parsed_response, context.message_dedup->full_response,
                                              context.message_dedup->full_response_len);
    CU_ASSERT_EQUAL(status, NO_ERROR);

    CU_ASSERT_EQUAL(parsed_response.code, COAP_205_CONTENT);

    CU_ASSERT_EQUAL(parsed_response.token_len, 5);
    CU_ASSERT_NSTRING_EQUAL(parsed_response.token, "token", 5);

    CU_ASSERT_PTR_NOT_NULL(parsed_response.uri_path);
    CU_ASSERT_EQUAL(parsed_response.uri_path->len, 3);
    CU_ASSERT_NSTRING_EQUAL(parsed_response.uri_path->data, "999", 3);

    CU_ASSERT_PTR_NOT_NULL(parsed_response.uri_path->next);
    CU_ASSERT_EQUAL(parsed_response.uri_path->next->len, 3);
    CU_ASSERT_NSTRING_EQUAL(parsed_response.uri_path->next->data, "888", 3);

    CU_ASSERT_PTR_NULL(parsed_response.uri_path->next->next);

    char const *const expected_payload = "deadbeef";
    CU_ASSERT_PTR_NOT_NULL(parsed_response.payload);
    CU_ASSERT_EQUAL(parsed_response.payload_len, strlen(expected_payload));                       // NOSONAR
    CU_ASSERT_NSTRING_EQUAL(parsed_response.payload, expected_payload, strlen(expected_payload)); // NOSONAR

    coap_free_header(&parsed_response);
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
    lwm2m_context_t context;
    context.message_dedup = NULL;
    context.message_dedup = NULL;
    uint8_t *msg_buffer;
    size_t msg_buffer_len;

    coap_check_message_duplication(&context.message_dedup, 1, &session, &msg_buffer, &msg_buffer_len);

    time_t timeoutP = 10;
    coap_cleanup_message_deduplication_step(&context.message_dedup, lwm2m_gettime(), &timeoutP);
    CU_ASSERT_PTR_NOT_NULL(context.message_dedup);
    CU_ASSERT_PTR_NULL(context.message_dedup->next);
    CU_ASSERT_EQUAL(context.message_dedup->mid, 1);
    CU_ASSERT_EQUAL(*(int *)context.message_dedup->session, 0xcaffee);
    CU_ASSERT_EQUAL(context.message_dedup->full_response_len, 0);

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
