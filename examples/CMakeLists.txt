cmake_minimum_required(VERSION 3.13)

add_compile_definitions(_POSIX_C_SOURCE=200809)
add_compile_options(-pedantic)

if(DTLS_MBEDTLS)
    option(ENABLE_PROGRAMS "Build mbed TLS programs." OFF)
    option(ENABLE_TESTING "Build mbed TLS tests." OFF)
    # Use custom config file for Mbed TLS
    add_definitions(-DMBEDTLS_CONFIG_FILE="${CMAKE_CURRENT_LIST_DIR}/shared/dtls/config-ccm-psk-tls1_2.h")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/shared/dtls/mbedtls)
else()
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lightclient)
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/bootstrap_server)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/client)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/server)