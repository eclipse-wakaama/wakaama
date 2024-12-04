cmake_minimum_required(VERSION 3.21)

project(bootstrap_server C)

set(WAKAAMA_MODE_BOOTSTRAP_SERVER ON)
set(WAKAAMA_MODE_SERVER OFF)
set(WAKAAMA_MODE_CLIENT OFF)
set(WAKAAMA_CLI ON)
set(WAKAAMA_TRANSPORT POSIX_UDP)
set(WAKAAMA_PLATFORM POSIX)
add_subdirectory(../../ wakaama_lib)

add_executable(bootstrap_server)
target_sources(bootstrap_server PRIVATE bootstrap_info.c bootstrap_server.c bootstrap_info.h bootstrap_server.ini)
target_link_libraries(bootstrap_server PRIVATE wakaama_static)

enable_testing()
