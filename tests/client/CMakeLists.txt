cmake_minimum_required (VERSION 2.8.3)

project (lwm2mclient)

SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

add_definitions(-DLWM2M_CLIENT_MODE)

include_directories (${LIBLWM2M_DIR} ${PROJECT_SOURCE_DIR}/../utils)

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)

SET(SOURCES lwm2mclient.c ../utils/commandline.c ../utils/connection.c object_device.c object_firmware.c test_object.c)

add_executable(lwm2mclient ${SOURCES} ${CORE_SOURCES})
