cmake_minimum_required (VERSION 2.6)

project (tlvtests)

SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

include_directories (${LIBLWM2M_DIR})

add_definitions(-DLWM2M_LITTLE_ENDIAN -DLWM2M_SUPPORT_JSON)

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)

SET(SOURCES decode.c)
# SET(CMAKE_C_FLAGS "-Wall -Wextra -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wwrite-strings -Waggregate-return -Wswitch-default -Wswitch-enum")

add_executable(tlvtests ${SOURCES} ${CORE_SOURCES})
