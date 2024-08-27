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

add_test_variant(TARGET_NAME lwm2munittests SOURCE_FILES ${TEST_SOURCES})
