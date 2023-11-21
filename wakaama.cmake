set(WAKAAMA_TOP_LEVEL_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
set(WAKAAMA_EXAMPLE_DIRECTORY "${WAKAAMA_TOP_LEVEL_DIRECTORY}/examples")
set(WAKAAMA_EXAMPLE_SHARED_DIRECTORY "${WAKAAMA_EXAMPLE_DIRECTORY}/shared")

# Add data format source files to an existing target.
#
# Separated from target_sources_wakaama() for testability reasons.
function(target_sources_data target)
    target_sources(
        ${target}
        PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/data/data.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/data/json.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/data/json_common.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/data/senml_json.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/data/tlv.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/data/cbor.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/data/senml_common.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/data/senml_cbor.c
    )
endfunction()

# Add CoAP source files to an existing target.
#
# Separated from target_sources_wakaama() for testability reasons.
function(target_sources_coap target)
    target_sources(
        ${target}
        PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/coap/block.c ${WAKAAMA_TOP_LEVEL_DIRECTORY}/coap/er-coap-13/er-coap-13.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/coap/transaction.c
    )
    # We should not (have to) do this!
    target_include_directories(${target} PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/coap)
endfunction()

# Add Wakaama source files to an existing target.
#
# The following definitions are needed and default values get applied if not set:
#
# - LWM2M_COAP_DEFAULT_BLOCK_SIZE
# - Either LWM2M_LITTLE_ENDIAN or LWM2M_BIG_ENDIAN
function(target_sources_wakaama target)
    target_sources(
        ${target}
        PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/bootstrap.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/discover.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/internals.h
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/liblwm2m.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/list.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/management.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/objects.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/observe.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/packet.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/registration.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/uri.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/utils.c
    )

    target_include_directories(${target} PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/include)

    # We should not (have to) do this!
    target_include_directories(${target} PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core)

    # Extract pre-existing target specific definitions WARNING: Directory properties are not taken into account!
    get_target_property(CURRENT_TARGET_COMPILE_DEFINITIONS ${target} COMPILE_DEFINITIONS)

    if(NOT CURRENT_TARGET_COMPILE_DEFINITIONS MATCHES "LWM2M_LITTLE_ENDIAN|LWM2M_BIG_ENDIAN")
        # Replace TestBigEndian once we require CMake 3.20+
        include(TestBigEndian)
        test_big_endian(machine_is_big_endian)
        if(machine_is_big_endian)
            target_compile_definitions(${target} PRIVATE LWM2M_BIG_ENDIAN)
            message(STATUS "${target}: Endiannes not set, defaulting to big endian")
        else()
            target_compile_definitions(${target} PRIVATE LWM2M_LITTLE_ENDIAN)
            message(STATUS "${target}: Endiannes not set, defaulting to little endian")
        endif()
    endif()

    # LWM2M_COAP_DEFAULT_BLOCK_SIZE is needed by source files -> always set it
    if(NOT CURRENT_TARGET_COMPILE_DEFINITIONS MATCHES "LWM2M_COAP_DEFAULT_BLOCK_SIZE=")
        target_compile_definitions(${target} PRIVATE "LWM2M_COAP_DEFAULT_BLOCK_SIZE=${LWM2M_COAP_DEFAULT_BLOCK_SIZE}")
        message(STATUS "${target}: Default CoAP block size not set, using ${LWM2M_COAP_DEFAULT_BLOCK_SIZE}")
    endif()

    # Detect invalid configuration already during CMake run
    if(NOT CURRENT_TARGET_COMPILE_DEFINITIONS MATCHES "LWM2M_SERVER_MODE|LWM2M_BOOTSTRAP_SERVER_MODE|LWM2M_CLIENT_MODE")
        message(FATAL_ERROR "${target}: At least one mode (client, server, bootstrap server) must be enabled!")
    endif()

    target_sources_coap(${target})
    target_sources_data(${target})
endfunction()

# Add shared source files to an existing target.
function(target_sources_shared target)
    get_target_property(TARGET_PROPERTY_CONN_IMPL ${target} CONNECTION_IMPLEMENTATION)

    target_sources(
        ${target} PRIVATE ${WAKAAMA_EXAMPLE_SHARED_DIRECTORY}/commandline.c
                          ${WAKAAMA_EXAMPLE_SHARED_DIRECTORY}/platform.c
    )

    if(NOT TARGET_PROPERTY_CONN_IMPL)
        target_sources(${target} PRIVATE ${WAKAAMA_EXAMPLE_SHARED_DIRECTORY}/connection.c)
    elseif(TARGET_PROPERTY_CONN_IMPL MATCHES "tinydtls")
        include(${WAKAAMA_EXAMPLE_SHARED_DIRECTORY}/tinydtls.cmake)
        target_sources(${target} PRIVATE ${WAKAAMA_EXAMPLE_SHARED_DIRECTORY}/dtlsconnection.c)
        target_compile_definitions(${target} PRIVATE WITH_TINYDTLS)
        target_sources_tinydtls(${target})
    elseif(TARGET_PROPERTY_CONN_IMPL MATCHES "testing")
        target_include_directories(${target} PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/tests/helper/)
        target_sources(${target} PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/tests/helper/connection.c)
    else()
        message(
            FATAL_ERROR "${target}: Unknown connection (DTLS) implementation '${TARGET_PROPERTY_CONN_IMPL} requested"
        )
    endif()

    target_include_directories(${target} PUBLIC ${WAKAAMA_EXAMPLE_SHARED_DIRECTORY})
endfunction()

# Enforce a certain level of hygiene
add_compile_options(
    -Waggregate-return
    -Wall
    -Wcast-align
    -Wextra
    -Wfloat-equal
    -Wpointer-arith
    -Wshadow
    -Wswitch-default
    -Wwrite-strings
    -pedantic
    # Reduce noise: Unused parameters are common in this ifdef-littered code-base, but of no danger
    -Wno-unused-parameter
    # Reduce noise: Too many false positives
    -Wno-uninitialized
    # Turn (most) warnings into errors
    -Werror
    # Disabled because of existing, non-trivially fixable code
    -Wno-error=cast-align
)

# The maximum buffer size that is provided for resource responses and must be respected due to the limited IP buffer.
# Larger data must be handled by the resource and will be sent chunk-wise through a TCP stream or CoAP blocks. Block
# size is set to 1024 bytes if not specified otherwise to avoid block transfers in common use cases.
set(LWM2M_COAP_DEFAULT_BLOCK_SIZE
    1024
    CACHE STRING "Default CoAP block size; Used if not set on a per-target basis"
)
