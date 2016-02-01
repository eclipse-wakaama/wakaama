cmake_minimum_required (VERSION 2.8.3)

project (secureclient)

SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

add_definitions(-DLWM2M_CLIENT_MODE -DLWM2M_LITTLE_ENDIAN -DLWM2M_BOOTSTRAP)

include_directories (${LIBLWM2M_DIR} ${PROJECT_SOURCE_DIR}/../utils)

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)

set(TINYDTLS_SOURCES_DIR ${CMAKE_CURRENT_LIST_DIR}/../utils/tinydtls)
set(TINYDTLS_SOURCES
	${TINYDTLS_SOURCES_DIR}/dtls.c
    ${TINYDTLS_SOURCES_DIR}/crypto.c
    ${TINYDTLS_SOURCES_DIR}/ccm.c
    ${TINYDTLS_SOURCES_DIR}/hmac.c
    ${TINYDTLS_SOURCES_DIR}/debug.c
    ${TINYDTLS_SOURCES_DIR}/netq.c
    ${TINYDTLS_SOURCES_DIR}/peer.c
    ${TINYDTLS_SOURCES_DIR}/dtls_time.c
    ${TINYDTLS_SOURCES_DIR}/session.c
    ${TINYDTLS_SOURCES_DIR}/sha2/sha2.c
    ${TINYDTLS_SOURCES_DIR}/aes/rijndael.c
    ${TINYDTLS_SOURCES_DIR}/sha2/sha2.c
    ${TINYDTLS_SOURCES_DIR}/ecc/ecc.c)

include_directories (${TINYDTLS_SOURCES_DIR})

SET(SOURCES
    secureclient.c
    ../utils/dtlsconnection.c
    ../../platforms/Linux/platform.c
    ../../platforms/Linux/memtrace.c
    object_security.c
    object_server.c
    object_device.c
    test_object.c)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "-std=gnu99") 
add_definitions(-DWITH_SHA256)

add_executable(secureclient ${SOURCES} ${TINYDTLS_SOURCES} ${CORE_SOURCES})
