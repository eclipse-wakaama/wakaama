cmake_minimum_required(VERSION 3.13)

project(lightclient C)

include(../../wakaama.cmake)

add_executable(lightclient lightclient.c object_device.c object_security.c object_server.c object_test.c)
target_compile_definitions(lightclient PRIVATE LWM2M_CLIENT_MODE)
target_sources_wakaama(lightclient)
target_sources_shared(lightclient)
