cmake_minimum_required(VERSION 3.13)

project(bootstrap_server C)

include(../../wakaama.cmake)

add_executable(bootstrap_server bootstrap_info.c bootstrap_server.c bootstrap_info.h bootstrap_server.ini)
target_compile_definitions(bootstrap_server PRIVATE LWM2M_BOOTSTRAP_SERVER_MODE)
target_sources_wakaama(bootstrap_server)
target_sources_shared(bootstrap_server)
