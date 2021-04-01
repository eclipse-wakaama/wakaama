cmake_minimum_required(VERSION 3.13)

project(bootstrap_server C)

if(DTLS)
    message(FATAL_ERROR "DTLS option is not supported.")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../../core/wakaama.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../coap/coap.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../data/data.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../shared/shared.cmake)

add_compile_definitions(LWM2M_BOOTSTRAP_SERVER_MODE)

include_directories(${WAKAAMA_HEADERS_DIR} ${COAP_HEADERS_DIR} ${DATA_HEADERS_DIR} ${WAKAAMA_SOURCES_DIR} ${SHARED_INCLUDE_DIRS})

SET(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/bootstrap_server.c
    ${CMAKE_CURRENT_LIST_DIR}/bootstrap_info.c
    ${CMAKE_CURRENT_LIST_DIR}/bootstrap_info.h
)

SET(AUXILIARY_FILES ${CMAKE_CURRENT_LIST_DIR}/bootstrap_server.ini)

add_executable(${PROJECT_NAME} ${SOURCES} ${AUXILIARY_FILES} ${WAKAAMA_SOURCES} ${COAP_SOURCES} ${DATA_SOURCES} ${SHARED_SOURCES})

SOURCE_GROUP(wakaama FILES ${WAKAAMA_SOURCES})
