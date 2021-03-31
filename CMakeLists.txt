cmake_minimum_required (VERSION 3.13)

project(wakaama C)

# Enable "test" target
enable_testing()
add_subdirectory(tests)

add_subdirectory(examples)
