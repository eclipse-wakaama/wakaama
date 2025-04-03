include_guard(DIRECTORY)

set(WAKAAMA_TOP_LEVEL_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
set(WAKAAMA_EXAMPLE_DIRECTORY "${WAKAAMA_TOP_LEVEL_DIRECTORY}/examples")
set(WAKAAMA_EXAMPLE_SHARED_DIRECTORY "${WAKAAMA_EXAMPLE_DIRECTORY}/shared")

# Mode
option(WAKAAMA_MODE_SERVER "Enable LwM2M Server interfaces" ON)
option(WAKAAMA_MODE_BOOTSTRAP_SERVER "Enable LwM2M Bootstrap Server interfaces" ON)
option(WAKAAMA_MODE_CLIENT "Enable LwM2M Client interfaces" ON)

# Client
option(WAKAAMA_CLIENT_INITIATED_BOOTSTRAP "Enable client initiated bootstrap support in a client" OFF)
option(WAKAAMA_CLIENT_LWM2M_V_1_0 "Restrict the client code to use LwM2M version 1.0" OFF)

# Data Types

option(WAKAAMA_DATA_TLV "Enable TLV payload support" ON)
option(WAKAAMA_DATA_JSON "Enable JSON payload support" ON)
option(WAKAAMA_DATA_SENML_JSON "Enable SenML JSON payload support" ON)
option(WAKAAMA_DATA_SENML_CBOR "Enable SenML CBOR payload support" ON)

option(WAKAAMA_DATA_SENML_CBOR_FLOAT16_SUPPORT "Enable 16-bit float support in CBOR" ON)

option(WAKAAMA_DATA_OLD_CONTENT_FORMAT "Support the deprecated content format values for TLV and JSON" ON)

# CoAP
option(WAKAAMA_COAP_RAW_BLOCK1_REQUESTS "Pass each unprocessed block 1 payload to the application" OFF)

set(WAKAAMA_COAP_DEFAULT_BLOCK_SIZE
    1024
    CACHE STRING "Default CoAP block size for block-wise transfers"
)
set_property(
    CACHE WAKAAMA_COAP_DEFAULT_BLOCK_SIZE
    PROPERTY STRINGS
             LOG_DISABLED
             16
             32
             64
             128
             256
             512
             1024
)

set(WAKAAMA_COAP_MAX_MESSAGE_SIZE
    2048
    CACHE STRING "Max. CoAP packet size."
)

if(WAKAAMA_COAP_DEFAULT_BLOCK_SIZE GREATER WAKAAMA_COAP_MAX_MESSAGE_SIZE)
    message(FATAL_ERROR "Packet size needs to be bigger than the block size.")
endif()

# The maximum number of retransmissions used for confirmable messages.
set(WAKAAMA_COAP_DEFAULT_MAX_RETRANSMIT
    4
    CACHE STRING "Default CoAP max retransmissions"
)

# The max time to wait between the empty ack and the separate response message.
set(WAKAAMA_COAP_SEPARATE_TIMEOUT
    15
    CACHE STRING "CoAP separate response timeout; Used if not set on a per-target basis"
)

# Logging
set(WAKAAMA_LOG_LEVEL
    LOG_DISABLED
    CACHE STRING "The lowest log level provided at build time"
)
set_property(
    CACHE WAKAAMA_LOG_LEVEL
    PROPERTY STRINGS
             LOG_DISABLED
             DBG
             INFO
             WARN
             ERR
             FATAL
)

option(WAKAAMA_LOG_CUSTOM_HANDLER "Provide a custom handler for logging messages" OFF)

set(WAKAAMA_LOG_MAX_MSG_TXT_SIZE
    200
    CACHE STRING "The buffer size for the log message (without additional data)"
)

# Transport
set(WAKAAMA_TRANSPORT
    NONE
    CACHE STRING "The transport layer implementation"
)
set_property(CACHE WAKAAMA_TRANSPORT PROPERTY STRINGS NONE POSIX_UDP TINYDTLS)

# Platform
set(WAKAAMA_PLATFORM
    NONE
    CACHE STRING "The platform abstraction layer implementation"
)
set_property(CACHE WAKAAMA_TRANSPORT PROPERTY STRINGS POSIX NONE)

# Command line interface
option(WAKAAMA_CLI "Command line interface library" OFF)

# Endianess
add_compile_definitions("$<IF:$<STREQUAL:${CMAKE_C_BYTE_ORDER},BIG_ENDIAN>,LWM2M_BIG_ENDIAN,LWM2M_LITTLE_ENDIAN>")

# Possibility to disable the examples
option(WAKAAMA_ENABLE_EXAMPLES "Build all the example applications" ON)

# Set the defines for Wakaama mode configuration
function(set_mode_defines target)
    # Mode
    if(WAKAAMA_MODE_CLIENT)
        target_compile_definitions(${target} PUBLIC LWM2M_CLIENT_MODE)
    endif()
    if(WAKAAMA_MODE_SERVER)
        target_compile_definitions(${target} PUBLIC LWM2M_SERVER_MODE)
    endif()
    if(WAKAAMA_MODE_BOOTSTRAP_SERVER)
        target_compile_definitions(${target} PUBLIC LWM2M_BOOTSTRAP_SERVER_MODE)
    endif()
endfunction()

# Set the defines for client specific configuration
function(set_client_defines target)
    if(WAKAAMA_CLIENT_INITIATED_BOOTSTRAP)
        target_compile_definitions(${target} PUBLIC LWM2M_BOOTSTRAP)
    endif()

    if(WAKAAMA_CLIENT_LWM2M_V_1_0)
        target_compile_definitions(${target} PUBLIC LWM2M_VERSION_1_0)
    endif()
endfunction()

# Set defines regarding the different data types
function(set_data_format_defines target)
    if(WAKAAMA_DATA_TLV)
        target_compile_definitions(${target} PUBLIC LWM2M_SUPPORT_TLV)
    endif()
    if(WAKAAMA_DATA_JSON)
        target_compile_definitions(${target} PUBLIC LWM2M_SUPPORT_JSON)
    endif()
    if(WAKAAMA_DATA_SENML_JSON)
        target_compile_definitions(${target} PUBLIC LWM2M_SUPPORT_SENML_JSON)
    endif()
    if(WAKAAMA_DATA_SENML_CBOR)
        target_compile_definitions(${target} PUBLIC LWM2M_SUPPORT_SENML_CBOR)
    endif()
    if(NOT WAKAAMA_DATA_SENML_CBOR_FLOAT16_SUPPORT)
        target_compile_definitions(${target} PUBLIC CBOR_NO_FLOAT16_ENCODING)
    endif()
    if(WAKAAMA_DATA_OLD_CONTENT_FORMAT)
        target_compile_definitions(${target} PUBLIC LWM2M_OLD_CONTENT_FORMAT_SUPPORT)
    endif()
endfunction()

# Set the defines related to CoAP
function(set_coap_defines)
    if(WAKAAMA_COAP_RAW_BLOCK1_REQUESTS)
        target_compile_definitions(${target} PUBLIC LWM2M_RAW_BLOCK1_REQUESTS)
    endif()

    target_compile_definitions(${target} PUBLIC LWM2M_COAP_DEFAULT_BLOCK_SIZE=${WAKAAMA_COAP_DEFAULT_BLOCK_SIZE})

    target_compile_definitions(${target} PUBLIC LWM2M_COAP_MAX_MESSAGE_SIZE=${WAKAAMA_COAP_MAX_MESSAGE_SIZE})

    target_compile_definitions(
        ${target} PUBLIC LWM2M_COAP_DEFAULT_MAX_RETRANSMIT=${WAKAAMA_COAP_DEFAULT_MAX_RETRANSMIT}
    )

    target_compile_definitions(${target} PUBLIC LWM2M_COAP_SEPARATE_TIMEOUT=${WAKAAMA_COAP_SEPARATE_TIMEOUT})
endfunction()

# Set the defines for logging configuration
function(set_logging_defines target)
    # Logging
    target_compile_definitions(${target} PUBLIC LWM2M_LOG_LEVEL=LWM2M_${WAKAAMA_LOG_LEVEL})

    if(WAKAAMA_LOG_CUSTOM_HANDLER)
        target_compile_definitions(${target} PUBLIC LWM2M_LOG_CUSTOM_HANDLER)
    endif()

    target_compile_definitions(${target} PUBLIC LWM2M_LOG_MAX_MSG_TXT_SIZE=${WAKAAMA_LOG_MAX_MSG_TXT_SIZE})
endfunction()

# Set all the requested defines on target
function(set_defines target)
    set_mode_defines(${target})
    set_client_defines(${target})
    set_coap_defines(${target})
    set_data_format_defines(${target})
    set_logging_defines(${target})
endfunction()

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
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/logging.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/management.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/objects.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/observe.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/packet.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/registration.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/uri.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/utils.c
                ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core/reporting.c
    )

    target_include_directories(${target} PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/include)

    # We should not (have to) do this!
    target_include_directories(${target} PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/core)

    # set defines
    set_defines(${target})

    # Extract pre-existing target specific definitions WARNING: Directory properties are not taken into account!
    get_target_property(CURRENT_TARGET_COMPILE_DEFINITIONS ${target} COMPILE_DEFINITIONS)

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

# Commandline library
add_library(wakaama_command_line OBJECT)
target_sources(wakaama_command_line PRIVATE ${WAKAAMA_EXAMPLE_SHARED_DIRECTORY}/commandline.c)
target_include_directories(wakaama_command_line PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/include)
target_include_directories(wakaama_command_line PUBLIC ${WAKAAMA_EXAMPLE_SHARED_DIRECTORY})

# POSIX platform library
add_library(wakaama_platform_posix OBJECT)
target_sources(wakaama_platform_posix PRIVATE ${WAKAAMA_EXAMPLE_SHARED_DIRECTORY}/platform.c)
target_include_directories(wakaama_platform_posix PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/include)
target_compile_definitions(wakaama_platform_posix PRIVATE _POSIX_C_SOURCE=200809)

# Transport UDP (POSIX) implementation library
add_library(wakaama_transport_posix_udp OBJECT)
target_sources(wakaama_transport_posix_udp PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/transport/udp/connection.c)
target_include_directories(wakaama_transport_posix_udp PUBLIC ${WAKAAMA_TOP_LEVEL_DIRECTORY}/transport/udp/include)
target_include_directories(wakaama_transport_posix_udp PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/include)
target_link_libraries(wakaama_transport_posix_udp PRIVATE wakaama_command_line)
target_compile_definitions(wakaama_transport_posix_udp PRIVATE _POSIX_C_SOURCE=200809)

# Transport 'tinydtls' implementation library
add_library(wakaama_transport_tinydtls OBJECT)
include(${WAKAAMA_TOP_LEVEL_DIRECTORY}/transport/tinydtls/tinydtls.cmake)
target_sources(wakaama_transport_tinydtls PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/transport/tinydtls/connection.c)
target_compile_definitions(wakaama_transport_tinydtls PUBLIC WITH_TINYDTLS)
target_include_directories(
    wakaama_transport_tinydtls PUBLIC ${WAKAAMA_TOP_LEVEL_DIRECTORY}/transport/tinydtls/include
                                      ${WAKAAMA_TOP_LEVEL_DIRECTORY}/transport/tinydtls/third_party
)
target_include_directories(wakaama_transport_tinydtls PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/include)
target_sources_tinydtls(wakaama_transport_tinydtls)
target_link_libraries(wakaama_transport_tinydtls PRIVATE wakaama_command_line)
target_compile_definitions(wakaama_transport_tinydtls PRIVATE _POSIX_C_SOURCE=200809)

# Transport 'testing' implementation library
add_library(wakaama_transport_testing_fake OBJECT)
target_include_directories(wakaama_transport_testing_fake PUBLIC ${WAKAAMA_TOP_LEVEL_DIRECTORY}/tests/helper/)
target_include_directories(wakaama_transport_testing_fake PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/include/)
target_sources(wakaama_transport_testing_fake PRIVATE ${WAKAAMA_TOP_LEVEL_DIRECTORY}/tests/helper/connection.c)

# Static library that users of Wakaama can link against
#
# This library simplifies building and maintaining Wakaama. It handles defines and compiler flags, adding the right
# source files and setting include directories...
add_library(wakaama_static STATIC)
target_sources_wakaama(wakaama_static)
target_include_directories(wakaama_static PUBLIC ${WAKAAMA_TOP_LEVEL_DIRECTORY}/include/)

if(WAKAAMA_TRANSPORT STREQUAL POSIX_UDP)
    target_link_libraries(wakaama_static PUBLIC wakaama_transport_posix_udp)
elseif(WAKAAMA_TRANSPORT STREQUAL TINYDTLS)
    target_link_libraries(wakaama_static PUBLIC wakaama_transport_tinydtls)
endif()

if(WAKAAMA_PLATFORM STREQUAL POSIX)
    target_link_libraries(wakaama_static PUBLIC wakaama_platform_posix)
endif()

if(WAKAAMA_CLI)
    target_link_libraries(wakaama_static PUBLIC wakaama_command_line)
endif()

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
