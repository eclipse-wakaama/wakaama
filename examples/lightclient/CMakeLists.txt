cmake_minimum_required (VERSION 2.8.3)

project (lightclient)

include(${CMAKE_CURRENT_LIST_DIR}/../../core/wakaama.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../platforms/Linux/platform.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../utils/utils.cmake)

set(CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG} -DWITH_LOGS)
add_definitions(-DLWM2M_CLIENT_MODE)
add_definitions(${PLATFORM_DEFINITIONS} ${WAKAAMA_DEFINITIONS})

include_directories (${WAKAAMA_SOURCES_DIR} ${PLATFORM_INCLUDE_DIRS} ${UTILS_SOURCES_DIR})

SET(SOURCES
    lightclient.c
    object_security.c
    object_server.c
    object_device.c
    test_object.c
    )

add_executable(${PROJECT_NAME} ${SOURCES} ${WAKAAMA_SOURCES} ${PLATFORM_SOURCES} ${UTILS_SOURCES})

