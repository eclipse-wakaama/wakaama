cmake_minimum_required (VERSION 2.8.12)

# Checkout submodules if necessary
if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/platforms/Linux/tinydtls/dtls.h")
    find_package(Git REQUIRED)
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} )
endif()

include(CMakeDependentOption)

# Add platform library
if (NOT DEFINED PLATFORM)
    set(PLATFORM "Linux")
endif()
add_subdirectory("platforms/${PLATFORM}" libplatform)
add_subdirectory("core")


# Add examples
option(WITH_EXAMPLES "Compile examples" ON)

# Add all sub directories of examples
file(GLOB children RELATIVE ${CMAKE_CURRENT_LIST_DIR}/examples ${CMAKE_CURRENT_LIST_DIR}/examples/*)
foreach(child ${children})
  if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/examples/${child}/CMakeLists.txt" AND NOT "${child}" STREQUAL "..")
     CMAKE_DEPENDENT_OPTION(WITH_EXAMPLE_${child} "Compile ${child} example" ON "WITH_EXAMPLES" OFF)
     if (WITH_EXAMPLE_${child})
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/examples/${child})
    endif()
  endif()
endforeach()

# Add tests
option(WITH_TESTS "Compile Tests" OFF)
if (WITH_TESTS)
    add_subdirectory(tests)
endif()
