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
#include "liblwm2m.h"
#include "tests.h"

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
    {"test_registration_with_rt", test_registration_with_rt},
    {"test_registration_without_rt", test_registration_without_rt},
    {"test_registration_with_tww_rt", test_registration_with_two_rt},
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
