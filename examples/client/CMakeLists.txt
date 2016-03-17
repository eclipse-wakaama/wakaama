cmake_minimum_required (VERSION 2.8.12)

SET(SOURCES
    lwm2mclient.c
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
    ../utils/commandline.c
    ../utils/commandline.h
    ../utils/output_utils.c
    ../utils/output_utils.h)

project (lwm2mclient)

if (NOT DEFINED PLATFORM)
    set(PLATFORM "Linux")
endif()

#TODO system_api.c: LWM2M_EMBEDDED_MODE definition.

include_directories(../utils)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../core" libLwm2m)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../platforms/${PLATFORM}" libplatform)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} libLwm2m_client_with_bootstrap libplatform)
