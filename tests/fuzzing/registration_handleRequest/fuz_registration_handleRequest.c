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

#include "er-coap-13/er-coap-13.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "internals.h"
#include "liblwm2m.h"

bool lwm2m_session_is_equal(void *session1, void *session2, void *userData) {
    (void)userData;

    return (session1 == session2);
}

uint8_t lwm2m_buffer_send(void *sessionH, uint8_t *buffer, size_t length, void *userdata) {
    (void)sessionH;
    (void)buffer;
    (void)length;
    (void)userdata;

    return COAP_NO_ERROR;
}

static void init_test_packet(coap_packet_t *message, const char *registration_message, uint8_t *payload,
                             const size_t payload_len) {

    memset(message, 0, sizeof(coap_packet_t));
    message->code = COAP_POST;
    coap_set_header_uri_query(message, "lwm2m=1.1&ep=abc");

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

    uint8_t payload[reg_msg_len == 0 ? 1 : reg_msg_len];
    init_test_packet(&message, reg_msg, payload, reg_msg_len);

    coap_packet_t response;
    memset(&response, 0, sizeof(coap_packet_t));

    ret = registration_handleRequest(context, &uri, NULL, &message, &response);

    free_multi_option(message.uri_query);
    free_multi_option(response.location_path);

    lwm2m_close(context);

    return ret;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {

    call_test_function((const char *)data, size);

    return 0;
}
