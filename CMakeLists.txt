cmake_minimum_required(VERSION 3.20)

project(wakaama C)

add_subdirectory(examples)

# Enable "test" target
enable_testing()
add_subdirectory(tests)
