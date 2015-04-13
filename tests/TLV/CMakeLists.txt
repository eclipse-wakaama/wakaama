cmake_minimum_required (VERSION 2.6)

project (tlvtests)

include_directories ("${PROJECT_SOURCE_DIR}/../..")

SET(SOURCES decode.c)
SET(CORE_SOURCES ${PROJECT_SOURCE_DIR}/../../core/tlv.c ${PROJECT_SOURCE_DIR}/../../core/json.c ${PROJECT_SOURCE_DIR}/../../core/uri.c)

add_executable(tlvtests ${SOURCES} ${CORE_SOURCES})
