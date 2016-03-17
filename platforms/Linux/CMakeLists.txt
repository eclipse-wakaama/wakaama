# This cmake file provides the "libPlatform" target.
# Add it to your executable with target_link_libraries(project_name libPlatform)
cmake_minimum_required (VERSION 2.8.12)

# Avoid multiple inclusions
if (TARGET libplatform)
    return()
endif()

SET(PLATFORM_SOURCES_NO_DTLS
    connection.c connection.h
    platform.c memtrace.c)

SET(PLATFORM_SOURCES_WITH_DTLS
    dtlsconnection.c dtlsconnection.h
    platform.c memtrace.c)

set(TINYDTLS_SOURCES_DIR ${CMAKE_CURRENT_LIST_DIR}/tinydtls)
set(TINYDTLS_SOURCES
    ${TINYDTLS_SOURCES_DIR}/dtls.c
    ${TINYDTLS_SOURCES_DIR}/crypto.c
    ${TINYDTLS_SOURCES_DIR}/ccm.c
    ${TINYDTLS_SOURCES_DIR}/hmac.c
    ${TINYDTLS_SOURCES_DIR}/dtls_debug.c
    ${TINYDTLS_SOURCES_DIR}/netq.c
    ${TINYDTLS_SOURCES_DIR}/peer.c
    ${TINYDTLS_SOURCES_DIR}/dtls_time.c
    ${TINYDTLS_SOURCES_DIR}/session.c
    ${TINYDTLS_SOURCES_DIR}/sha2/sha2.c
    ${TINYDTLS_SOURCES_DIR}/aes/rijndael.c
    ${TINYDTLS_SOURCES_DIR}/sha2/sha2.c
    ${TINYDTLS_SOURCES_DIR}/ecc/ecc.c)

## configure tinydtls if necessary
if (NOT EXISTS "${TINYDTLS_SOURCES_DIR}/tinydtls.h")
    # Use cmake "External Project" modul to call autoreconf and configure
    include(ExternalProject)
    ExternalProject_Add(tinydtls
        SOURCE_DIR  "${TINYDTLS_SOURCES_DIR}"
        BUILD_IN_SOURCE 1
        LOG_CONFIGURE 1
        CONFIGURE_COMMAND "${TINYDTLS_SOURCES_DIR}/configure"
        INSTALL_COMMAND "")
    ExternalProject_Add_Step(tinydtls configure2
       COMMAND "autoreconf" "-i"
       ALWAYS 1
       WORKING_DIRECTORY "${TINYDTLS_SOURCES_DIR}"
       DEPENDERS configure
    )
else()
    add_custom_target(tinydtls)
endif()

project(libplatform_with_dtls)
add_definitions(-DWITH_SHA256)
include_directories(${TINYDTLS_SOURCES_DIR} ../../core .)
add_library(${PROJECT_NAME} ${PLATFORM_SOURCES_WITH_DTLS} ${TINYDTLS_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
if(CMAKE_COMPILER_IS_GNUCC)
    target_compile_options(${PROJECT_NAME} PRIVATE --std=gnu99)
endif()
add_dependencies(${PROJECT_NAME} tinydtls)

project(libplatform)
include_directories(../../core)
add_library(${PROJECT_NAME} ${PLATFORM_SOURCES_NO_DTLS})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
add_dependencies(${PROJECT_NAME} tinydtls)
