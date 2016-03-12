cmake_minimum_required (VERSION 2.6)

project (misctest)

SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../../core)

include_directories (${LIBLWM2M_DIR})

SET(SOURCES test.c)
SET(CORE_SOURCES
    ../../platforms/Linux/platform.c
    ${PROJECT_SOURCE_DIR}/../../core/utils.c)

add_executable(misctest ${SOURCES} ${CORE_SOURCES})
