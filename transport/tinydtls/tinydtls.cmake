# List source files
set(TINYDTLS_SOURCES_DIR ${CMAKE_CURRENT_LIST_DIR}/third_party/tinydtls)
set(TINYDTLS_SOURCES
    ${TINYDTLS_SOURCES_DIR}/tinydtls.h
    ${TINYDTLS_SOURCES_DIR}/dtls.h
    ${TINYDTLS_SOURCES_DIR}/dtls.c
    ${TINYDTLS_SOURCES_DIR}/crypto.c
    ${TINYDTLS_SOURCES_DIR}/ccm.c
    ${TINYDTLS_SOURCES_DIR}/hmac.c
    ${TINYDTLS_SOURCES_DIR}/dtls_debug.c
    ${TINYDTLS_SOURCES_DIR}/netq.c
    ${TINYDTLS_SOURCES_DIR}/peer.c
    ${TINYDTLS_SOURCES_DIR}/dtls_prng.c
    ${TINYDTLS_SOURCES_DIR}/dtls_time.c
    ${TINYDTLS_SOURCES_DIR}/session.c
    ${TINYDTLS_SOURCES_DIR}/aes/rijndael.c
    ${TINYDTLS_SOURCES_DIR}/aes/rijndael_wrap.c
    ${TINYDTLS_SOURCES_DIR}/sha2/sha2.c
    ${TINYDTLS_SOURCES_DIR}/ecc/ecc.c
)

set(TINYDTLS_SOURCES_GENERATED ${TINYDTLS_SOURCES_DIR}/dtls_config.h)

get_filename_component(COMPILER_FILENAME "${CMAKE_C_COMPILER}" NAME)
string(REGEX REPLACE "-[^-]+$" "" TOOLCHAIN_NAME "${COMPILER_FILENAME}")

# The tinydtls configure step will create some more source files (tinydtls.h etc). Use CMake "External Project" module
# to call autoreconf and configure on tinydtls if necessary.
if(NOT EXISTS ${TINYDTLS_SOURCES_GENERATED})
    include(ExternalProject)
    ExternalProject_Add(
        external_tinydtls
        SOURCE_DIR "${TINYDTLS_SOURCES_DIR}"
        DOWNLOAD_COMMAND ""
        UPDATE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        CONFIGURE_COMMAND ${TINYDTLS_SOURCES_DIR}/configure --host=${TOOLCHAIN_NAME}
        BUILD_IN_SOURCE 1
        LOG_DOWNLOAD 1
        LOG_CONFIGURE 1
    )

    ExternalProject_Add_Step(
        external_tinydtls autoheader
        COMMAND "./autogen.sh"
        ALWAYS 1
        WORKING_DIRECTORY "${TINYDTLS_SOURCES_DIR}"
        DEPENDERS configure
    )

    add_custom_command(
        OUTPUT ${TINYDTLS_SOURCES_GENERATED}
        COMMAND "true"
        COMMENT "Let CMake know that it needs to execute the external_tinydtls target to generate those files."
        DEPENDS external_tinydtls
    )
endif()

# Prevent cryptic error messages down the road
if(NOT EXISTS ${TINYDTLS_SOURCES_DIR}/tinydtls.h)
    message(FATAL_ERROR "${target}: tinydtls support requested, but submodule not checked out")
endif()

# Add tinydtls sources to an existing target.
function(target_sources_tinydtls target)
    target_sources(${target} PRIVATE ${TINYDTLS_SOURCES} ${TINYDTLS_SOURCES_GENERATED})
    set_source_files_properties(${TINYDTLS_SOURCES} PROPERTIES COMPILE_FLAGS -Wno-cast-align)
    target_compile_definitions(${target} PRIVATE WITH_SHA256 SHA2_USE_INTTYPES_H)
    target_include_directories(${target} PUBLIC ${TINYDTLS_SOURCES_DIR})
endfunction()
