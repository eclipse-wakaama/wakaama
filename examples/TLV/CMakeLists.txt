cmake_minimum_required (VERSION 2.6)

project (tlvdecode)

SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

include_directories (${LIBLWM2M_DIR} ${PROJECT_SOURCE_DIR}/../utils)

add_definitions(-DLWM2M_LITTLE_ENDIAN -DLWM2M_SUPPORT_JSON)

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)

SET(SOURCES decode.c ../utils/commandline.c ../utils/connection.c  ../../platforms/Linux/platform.c ../../platforms/Linux/memtrace.c)

add_executable(tlvdecode ${SOURCES} ${CORE_SOURCES})
