cmake_minimum_required (VERSION 2.8.3)

project (lwm2mclient)

add_definitions(-DLWM2M_CLIENT_MODE)

include_directories (${PROJECT_SOURCE_DIR}/../.. ${PROJECT_SOURCE_DIR}/../utils)

add_subdirectory(${PROJECT_SOURCE_DIR}/../../core ${CMAKE_CURRENT_BINARY_DIR}/core)

SET(SOURCES lwm2mclient.c ../utils/commandline.c object_device.c object_firmware.c test_object.c)

add_executable(lwm2mclient ${SOURCES} ${CORE_SOURCES})
