cmake_minimum_required (VERSION 2.8.3)

project (lwm2mclient)

include_directories ("${PROJECT_SOURCE_DIR}/..")

add_subdirectory(${PROJECT_SOURCE_DIR}/../core ${CMAKE_CURRENT_BINARY_DIR}/core)

SET(SOURCES lwm2mclient.c object_device.c test_object.c)

add_executable(lwm2mclient ${SOURCES} ${CORE_SOURCES})
