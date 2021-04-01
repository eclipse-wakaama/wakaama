# Provides WAKAAMA_SOURCES_DIR and WAKAAMA_SOURCES variables.
# Add LWM2M_WITH_LOGS to compile definitions to enable logging.
# Set LWM2M_LITTLE_ENDIAN to FALSE or TRUE according to your destination platform or leave
# it unset to determine endianess automatically.
# Set LWM2M_VERSION to use a particular LWM2M version or leave it unset to use the latest.

set(WAKAAMA_SOURCES_DIR ${CMAKE_CURRENT_LIST_DIR})
set(WAKAAMA_HEADERS_DIR ${CMAKE_CURRENT_LIST_DIR}/../include)

set(WAKAAMA_SOURCES
    ${WAKAAMA_SOURCES_DIR}/liblwm2m.c
    ${WAKAAMA_SOURCES_DIR}/uri.c
    ${WAKAAMA_SOURCES_DIR}/utils.c
    ${WAKAAMA_SOURCES_DIR}/objects.c
    ${WAKAAMA_SOURCES_DIR}/list.c
    ${WAKAAMA_SOURCES_DIR}/packet.c
    ${WAKAAMA_SOURCES_DIR}/registration.c
    ${WAKAAMA_SOURCES_DIR}/bootstrap.c
    ${WAKAAMA_SOURCES_DIR}/management.c
    ${WAKAAMA_SOURCES_DIR}/observe.c
    ${WAKAAMA_SOURCES_DIR}/discover.c
    ${WAKAAMA_SOURCES_DIR}/internals.h
)

# This will not work for multi project cmake generators like the Visual Studio Generator
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(LWM2M_WITH_LOGS)
endif()

# Automatically determine endianess. This can be overwritten by setting LWM2M_LITTLE_ENDIAN
# accordingly in a cross compile toolchain file.
if(NOT DEFINED LWM2M_LITTLE_ENDIAN)
    include(TestBigEndian)
    TEST_BIG_ENDIAN(LWM2M_BIG_ENDIAN)
    if (LWM2M_BIG_ENDIAN)
        set(LWM2M_LITTLE_ENDIAN FALSE)
    else()
        set(LWM2M_LITTLE_ENDIAN TRUE)
    endif()
endif()

if (LWM2M_LITTLE_ENDIAN)
    add_compile_definitions(LWM2M_LITTLE_ENDIAN)
endif()

# Set the LWM2M version
set(LWM2M_VERSION "1.1" CACHE STRING "LWM2M version for client and max LWM2M version for server.")
if(LWM2M_VERSION VERSION_EQUAL "1.0")
    add_compile_definitions(LWM2M_VERSION_1_0)
endif()
