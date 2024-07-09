add_fuzzing_test(
    TARGET_NAME fuz_coap_parse_message
    SOURCE_FILES fuz_coap_parse_message.c ${CMAKE_SOURCE_DIR}/coap/er-coap-13/er-coap-13.c
                 ${CMAKE_SOURCE_DIR}/examples/shared/platform.c
)
