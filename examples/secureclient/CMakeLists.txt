cmake_minimum_required (VERSION 2.8.12)

SET(SOURCES
    secureclient.c
    object_security.c
    object_server.c
    object_device.c
    test_object.c
    ../utils/commandline.c
    ../utils/commandline.h)

project (secureclient)

if (NOT DEFINED PLATFORM)
    set(PLATFORM "Linux")
endif()

include_directories(../utils)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../core" libLwm2m_${PROJECT_NAME})
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../platforms/${PLATFORM}" libplatform)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} libLwm2m_client_with_bootstrap libplatform_with_dtls)

