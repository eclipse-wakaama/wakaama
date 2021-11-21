cmake_minimum_required(VERSION 3.13)

project(wakaama C)

option(DTLS_MBEDTLS "Build wakaama with mbedtls support" OFF)
option(DTLS_TINYDTLS "Build wakaama with tinydtls support" OFF)

if(DTLS_MBEDTLS AND DTLS_TINYDTLS)
    message(FATAL_ERROR "Cant build wakaama with multiple DTLS libraries.")
endif()

set(DTLS DTLS_MBEDTLS OR DTLS_TINYDTLS)

# Enable "test" target
enable_testing()


if(DTLS)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/examples/shared/dtls)
endif()

add_subdirectory(tests)
add_subdirectory(examples)
