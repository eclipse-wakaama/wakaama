cmake_minimum_required(VERSION 3.13)

project(lightclient C)

if(DTLS)
    message(FATAL_ERROR "DTLS option is not supported." )
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../../core/wakaama.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../coap/coap.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../data/data.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../shared/shared.cmake)

add_compile_definitions(LWM2M_CLIENT_MODE)

if(LWM2M_VERSION VERSION_GREATER "1.0")
    add_compile_definitions(LWM2M_SUPPORT_SENML_JSON)
endif()

include_directories(${WAKAAMA_HEADERS_DIR} ${COAP_HEADERS_DIR} ${DATA_HEADERS_DIR} ${WAKAAMA_SOURCES_DIR} ${SHARED_INCLUDE_DIRS})

SET(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/lightclient.c
    ${CMAKE_CURRENT_LIST_DIR}/object_security.c
    ${CMAKE_CURRENT_LIST_DIR}/object_server.c
    ${CMAKE_CURRENT_LIST_DIR}/object_device.c
    ${CMAKE_CURRENT_LIST_DIR}/object_test.c
)

add_executable(${PROJECT_NAME} ${SOURCES} ${WAKAAMA_SOURCES} ${COAP_SOURCES} ${DATA_SOURCES} ${SHARED_SOURCES})

SOURCE_GROUP(wakaama FILES ${WAKAAMA_SOURCES})
