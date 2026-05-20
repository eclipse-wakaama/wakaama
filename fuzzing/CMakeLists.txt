cmake_minimum_required(VERSION 3.13)

project(fuzzing_tests C)

include(Fuzzing.cmake)

add_subdirectory(coap_parse_message)
add_subdirectory(registration_handleRequest)
