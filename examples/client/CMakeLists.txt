cmake_minimum_required (VERSION 3.0)

project (lwm2mclient)

option(DTLS "Enable DTLS" OFF)

include(${CMAKE_CURRENT_LIST_DIR}/../../core/wakaama.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../platforms/Linux/platform.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../utils/utils.cmake)

add_definitions(-DLWM2M_CLIENT_MODE -DLWM2M_BOOTSTRAP -DLWM2M_SUPPORT_JSON)
add_definitions(${PLATFORM_DEFINITIONS} ${WAKAAMA_DEFINITIONS})

include_directories (${WAKAAMA_SOURCES_DIR} ${PLATFORM_INCLUDE_DIRS} ${UTILS_SOURCES_DIR})

SET(SOURCES
    lwm2mclient.c
    lwm2mclient.h
    system_api.c
    object_security.c
    object_server.c
    object_device.c
    object_firmware.c
    object_location.c
    object_connectivity_moni.c
    object_connectivity_stat.c
    object_access_control.c
    test_object.c
    )

add_executable(${PROJECT_NAME} ${SOURCES} ${WAKAAMA_SOURCES} ${PLATFORM_SOURCES} ${UTILS_SOURCES})

# Add WITH_LOGS to debug variant
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>:WITH_LOGS>)
