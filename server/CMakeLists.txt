cmake_minimum_required (VERSION 2.6)

project (lwm2mserver)

include_directories ("${PROJECT_SOURCE_DIR}/..")

SET(SOURCES lwm2mserver.c)
SET(CORE_SOURCES ../core/liblwm2m.c ../core/uri.c ../core/utils.c ../core/objects.c ../core/tlv.c ../core/list.c ../core/packet.c ../core/transaction.c)
SET(EXT_SOURCES ${PROJECT_SOURCE_DIR}/../externals/er-coap-13/er-coap-13.c)

add_executable(lwm2mserver ${SOURCES} ${CORE_SOURCES} ${EXT_SOURCES})
