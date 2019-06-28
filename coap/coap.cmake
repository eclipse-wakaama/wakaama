# Provides COAP_SOURCES_DIR and COAP_HEADERS_DIR variables.
# Add LWM2M_WITH_LOGS to compile definitions to enable logging.

set(COAP_SOURCES_DIR ${CMAKE_CURRENT_LIST_DIR})
set(COAP_HEADERS_DIR ${CMAKE_CURRENT_LIST_DIR})
    
set(COAP_SOURCES 
    ${COAP_SOURCES_DIR}/transaction.c
    ${COAP_SOURCES_DIR}/block1.c
    ${COAP_SOURCES_DIR}/er-coap-13/er-coap-13.c)


# This will not work for multi project cmake generators like the Visual Studio Generator
if(CMAKE_BUILD_TYPE MATCHES Debug)
   set(WAKAAMA_DEFINITIONS ${WAKAAMA_DEFINITIONS} -DLWM2M_WITH_LOGS)
endif()
