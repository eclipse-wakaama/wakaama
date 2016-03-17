cmake_minimum_required (VERSION 2.8.12)

SET(SOURCES
    tests.h
    memtest.h
    unittests.c
    tlvtests.c
    uritests.c
    convert_numbers_test.c
    tlv_json_lwm2m_data_test.c
    ../examples/utils/output_utils.c)

project (lwm2munittests)

# Enable all warnings for this test build
add_definitions(-Wall -Wextra -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Waggregate-return -Wswitch-default -Wswitch-enum)

# Enable CMake Test Framework (CTest) which make testing available by
# a "test" target. For "make" this is "make test".
enable_testing ()

include_directories(../examples/utils)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../core" libLwm2m)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../platforms/Linux" libplatform)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} libLwm2m_client_with_json libplatform cunit)
#set_target_properties (test_lib PROPERTIES LINK_FLAGS -Wl,-lcunit)

add_test (test_all ${PROJECT_NAME})
