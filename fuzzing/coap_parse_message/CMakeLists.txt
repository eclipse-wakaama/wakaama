add_fuzzing_test(TARGET_NAME fuz_coap_parse_message SOURCE_FILES fuz_coap_parse_message.c)
# workaround to be able to test private functions
target_include_directories(fuz_coap_parse_message PRIVATE ../../coap)
