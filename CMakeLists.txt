cmake_minimum_required(VERSION 3.13)

project(wakaama C)

include(wakaama.cmake)

add_subdirectory(examples)

# Enable "test" target
enable_testing()
add_subdirectory(tests)
