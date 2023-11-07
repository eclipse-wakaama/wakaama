cmake_minimum_required(VERSION 3.13)

project(lwm2munittests C)

include(../wakaama.cmake)
include(tests.cmake)

set(TEST_SOURCES
    block1tests.c
    block2tests.c
    cbor_tests.c
    convert_numbers_test.c
    er_coap_coap_parse_message.c
    registration_tests.c
    list_tests.c
    senml_json_tests.c
    senml_cbor_tests.c
    tests.h
    tlv_json_lwm2m_data_test.c
    tlvtests.c
    unittests.c
    uritests.c
)

add_test_variant(
    TARGET_NAME lwm2munittests
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS LWM2M_CLIENT_MODE
)

add_test_variant(
    TARGET_NAME lwm2munittests_client_lwm2m_1_0
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS LWM2M_CLIENT_MODE LWM2M_VERSION_1_0
)

add_test_variant(
    TARGET_NAME lwm2munittests_client_bootstrap
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS LWM2M_CLIENT_MODE LWM2M_BOOTSTRAP
)

add_test_variant(
    TARGET_NAME lwm2munittests_server
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS LWM2M_SERVER_MODE LWM2M_WITH_LOGS
)

add_test_variant(
    TARGET_NAME lwm2munittests_server_formats
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS LWM2M_SERVER_MODE LWM2M_OLD_CONTENT_FORMAT_SUPPORT LWM2M_SUPPORT_SENML_JSON
                        LWM2M_SUPPORT_SENML_CBOR
)

add_test_variant(
    TARGET_NAME lwm2munittests_server_formats_no_float16
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS LWM2M_SERVER_MODE LWM2M_OLD_CONTENT_FORMAT_SUPPORT LWM2M_SUPPORT_SENML_JSON
                        LWM2M_SUPPORT_SENML_CBOR CBOR_NO_FLOAT16_ENCODING
)

add_test_variant(
    TARGET_NAME lwm2munittests_bootstrap_server
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS LWM2M_BOOTSTRAP_SERVER_MODE
)

add_test_variant(
    TARGET_NAME lwm2munittests_server_and_bootstrap_server
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS LWM2M_SERVER_MODE LWM2M_BOOTSTRAP_SERVER_MODE
)

add_test_variant(
    TARGET_NAME lwm2munittests_server_blocksize_16
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS LWM2M_CLIENT_MODE COAP_DEFAULT_BLOCK_SIZE=16
)

add_test_variant(
    TARGET_NAME lwm2munittests_server_blocksize_64
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS LWM2M_SERVER_MODE COAP_DEFAULT_BLOCK_SIZE=64
)

add_test_variant(
    TARGET_NAME lwm2munittests_server_blocksize_1024
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS LWM2M_SERVER_MODE COAP_DEFAULT_BLOCK_SIZE=1024
)

add_test_variant(
    TARGET_NAME lwm2munittests_client_server_bootstrap_all_formats
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS
        LWM2M_CLIENT_MODE
        LWM2M_SERVER_MODE
        LWM2M_BOOTSTRAP_SERVER_MODE
        LWM2M_SERVER_MODE
        LWM2M_WITH_LOGS
        LWM2M_OLD_CONTENT_FORMAT_SUPPORT
        LWM2M_SUPPORT_SENML_JSON
        COAP_DEFAULT_BLOCK_SIZE=32
)
