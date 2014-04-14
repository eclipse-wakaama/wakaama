cmake_minimum_required (VERSION 2.8.3)

project (lwm2mserver)

SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

add_definitions(-DLWM2M_SERVER_MODE)

include_directories (${LIBLWM2M_DIR} ${PROJECT_SOURCE_DIR}/../utils)

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)

SET(SOURCES lwm2mserver.c ../utils/commandline.c ../utils/connection.c)

add_executable(lwm2mserver ${SOURCES} ${CORE_SOURCES})
