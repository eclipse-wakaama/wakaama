cmake_minimum_required (VERSION 2.8.3)

project (lwm2mserver)

SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

add_definitions(-DLWM2M_SERVER_MODE -DWITH_LOGS -DLWM2M_LITTLE_ENDIAN)

include_directories (${LIBLWM2M_DIR} ${PROJECT_SOURCE_DIR}/../utils)

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)

# SET(CMAKE_C_FLAGS "-Wall -Wextra -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wwrite-strings -Waggregate-return -Wswitch-default -Wswitch-enum")

SET(SOURCES lwm2mserver.c ../utils/commandline.c ../utils/connection.c ../../platforms/Linux/platform.c ../../platforms/Linux/memtrace.c)

add_executable(lwm2mserver ${SOURCES} ${CORE_SOURCES})
