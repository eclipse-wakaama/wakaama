cmake_minimum_required(VERSION 3.21)

project(bootstrap_server C)

include(../../wakaama.cmake)

if(WAKAAMA_ENABLE_EXAMPLES AND WAKAAMA_MODE_BOOTSTRAP_SERVER)
    add_executable(bootstrap_server bootstrap_info.c bootstrap_server.c bootstrap_info.h bootstrap_server.ini)
    target_compile_definitions(bootstrap_server PRIVATE LWM2M_BOOTSTRAP_SERVER_MODE)
    target_sources_wakaama(bootstrap_server)
    target_sources_shared(bootstrap_server)
endif()
