cmake_minimum_required (VERSION 2.8.3)

project (lightclient)

SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

add_definitions(-DLWM2M_CLIENT_MODE -DLWM2M_LITTLE_ENDIAN)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
add_definitions(-DLWM2M_WITH_LOGS)
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

include_directories (${LIBLWM2M_DIR} ${PROJECT_SOURCE_DIR}/../utils)

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)

SET(SOURCES
    lightclient.c
    ../utils/connection.c
    ../../platforms/Linux/platform.c
    ../../platforms/Linux/memtrace.c
    object_security.c
    object_server.c
    object_device.c
    test_object.c)

add_executable(lightclient ${SOURCES} ${CORE_SOURCES})
