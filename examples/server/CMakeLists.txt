cmake_minimum_required(VERSION 3.13)

project(lwm2mserver C)

include(../../wakaama.cmake)

if(WAKAAMA_ENABLE_EXAMPLES AND WAKAAMA_MODE_SERVER)
    add_executable(lwm2mserver lwm2mserver.c)
    target_compile_definitions(lwm2mserver PRIVATE LWM2M_SERVER_MODE)
    target_sources_wakaama(lwm2mserver)
    target_sources_shared(lwm2mserver)
endif()
