cmake_minimum_required(VERSION 3.21)

# Our examples are designed for POSIX systems
add_compile_definitions(_POSIX_C_SOURCE=200809)

add_subdirectory(bootstrap_server)
add_subdirectory(client)
add_subdirectory(lightclient)
