cmake_minimum_required (VERSION 2.8.12)

SET(SOURCES
    bootstrap_server.c
    bootstrap_info.c
    ../utils/commandline.c
    ../utils/commandline.h
    ../utils/output_utils.c
    ../utils/output_utils.h)

project (bootstrap_server)

if (NOT DEFINED PLATFORM)
    set(PLATFORM "Linux")
endif()

include_directories(../utils)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../core" libLwm2m)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../platforms/${PLATFORM}" libplatform)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} libLwm2m_bootstrap_server libplatform)
