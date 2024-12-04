cmake_minimum_required(VERSION 3.21)

project(lwm2mserver C)

set(WAKAAMA_MODE_SERVER ON)
set(WAKAAMA_MODE_CLIENT OFF)
set(WAKAAMA_MODE_BOOTSTRAP_SERVER OFF)
set(WAKAAMA_CLI ON)
set(WAKAAMA_TRANSPORT POSIX_UDP)
set(WAKAAMA_PLATFORM POSIX)
add_subdirectory(../../ wakaama_lib)

add_executable(lwm2mserver)
target_sources(lwm2mserver PRIVATE lwm2mserver.c)
target_link_libraries(lwm2mserver PRIVATE wakaama_static)
