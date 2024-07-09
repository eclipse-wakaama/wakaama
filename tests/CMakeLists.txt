cmake_minimum_required(VERSION 3.13)

project(lwm2munittests C)

include(../wakaama.cmake)
include(tests.cmake)

set(TEST_SOURCES
    helper/log_handler.c
    coap_block1tests.c
    coap_block2tests.c
    coap_parse_message.c
    data_cbor_tests.c
    core_convert_numbers_test.c
    core_list_tests.c
    core_logging_tests.c
    core_registration_tests.c
    core_uritests.c
    data_senml_cbor_tests.c
    data_senml_json_tests.c
    data_tlv_json_lwm2m_data_test.c
    data_tlvtests.c
    tests.h
    unittests.c
)

if(WAKAAMA_MODE_CLIENT AND NOT WAKAAMA_MODE_BOOTSTRAP_SERVER)
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
endif()

add_test_variant(
    TARGET_NAME lwm2munittests_server_formats_no_float16_blocksize_64
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS LWM2M_SERVER_MODE LWM2M_OLD_CONTENT_FORMAT_SUPPORT LWM2M_SUPPORT_SENML_JSON
                        LWM2M_SUPPORT_SENML_CBOR CBOR_NO_FLOAT16_ENCODING COAP_DEFAULT_BLOCK_SIZE=64
)

add_test_variant(
    TARGET_NAME lwm2munittests_server
    SOURCE_FILES ${TEST_SOURCES}
    COMPILE_DEFINITIONS LWM2M_SERVER_MODE
)

if(NOT WAKAAMA_CLIENT_INITIATED_BOOTSTRAP)
    add_test_variant(
        TARGET_NAME lwm2munittests_client_server_bootstrap_all_formats
        SOURCE_FILES ${TEST_SOURCES}
        COMPILE_DEFINITIONS
            LWM2M_CLIENT_MODE
            LWM2M_SERVER_MODE
            LWM2M_BOOTSTRAP_SERVER_MODE
            LWM2M_SERVER_MODE
            LWM2M_OLD_CONTENT_FORMAT_SUPPORT
            LWM2M_SUPPORT_SENML_JSON
            LWM2M_SUPPORT_SENML_CBOR
    )
endif()

add_subdirectory(fuzzing)
