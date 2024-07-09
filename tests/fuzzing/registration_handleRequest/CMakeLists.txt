add_fuzzing_test(
    TARGET_NAME fuz_registration_handleRequest
    SOURCE_FILES
        fuz_registration_handleRequest.c
        ${CMAKE_SOURCE_DIR}/core/registration.c
        ${CMAKE_SOURCE_DIR}/core/list.c
        ${CMAKE_SOURCE_DIR}/core/utils.c
        ${CMAKE_SOURCE_DIR}/core/uri.c
        ${CMAKE_SOURCE_DIR}/core/observe.c
        ${CMAKE_SOURCE_DIR}/core/liblwm2m.c
        ${CMAKE_SOURCE_DIR}/core/packet.c
        ${CMAKE_SOURCE_DIR}/core/reporting.c
        ${CMAKE_SOURCE_DIR}/coap/er-coap-13/er-coap-13.c
        ${CMAKE_SOURCE_DIR}/coap/block.c
        ${CMAKE_SOURCE_DIR}/coap/transaction.c
        ${CMAKE_SOURCE_DIR}/examples/shared/platform.c
    COMPILE_DEFINITIONS LWM2M_SERVER_MODE LWM2M_COAP_DEFAULT_BLOCK_SIZE=64
    CRASH_FILES_DIR crash_files/
)
