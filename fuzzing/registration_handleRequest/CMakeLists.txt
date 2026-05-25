add_fuzzing_test(TARGET_NAME fuz_registration_handleRequest SOURCE_FILES fuz_registration_handleRequest.c)
# workaround to be able to test private functions
target_include_directories(fuz_registration_handleRequest PRIVATE ../../coap ../../core)
