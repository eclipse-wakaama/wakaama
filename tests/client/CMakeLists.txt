cmake_minimum_required (VERSION 2.8.3)

project (lwm2mclient)

SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

#SET(CMAKE_C_FLAGS "-g")

add_definitions(-DLWM2M_CLIENT_MODE -DWITH_LOGS -DLWM2M_LITTLE_ENDIAN -DLWM2M_BOOTSTRAP -DLWM2M_SUPPORT_JSON)

include_directories (${LIBLWM2M_DIR} ${PROJECT_SOURCE_DIR}/../utils)

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)
# SET(CMAKE_C_FLAGS "-Wall -Wextra -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wwrite-strings -Waggregate-return -Wswitch-default -Wswitch-enum")

SET(SOURCES
    lwm2mclient.c
    ../utils/commandline.c
    ../utils/connection.c
    ../../platforms/Linux/platform.c
    ../../platforms/Linux/memtrace.c
    system_api.c
    object_security.c
    object_server.c
    object_device.c
    object_firmware.c
    object_location.c
    object_connectivity_moni.c
    object_connectivity_stat.c
    object_access_control.c
    test_object.c)

add_executable(lwm2mclient ${SOURCES} ${CORE_SOURCES})
