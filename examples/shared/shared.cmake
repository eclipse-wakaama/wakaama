# Provides SHARED_SOURCES_DIR, SHARED_SOURCES, SHARED_INCLUDE_DIRS and SHARED_DEFINITIONS variables

set(SHARED_SOURCES_DIR ${CMAKE_CURRENT_LIST_DIR})

set(SHARED_SOURCES 
    ${SHARED_SOURCES_DIR}/commandline.c
    ${SHARED_SOURCES_DIR}/platform.c
    ${SHARED_SOURCES_DIR}/memtrace.c
)

if(DTLS_TINYDTLS)
    include(${CMAKE_CURRENT_LIST_DIR}/dtls/tinydtls.cmake)

    set(SHARED_SOURCES
        ${SHARED_SOURCES}
        ${TINYDTLS_SOURCES}
        ${SHARED_SOURCES_DIR}/tinydtlsconnection.c
    )

    set(SHARED_INCLUDE_DIRS
        ${SHARED_SOURCES_DIR}
        ${TINYDTLS_SOURCES_DIR}
    )

    add_compile_definitions(WITH_TINYDTLS)

elseif(DTLS_MBEDTLS)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/dtls/mbedtls/include)

    set(SHARED_INCLUDE_DIRS ${SHARED_SOURCES_DIR})
    set(SHARED_SOURCES
        ${SHARED_SOURCES}
        ${SHARED_SOURCES_DIR}/mbedtlsconnection.c
    )
    add_compile_definitions(WITH_MBEDTLS)
else()
    set(SHARED_INCLUDE_DIRS ${SHARED_SOURCES_DIR})
endif()


set(SHARED_SOURCES
    ${SHARED_SOURCES}
    ${SHARED_SOURCES_DIR}/connection.c
    ${SHARED_SOURCES_DIR}/object_utils.c
)
