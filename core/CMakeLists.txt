# This cmake file provides the "libLwm2m" target.
# Add it to your executable with target_link_libraries(project_name libLwm2m)
cmake_minimum_required (VERSION 2.8.12)

# Avoid multiple inclusions
if (TARGET libLwm2m_server)
    return()
endif()

SET(EXT_SOURCES 
    ${CMAKE_CURRENT_LIST_DIR}/er-coap-13/er-coap-13.c)

SET(CORE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/liblwm2m.c
    ${CMAKE_CURRENT_LIST_DIR}/uri.c
    ${CMAKE_CURRENT_LIST_DIR}/utils.c
    ${CMAKE_CURRENT_LIST_DIR}/objects.c
    ${CMAKE_CURRENT_LIST_DIR}/tlv.c
    ${CMAKE_CURRENT_LIST_DIR}/list.c
    ${CMAKE_CURRENT_LIST_DIR}/packet.c
    ${CMAKE_CURRENT_LIST_DIR}/transaction.c
    ${CMAKE_CURRENT_LIST_DIR}/registration.c
    ${CMAKE_CURRENT_LIST_DIR}/bootstrap.c
    ${CMAKE_CURRENT_LIST_DIR}/management.c
    ${CMAKE_CURRENT_LIST_DIR}/observe.c
    ${CMAKE_CURRENT_LIST_DIR}/json.c
    ${EXT_SOURCES})

project(libLwm2m)

# Enable debug output if compiled as debug
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DLWM2M_WITH_LOGS)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

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
	add_definitions(-DLWM2M_LITTLE_ENDIAN)
endif()

project(libLwm2m_server)
add_library(${PROJECT_NAME} ${CORE_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(${PROJECT_NAME} PUBLIC -DLWM2M_SERVER_MODE)

project(libLwm2m_bootstrap_server)
add_library(${PROJECT_NAME} ${CORE_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(${PROJECT_NAME} PUBLIC -DLWM2M_BOOTSTRAP_SERVER_MODE)

project(libLwm2m_client)
add_library(${PROJECT_NAME} ${CORE_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(${PROJECT_NAME} PUBLIC -DLWM2M_CLIENT_MODE)

project(libLwm2m_client_with_json)
add_library(${PROJECT_NAME} ${CORE_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(${PROJECT_NAME} PUBLIC -DLWM2M_CLIENT_MODE -DLWM2M_SUPPORT_JSON)

project(libLwm2m_client_with_bootstrap)
add_library(${PROJECT_NAME} ${CORE_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(${PROJECT_NAME} PUBLIC -DLWM2M_CLIENT_MODE -DLWM2M_SUPPORT_JSON -DLWM2M_BOOTSTRAP)
