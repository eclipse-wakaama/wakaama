cmake_minimum_required (VERSION 2.8.3)

project (lwm2munittests)
  
SET(LIBLWM2M_DIR ${PROJECT_SOURCE_DIR}/../core)

add_definitions(-DLWM2M_CLIENT_MODE -DLWM2M_SUPPORT_JSON -DLWM2M_LITTLE_ENDIAN)

include_directories (${LIBLWM2M_DIR} ${PROJECT_SOURCE_DIR}/../examples/utils ${PROJECT_SOURCE_DIR}/../core)

add_subdirectory(${LIBLWM2M_DIR} ${CMAKE_CURRENT_BINARY_DIR}/core)

# Enable all warnings for this test build  
add_definitions(-Wall -Wextra -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Waggregate-return -Wswitch-default -Wswitch-enum)  

SET(SOURCES
    unittests.c
    tlvtests.c
    uritests.c
    convert_numbers_test.c
    tlv_json_lwm2m_data_test.c
    ${PROJECT_SOURCE_DIR}/../platforms/Linux/platform.c
    ${PROJECT_SOURCE_DIR}/../platforms/Linux/connection.c
    ${PROJECT_SOURCE_DIR}/../examples/utils/commandline.c)

add_executable(lwm2munittests ${SOURCES} ${CORE_SOURCES})

target_link_libraries(lwm2munittests cunit)
