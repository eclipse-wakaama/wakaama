cmake_minimum_required(VERSION 3.5)

project(wakaama C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_COMPILER "gcc")

set(PROJECT_DESCRIPTION "Wakaama is an implementation of the Open Mobile Alliance's LightWeight M2M protocol (LWM2M).")

set(CMAKE_BUILD_TYPE Release)

# set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_LIST_DIR}/lib)

include(GNUInstallDirs)

include(${CMAKE_CURRENT_LIST_DIR}/core/wakaama.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/examples/shared/shared.cmake)

include_directories(
    ${WAKAAMA_SOURCES_DIR}
    ${SHARED_INCLUDE_DIRS}
    )

set(SOURCES
    ${WAKAAMA_SOURCES}
    ${SHARED_SOURCES}
    )

add_definitions(${SHARED_DEFINITIONS} ${WAKAAMA_DEFINITIONS})

add_library(${PROJECT_NAME} STATIC ${SOURCES})

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
