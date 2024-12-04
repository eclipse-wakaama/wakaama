cmake_minimum_required(VERSION 3.21)

project(lightclient C)

set(WAKAAMA_MODE_CLIENT ON)
set(WAKAAMA_MODE_SERVER OFF)
set(WAKAAMA_MODE_BOOTSTRAP_SERVER OFF)

set(WAKAAMA_CLI ON)
set(WAKAAMA_TRANSPORT POSIX_UDP)
set(WAKAAMA_PLATFORM POSIX)
add_subdirectory(../../ wakaama_lib)

add_executable(lightclient)
target_sources(lightclient PRIVATE lightclient.c object_device.c object_security.c object_server.c object_test.c)
target_link_libraries(lightclient PRIVATE wakaama_static)
