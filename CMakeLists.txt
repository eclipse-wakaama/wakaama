cmake_minimum_required(VERSION 3.13)

project(wakaama C)

# Enable "test" target
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
endif()

add_subdirectory(tests)
add_subdirectory(examples)
