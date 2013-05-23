cmake_minimum_required (VERSION 2.6)

project (lwm2mclient)

include_directories ("${PROJECT_SOURCE_DIR}/..")

add_executable(lwm2mclient lwm2mclient.c ${PROJECT_SOURCE_DIR}/../externals/er-coap-13/er-coap-13.c ${PROJECT_SOURCE_DIR}/../externals/er-coap-13/er-coap-13-transactions.c)
