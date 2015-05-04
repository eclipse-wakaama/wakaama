cmake_minimum_required (VERSION 2.6)

project (tlvdecode)

SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

include_directories (${LIBLWM2M_DIR} ${PROJECT_SOURCE_DIR}/../utils)

add_definitions(-DLWM2M_LITTLE_ENDIAN)

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)

SET(SOURCES decode.c ../utils/commandline.c)

add_executable(tlvdecode ${SOURCES} ${CORE_SOURCES})
