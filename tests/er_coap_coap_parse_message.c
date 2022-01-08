#include "er-coap-13/er-coap-13.h"
#include "liblwm2m.h"

#include "tests.h"

static coap_packet_t coap_pkt;

static void test_regression_bugzilla_577968_1(void) {
    /* data_len does not get checked before access the first 4 bytes (header) */
    uint8_t data[] = {0x6E, 0x8D};

    CU_ASSERT_EQUAL(BAD_REQUEST_4_00, coap_parse_message(&coap_pkt, data, sizeof(data)))
}

static void test_regression_bugzilla_577968_2(void) {
    /*
     * Option number 40 triggers a buffer overflow in SET_OPTION due to an off by one error.
     * Error can be found run running with the undefined sanitizer.
     */
    uint8_t data[] = {
        0x40, // version 1, no options, no token
        0x02, // Non-empty message, PUT
        0x00, // Message ID
        0x00, // Message ID
        0xD0, // Option Delta: extended, 1 byte long; Option Length: 0 bytes
        0x1B, // Option number (40 - 13 = 27 = 0x1B) which triggers the buffer overflow
        0xFF, // Payload marker
        0xAA, // Payload
    };

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
}

static void test_empty_message(void) {
    uint8_t data[] = {
        0x40, // version 1, no options, no token
        0x00, // Empty message
        0x00, // Message ID 0x0000
        0x00,
    };

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR)
}

static void test_empty_message_with_token(void) {
    uint8_t data[] = {
        0x41, // version 1, no options, 1 byte token
        0x00, // Empty message
        0x00, // Message ID 0x0000
        0x00,
        0x00, // Token
    };

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), BAD_REQUEST_4_00)
}

static void test_empty_message_with_superfluous_data(void) {
    uint8_t data[] = {
        0x40, // version 1, no options, no token
        0x00, // Empty message
        0x00, // Message ID 0x0000
        0x00,
        0xFF, // Illegal payload marker
    };

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), BAD_REQUEST_4_00);
}

static void test_field_version_0(void) {
    uint8_t data[] = {0x00 /* Version 0 */, 0x00, 0x00, 0x00};

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), BAD_REQUEST_4_00);
}

static void test_field_version_1(void) {
    uint8_t data[] = {0x40 /* Version 1 */, 0x00, 0x00, 0x00};

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
    CU_ASSERT_EQUAL(coap_pkt.version, 1);
}

static void test_field_version_2(void) {
    uint8_t data[] = {0x80 /* Version 2*/, 0x00, 0x00, 0x00};

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), BAD_REQUEST_4_00);
}

static void test_field_version_3(void) {
    uint8_t data[] = {0xC0 /* Version 3*/, 0x00, 0x00, 0x00};

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), BAD_REQUEST_4_00);
}

static void test_field_type_confirmable(void) {
    uint8_t data[] = {0x40, 0x00, 0x00, 0x00};

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
    CU_ASSERT_EQUAL(coap_pkt.type, COAP_TYPE_CON);
}

static void test_field_type_non_confirmable(void) {
    uint8_t data[] = {0x50, 0x00, 0x00, 0x00};

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
    CU_ASSERT_EQUAL(coap_pkt.type, COAP_TYPE_NON);
}

static void test_field_type_acknowledgement(void) {
    uint8_t data[] = {0x60, 0x00, 0x00, 0x00};

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
    CU_ASSERT_EQUAL(coap_pkt.type, COAP_TYPE_ACK);
}

static void test_field_type_reset(void) {
    uint8_t data[] = {0x70, 0x00, 0x00, 0x00};

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
    CU_ASSERT_EQUAL(coap_pkt.type, COAP_TYPE_RST);
}

static void test_field_token_length_min(void) {
    uint8_t data[] = {0x40, 0x01 /* non-emtpy message */, 0x00, 0x00};

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
    CU_ASSERT_EQUAL(coap_pkt.token_len, 0);
}

static void test_field_token_length_max(void) {
    uint8_t data[] = {0x48 /* 8 bytes token */, 0x01, 0x00, 0x00,
                      /* Followed by 8 bytes of tokens */
                      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    const uint8_t expected_token[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
    CU_ASSERT_EQUAL(coap_pkt.token_len, 8);
    CU_ASSERT(memcmp(coap_pkt.token, expected_token, 8) == 0);
}

static void test_field_token_length_reserved(void) {
    uint8_t data[] = {0x49, 0x01, 0x00, 0x00,
                      /* Followed by 9 bytes wanna-be tokens */
                      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};

    CU_ASSERT_NOT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
}

static void test_payload_min(void) {
    uint8_t data[] = {
        0x41, 0x01, 0x00, 0x00, // Header
        0x11,                   // 1 byte token
        0xFF,                   // Payload marker
        0xAA,                   // Payload
    };

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
    CU_ASSERT_EQUAL(coap_pkt.payload_len, 1);
    CU_ASSERT_PTR_EQUAL(coap_pkt.payload, data + 6);
}

static void test_payload_max(void) {
    uint8_t data[UINT16_MAX] = {
        0x41, 0x01, 0x00, 0x00, // Header
        0x11,                   // 1 byte token
        0xFF,                   // Payload marker
        /* remaining 65535 - 6 = 65529 bytes of payload */
    };

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
    CU_ASSERT_EQUAL(coap_pkt.payload_len, 65529);
}

static void test_option_format_reserved_delta(void) {
    uint8_t data[] = {
        0x40, // version 1, no options, no token
        0x01, // Non-empty message
        0x00, // Message ID
        0x00, // Message ID
        0xF0, // Option Delta 15 is reserved
        /* Payload that would parse when not failing due to value above */
        0xFF, // Payload marker
        0x00, // Payload byte
    };

    CU_ASSERT_NOT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
}

static void test_option_format_reserved_length(void) {
    uint8_t data[] = {
        0x40, // version 1, no options, no token
        0x01, // Non-empty message
        0x00, // Message ID
        0x00, // Message ID
        0x0F, // Option Length 15 is reserved
        /* Payload that would parse when not failing due to value above */
        0xFF, // Payload marker
        0x01, // Payload byte
        0x02, // Payload byte
        0x03, // Payload byte
        0x04, // Payload byte
        0x05, // Payload byte
        0x06, // Payload byte
        0x07, // Payload byte
        0x08, // Payload byte
        0x09, // Payload byte
        0x0a, // Payload byte
        0x0b, // Payload byte
        0x0c, // Payload byte
        0x0d, // Payload byte
        0x0e, // Payload byte
        0x0f, // Payload byte
    };

    CU_ASSERT_NOT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
}

static void test_option_format_if_none_match(void) {
    uint8_t data[] = {
        0x40, // version 1, no options, no token
        0x02, // Non-empty message, PUT
        0x00, // Message ID
        0x00, // Message ID
        0x50, // Option If-None-Match
        0xFF, // Payload marker
        0xAA, // Payload
    };

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
    CU_ASSERT(IS_OPTION(&coap_pkt, COAP_OPTION_IF_NONE_MATCH));
}

static void test_option_format_unsupported_critical(void) {
    uint8_t data[] = {
        0x40, // version 1, no options, no token
        0x02, // Non-empty message, PUT
        0x00, // Message ID
        0x00, // Message ID
        0xD1, // Option Delta: extended, 1 byte long; Option Length: 0 bytes
        0x1A, // Option Delta: Proxy-Scheme (39 - 13 = 26 = 0x1A) is unsupported and critical
        'X',  // Option Value
        0xFF, // Payload marker
        0xAA, // Payload
    };

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), BAD_OPTION_4_02);
}

static void test_option_format_unsupported_elective(void) {
    uint8_t data[] = {
        0x40, // version 1, no options, no token
        0x02, // Non-empty message, PUT
        0x00, // Message ID
        0x00, // Message ID
        0xD0, // Option Delta: extended, 1 byte long; Option Length: 0 bytes
        0x2F, // Option Size1 (60 - 13 = 47 = 0x2F) is unsupported and elective
        0xFF, // Payload marker
        0xAA, // Payload
    };

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);
    CU_ASSERT_PTR_EQUAL(coap_pkt.payload, data + 7);
}

static void test_option_format_uri(void) {
    uint8_t data[] = {
        0x40, // version 1, no options, no token
        0x01, // Non-empty message
        0x00, // Message ID
        0x00, // Message ID
        0x3B, // Option Uri-Host
        'e',  'x',  'a', 'm', 'p', 'l', 'e', '.', 'c', 'o', 'm',
        0x42,       // +4 -> Option Uri-Port
        0x11, 0x22, // 0x1122
        0x44,       // +4 -> Uri-Path
        'p',  'a',  't', 'h',
        0x43, // +4 -> Uri-Query
        'A',  '=',  '1',
        0x04, // +0 -> Another Uri-Query
        'Z',  '=',  '2', '6',
    };

    CU_ASSERT_EQUAL(coap_parse_message(&coap_pkt, data, sizeof(data)), NO_ERROR);

    CU_ASSERT(IS_OPTION(&coap_pkt, COAP_OPTION_URI_HOST));
    CU_ASSERT(memcmp(coap_pkt.uri_host, "example.com", 11) == 0);

    CU_ASSERT(IS_OPTION(&coap_pkt, COAP_OPTION_URI_PORT));
    CU_ASSERT_EQUAL(coap_pkt.uri_port, 0x1122);

    CU_ASSERT(IS_OPTION(&coap_pkt, COAP_OPTION_URI_PATH));
    char *path = coap_get_multi_option_as_path_string(coap_pkt.uri_path);
    CU_ASSERT(memcmp(path, "/path", 4) == 0);
    lwm2m_free(path);

    CU_ASSERT(IS_OPTION(&coap_pkt, COAP_OPTION_URI_QUERY));
    char *query_id = coap_get_multi_option_as_query_string(coap_pkt.uri_query);
    CU_ASSERT(memcmp(query_id, "?A=1&Z=26", 9) == 0);
    lwm2m_free(query_id);

    coap_free_header(&coap_pkt);
}

static struct TestTable table[] = {
    {"Regression: Bugzilla #577968 #1", test_regression_bugzilla_577968_1},
    {"Regression: Bugzilla #577968 #2", test_regression_bugzilla_577968_2},
    {"Empty message", test_empty_message},
    {"Empty message with illegal extra data", test_empty_message_with_superfluous_data},
    {"Empty message with illegal token", test_empty_message_with_token},
    {"Field Version: 0", test_field_version_0},
    {"Field Version: 1", test_field_version_1},
    {"Field Version: 2", test_field_version_2},
    {"Field Version: 3", test_field_version_3},
    {"Field Type: Confirmable", test_field_type_confirmable},
    {"Field Type: Non-confirmable", test_field_type_non_confirmable},
    {"Field Type: Acknowledgement", test_field_type_acknowledgement},
    {"Field Type: Reset", test_field_type_reset},
    {"Field Token Length: Minimum", test_field_token_length_min},
    {"Field Token Length: Maximum", test_field_token_length_max},
    {"Field Token Length: Reserved", test_field_token_length_reserved},
    {"Option format: Reserved option delta", test_option_format_reserved_delta},
    {"Option format: Reserved option length", test_option_format_reserved_length},
    /* Option tests are not really exhaustive */
    {"Option format: URI", test_option_format_uri},
    {"Option format: If none match", test_option_format_if_none_match},
    {"Option format: Unsupported, elective", test_option_format_unsupported_elective},
    {"Option format: Unsupported, critical", test_option_format_unsupported_critical},
    {"Payload: Minimal", test_payload_min},
    {"Payload: Maximal", test_payload_max},
    {NULL, NULL},
};

CU_ErrorCode create_er_coap_parse_message_suit(void) {
    CU_pSuite pSuite = CU_add_suite("Suite_CoapParseMessage", NULL, NULL);

    if (NULL == pSuite) {
        return CU_get_error();
    }

    return add_tests(pSuite, table);
}
