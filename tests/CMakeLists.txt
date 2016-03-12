cmake_minimum_required (VERSION 2.8.3)

project (lwm2munittests)
  
SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

add_definitions(-DLWM2M_CLIENT_MODE -DMEMORY_TRACE -DLWM2M_LITTLE_ENDIAN)

include_directories (${LIBLWM2M_DIR})

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)

SET(SOURCES
    unittests.c
    tlvtests.c
    uritests.c
    ../../platforms/Linux/platform.c)

add_executable(lwm2munittests ${SOURCES} ${CORE_SOURCES})

target_link_libraries(lwm2munittests cunit)
