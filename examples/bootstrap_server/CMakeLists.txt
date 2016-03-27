cmake_minimum_required (VERSION 2.8.3)

project (bootstrap_server)

include(${CMAKE_CURRENT_LIST_DIR}/../../core/wakaama.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../platforms/Linux/platform.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../utils/utils.cmake)

set(CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG} -DWITH_LOGS)
add_definitions(-DLWM2M_BOOTSTRAP_SERVER_MODE)
add_definitions(${PLATFORM_DEFINITIONS} ${WAKAAMA_DEFINITIONS})

include_directories (${WAKAAMA_SOURCES_DIR} ${PLATFORM_INCLUDE_DIRS} ${UTILS_SOURCES_DIR})

SET(SOURCES
    bootstrap_server.c
    bootstrap_info.c
    bootstrap_info.h
    )

SET(AUXILIARY_FILES
    bootstrap_server.ini)

add_executable(${PROJECT_NAME} ${SOURCES} ${AUXILIARY_FILES} ${WAKAAMA_SOURCES} ${PLATFORM_SOURCES} ${UTILS_SOURCES})
