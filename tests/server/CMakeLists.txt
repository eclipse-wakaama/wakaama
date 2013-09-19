cmake_minimum_required (VERSION 2.8.3)

project (lwm2mserver)

include_directories (${PROJECT_SOURCE_DIR}/../.. ${PROJECT_SOURCE_DIR}/../utils)

add_subdirectory(${PROJECT_SOURCE_DIR}/../../core ${CMAKE_CURRENT_BINARY_DIR}/core)

SET(SOURCES lwm2mserver.c ../utils/commandline.c)

add_executable(lwm2mserver ${SOURCES} ${CORE_SOURCES})
