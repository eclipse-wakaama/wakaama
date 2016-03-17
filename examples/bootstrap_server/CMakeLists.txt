cmake_minimum_required (VERSION 2.8.3)

project (bootstrap_server)

SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

add_definitions(-DLWM2M_LITTLE_ENDIAN -DWITH_LOGS -DLWM2M_BOOTSTRAP_SERVER_MODE)

include_directories (${LIBLWM2M_DIR} ${PROJECT_SOURCE_DIR}/../utils)

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)

# SET(CMAKE_C_FLAGS "-Wall -Wextra -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wwrite-strings -Waggregate-return -Wswitch-default -Wswitch-enum")

SET(SOURCES bootstrap_server.c bootstrap_info.c ../utils/commandline.c ../utils/connection.c ../../platforms/Linux/platform.c ../../platforms/Linux/memtrace.c)

add_executable(bootstrap_server ${SOURCES} ${CORE_SOURCES})
