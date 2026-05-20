add_fuzzing_test(
    TARGET_NAME fuz_registration_handleRequest
    SOURCE_FILES fuz_registration_handleRequest.c
    CRASH_FILES_DIR crash_files/
)
# workaround to be able to test private functions
target_include_directories(fuz_registration_handleRequest PRIVATE ../../coap ../../core)
