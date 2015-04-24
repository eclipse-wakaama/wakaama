cmake_minimum_required (VERSION 2.6)

project (misctest)

include_directories ("${PROJECT_SOURCE_DIR}/../..")

SET(SOURCES test.c)
SET(CORE_SOURCES ${PROJECT_SOURCE_DIR}/../../core/utils.c)

add_executable(misctest ${SOURCES} ${CORE_SOURCES})
