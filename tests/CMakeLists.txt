cmake_minimum_required (VERSION 2.8.12)

project (lwm2munittests)

include(${CMAKE_CURRENT_LIST_DIR}/../core/wakaama.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../platforms/Linux/platform.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../examples/utils/utils.cmake)

add_definitions(-DLWM2M_CLIENT_MODE -DLWM2M_SUPPORT_JSON)
add_definitions(${PLATFORM_DEFINITIONS} ${WAKAAMA_DEFINITIONS})
# Enable all warnings for this test build  
add_definitions(-Wall -Wextra -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Waggregate-return -Wswitch-default -Wswitch-enum)  

include_directories (${WAKAAMA_SOURCES_DIR} ${PLATFORM_INCLUDE_DIRS} ${UTILS_SOURCES_DIR})

## Prepare gtest
find_package(GTest REQUIRED)
include_directories( ${GTEST_INCLUDE_DIRS})

## Prepare for coverage measuring
if(${COVERAGE})
        set(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
endif()

file(GLOB SOURCES "*.c" "*.h" "*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES} ${WAKAAMA_SOURCES} ${PLATFORM_SOURCES} ${UTILS_SOURCES} ${GTEST_FILES})
target_link_libraries(${PROJECT_NAME} ${GTEST_BOTH_LIBRARIES})

# Enable CMake Test Framework (CTest) which make testing available by
# a "test" target. For "make" this is "make test"
enable_testing()

add_test (test_all ${PROJECT_NAME})
