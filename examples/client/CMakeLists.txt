cmake_minimum_required (VERSION 2.8.3)

project (lwm2mclient)

SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

#SET(CMAKE_C_FLAGS "-g")

add_definitions(-DLWM2M_CLIENT_MODE -DWITH_LOGS -DLWM2M_LITTLE_ENDIAN -DLWM2M_BOOTSTRAP -DLWM2M_SUPPORT_JSON)

include_directories (${LIBLWM2M_DIR} ${PROJECT_SOURCE_DIR}/../utils ${PROJECT_SOURCE_DIR}/../../platforms/Linux)

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)
# SET(CMAKE_C_FLAGS "-Wall -Wextra -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wwrite-strings -Waggregate-return -Wswitch-default -Wswitch-enum")

if(DTLS)
  add_definitions(-DLWM2M_SUPPORT_DTLS -DWITH_SHA256)
  set(TINYDTLS_SOURCES_DIR ${CMAKE_CURRENT_LIST_DIR}/../../platforms/Linux/tinydtls)
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

endif()

include_directories (${TINYDTLS_SOURCES_DIR})
SET(SOURCES
    lwm2mclient.c
    ../utils/commandline.c
    ../../platforms/Linux/platform.c
    ../../platforms/Linux/memtrace.c
    system_api.c
    object_security.c
    object_server.c
    object_device.c
    object_firmware.c
    object_location.c
    object_connectivity_moni.c
    object_connectivity_stat.c
    object_access_control.c
    test_object.c)

if(DTLS)
  SET(SOURCES ${SOURCES} ../../platforms/Linux/dtlsconnection.c)
  add_executable(lwm2mclient_dtls ${SOURCES} ${TINYDTLS_SOURCES} ${CORE_SOURCES})
else()
  SET(SOURCES ${SOURCES} ../../platforms/Linux/connection.c)
  add_executable(lwm2mclient ${SOURCES} ${CORE_SOURCES})
endif()
