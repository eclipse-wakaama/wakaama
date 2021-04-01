cmake_minimum_required(VERSION 3.13)

add_compile_definitions(_POSIX_C_SOURCE=200809)
add_compile_options(-pedantic)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/bootstrap_server)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/client)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lightclient)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/server)
