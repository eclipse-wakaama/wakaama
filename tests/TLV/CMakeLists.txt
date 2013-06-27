cmake_minimum_required (VERSION 2.6)

project (tlvdecode)

include_directories ("${PROJECT_SOURCE_DIR}/../..")

SET(SOURCES decode.c)
SET(CORE_SOURCES ../core/tlv.c)

add_executable(tlvdecode ${SOURCES} ${CORE_SOURCES})
