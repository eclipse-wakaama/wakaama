cmake_minimum_required(VERSION 3.13)

project(lwm2munittests C)

include(../wakaama.cmake)

add_executable(
    lwm2munittests
    block1tests.c
    block2tests.c
    convert_numbers_test.c
    er_coap_coap_parse_message.c
    list_tests.c
    senml_json_tests.c
    tests.h
    tlv_json_lwm2m_data_test.c
    tlvtests.c
    unittests.c
    uritests.c
)

target_link_libraries(lwm2munittests cunit)
target_compile_definitions(lwm2munittests PRIVATE LWM2M_CLIENT_MODE LWM2M_SUPPORT_TLV LWM2M_SUPPORT_JSON)

# Our tests are designed for POSIX systems
target_compile_definitions(lwm2munittests PRIVATE _POSIX_C_SOURCE=200809)

target_sources_wakaama(lwm2munittests)
target_sources_shared(lwm2munittests)

if(SANITIZER)
    target_compile_options(lwm2munittests PRIVATE -fsanitize=${SANITIZER} -fno-sanitize-recover=all)
    target_link_options(lwm2munittests PRIVATE -fsanitize=${SANITIZER} -fno-sanitize-recover=all)
endif()

if(COVERAGE)
    target_compile_options(lwm2munittests PRIVATE --coverage)
    target_link_options(lwm2munittests PRIVATE --coverage)
endif()

# Add our unit tests to the "test" target
add_test(NAME lwm2munittests_test COMMAND lwm2munittests)
