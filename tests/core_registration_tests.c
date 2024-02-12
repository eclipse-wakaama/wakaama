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
#include "connection.h"
#include "er-coap-13/er-coap-13.h"
#include "internals.h"
#include "liblwm2m.h"
#include "tests.h"

#include <string.h>

#ifdef LWM2M_SERVER_MODE

#define ASSERT_RESPONSE_HEADER(packet, expected_code)                                                                  \
    do {                                                                                                               \
        CU_ASSERT_EQUAL(packet.version, 1);                                                                            \
        CU_ASSERT_EQUAL(packet.type, COAP_TYPE_ACK);                                                                   \
        CU_ASSERT_EQUAL(packet.code, expected_code);                                                                   \
        CU_ASSERT_EQUAL(packet.mid, 0xbeef);                                                                           \
        CU_ASSERT_EQUAL(packet.token_len, 5);                                                                          \
        const char *expected_token = "token";                                                                          \
        CU_ASSERT(memcmp(packet.token, expected_token, strlen(expected_token)) == 0);                                  \
    } while (0)

#define ASSERT_RESPONSE_LOCATION_PATH(packet, path)                                                                    \
    do {                                                                                                               \
        CU_ASSERT_PTR_NOT_NULL_FATAL(packet.location_path);                                                            \
        const size_t expected_path_len = strlen(path);                                                                 \
        CU_ASSERT_EQUAL(packet.location_path->len, expected_path_len);                                                 \
        CU_ASSERT(memcmp(packet.location_path->data, path, expected_path_len) == 0);                                   \
    } while (0)

#define ASSERT_RESPONSE_PAYLOAD_EMPTY(packet)                                                                          \
    do {                                                                                                               \
        CU_ASSERT_PTR_NULL(packet.payload);                                                                            \
        CU_ASSERT_EQUAL(packet.payload_len, 0);                                                                        \
    } while (0)

static void create_test_registration_message(coap_packet_t *packet, const lwm2m_media_type_t content_type) {
    coap_init_message(packet, COAP_TYPE_CON, COAP_POST, 0xbeef);
    uint8_t token[] = {'t', 'o', 'k', 'e', 'n'};
    coap_set_header_token(packet, token, sizeof(token));
    coap_set_header_uri_path(packet, "rd");
    coap_set_header_content_type(packet, content_type);
    coap_set_header_uri_query(packet, "b=U&lwm2m=1.1&lt=300&ep=dummy-client");

    char const *payload = "</1/0>,</3/0>;ver=1.2";
    coap_set_payload(packet, payload, strlen(payload));
}

static void test_registration_message_to_server(void) {
    /* arrange */
    coap_packet_t packet;
    memset(&packet, 0, sizeof(packet));
    create_test_registration_message(&packet, LWM2M_CONTENT_LINK);

    uint8_t reg_coap_msg[80];
    memset(reg_coap_msg, 0, sizeof(reg_coap_msg));
    size_t msg_size = coap_serialize_message(&packet, reg_coap_msg);
    CU_ASSERT_EQUAL(msg_size, 74);
    coap_free_header(&packet);

    /* act */
    lwm2m_context_t *const server_ctx = lwm2m_init(NULL);
    lwm2m_handle_packet(server_ctx, reg_coap_msg, msg_size, NULL);
    lwm2m_close(server_ctx);

    /* assert */
    size_t send_buffer_len;
    uint8_t *send_buffer = test_get_response_buffer(&send_buffer_len);
    coap_packet_t actual_response_packet;
    CU_ASSERT_EQUAL(14, send_buffer_len);
    coap_status_t status = coap_parse_message(&actual_response_packet, send_buffer, send_buffer_len);
    CU_ASSERT_EQUAL(status, NO_ERROR);

    ASSERT_RESPONSE_HEADER(actual_response_packet, COAP_201_CREATED);
    ASSERT_RESPONSE_LOCATION_PATH(actual_response_packet, "rd");
    ASSERT_RESPONSE_PAYLOAD_EMPTY(actual_response_packet);

    coap_free_header(&actual_response_packet);
}

static void init_test_packet(coap_packet_t *message, const char *registration_message, uint8_t *payload,
                             const size_t payload_len) {
    int ret = 0;

    memset(message, 0, sizeof(coap_packet_t));
    message->code = COAP_POST;
    ret = coap_set_header_uri_query(message, "lwm2m=1.1&ep=abc");
    CU_ASSERT(ret > 0);

    memcpy(payload, registration_message, payload_len);
    message->payload = payload;
    message->payload_len = payload_len;

    message->content_type = (coap_content_type_t)LWM2M_CONTENT_LINK;
}

void init_uri(lwm2m_uri_t *const uri) {
    memset(uri, 0, sizeof(*uri));
    uri->objectId = LWM2M_MAX_ID;
}

static int call_test_function(const char *reg_msg, const size_t reg_msg_len) {
    int ret;

    uint8_t user_data[1] = {0};
    lwm2m_context_t *context = lwm2m_init(user_data);

    lwm2m_uri_t uri;
    init_uri(&uri);
    coap_packet_t message;

    uint8_t payload[reg_msg_len];
    init_test_packet(&message, reg_msg, payload, reg_msg_len);

    coap_packet_t response;
    memset(&response, 0, sizeof(coap_packet_t));

    ret = registration_handleRequest(context, &uri, NULL, &message, &response);

    free_multi_option(message.uri_query);
    free_multi_option(response.location_path);

    lwm2m_close(context);

    return ret;
}

static void test_registration_with_rt(void) {
    static const char *registration_message = "</>;rt=\"oma.lwm2m\";ct=110,</28180>;ver=0.4,</28152>;ver=0.2,"
                                              "</30000>;ver=0.1,</28181>;ver=0.2,</28183>;ver=0.1,"
                                              "</28182>;ver=0.2,</1>,</3>;ver=1.0,</4>;ver=1.2,</5>;ver=1.0 ";
    int ret = call_test_function(registration_message, strlen(registration_message));
    CU_ASSERT_EQUAL(ret, COAP_201_CREATED);
}

static void test_registration_without_rt(void) {
    static const char *registration_message = "</>;ct=110,</28180>;ver=0.4,</28152>;ver=0.2,"
                                              "</30000>;ver=0.1,</28181>;ver=0.2,</28183>;ver=0.1,"
                                              "</28182>;ver=0.2,</1>,</3>;ver=1.0,</4>;ver=1.2,</5>;ver=1.0 ";
    int ret = call_test_function(registration_message, strlen(registration_message));
    CU_ASSERT_EQUAL(ret, COAP_201_CREATED);
}

static void test_registration_with_two_rt(void) {
    static const char *registration_message =
        "</>;rt=\"oma.lwm2m\";rt=\"oma.lwm2m\";ct=110,</28180>;ver=0.4,</28152>;ver=0.2,"
        "</30000>;ver=0.1,</28181>;ver=0.2,</28183>;ver=0.1,"
        "</28182>;ver=0.2,</1>,</3>;ver=1.0,</4>;ver=1.2,</5>;ver=1.0 ";
    int ret = call_test_function(registration_message, strlen(registration_message));
    CU_ASSERT_EQUAL(ret, COAP_400_BAD_REQUEST);
}

static struct TestTable table[] = {
    {"test_registration_message_to_server", test_registration_message_to_server},
    {"test_registration_with_rt", test_registration_with_rt},
    {"test_registration_without_rt", test_registration_without_rt},
    {"test_registration_with_two_rt", test_registration_with_two_rt},
    {NULL, NULL},
};

CU_ErrorCode create_registration_test_suit(void) {
    CU_pSuite pSuite = NULL;

    pSuite = CU_add_suite("Suite_list", NULL, NULL);
    if (NULL == pSuite) {
        return CU_get_error();
    }

    return add_tests(pSuite, table);
}

#endif /* LWM2M_SERVER_MODE */
