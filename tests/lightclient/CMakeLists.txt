cmake_minimum_required (VERSION 2.8.3)

project (lightclient)

SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

add_definitions(-DLWM2M_CLIENT_MODE -DLWM2M_LITTLE_ENDIAN)

include_directories (${LIBLWM2M_DIR} ${PROJECT_SOURCE_DIR}/../utils)

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)

SET(SOURCES
    lightclient.c
    ../utils/connection.c
    object_security.c
    object_server.c
    object_device.c
    test_object.c)

add_executable(lightclient ${SOURCES} ${CORE_SOURCES})
