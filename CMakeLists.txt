cmake_minimum_required(VERSION 3.21)

project(wakaama C)

include(wakaama.cmake)

# Enable "test" target
enable_testing()
add_subdirectory(tests)
