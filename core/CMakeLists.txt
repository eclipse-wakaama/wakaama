SET(EXT_SOURCES ${CMAKE_CURRENT_LIST_DIR}/../externals/er-coap-13/er-coap-13.c)
SET(CORE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/liblwm2m.c
    ${CMAKE_CURRENT_LIST_DIR}/uri.c
    ${CMAKE_CURRENT_LIST_DIR}/utils.c
    ${CMAKE_CURRENT_LIST_DIR}/objects.c
    ${CMAKE_CURRENT_LIST_DIR}/tlv.c
    ${CMAKE_CURRENT_LIST_DIR}/list.c
    ${CMAKE_CURRENT_LIST_DIR}/packet.c
    ${CMAKE_CURRENT_LIST_DIR}/transaction.c
    ${CMAKE_CURRENT_LIST_DIR}/registration.c
    ${EXT_SOURCES}
    PARENT_SCOPE)
    