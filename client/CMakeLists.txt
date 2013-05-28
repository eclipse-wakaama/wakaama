cmake_minimum_required (VERSION 2.6)

project (lwm2mclient)

include_directories ("${PROJECT_SOURCE_DIR}/..")

SET(SOURCES lwm2mclient.c)
SET(CORE_SOURCES ../core/uri.c ../core/value.c)
SET(EXT_SOURCES ${PROJECT_SOURCE_DIR}/../externals/er-coap-13/er-coap-13.c ${PROJECT_SOURCE_DIR}/../externals/er-coap-13/er-coap-13-transactions.c)

add_executable(lwm2mclient ${SOURCES} ${CORE_SOURCES} ${EXT_SOURCES})
