# Run the unit tests with different configurations (defines)
# ~~~
# TARGET_NAME: The name of the test executable target
# SOURCE_FILES: Source files used for the test
# COMPILE_DEFINITIONS: Defines set for this test variant
function(add_test_variant)
    set(oneValueArgs TARGET_NAME)
    set(multiValueArgs SOURCE_FILES COMPILE_DEFINITIONS)
    cmake_parse_arguments(ADD_TEST_VARIANT_ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${ADD_TEST_VARIANT_ARG_TARGET_NAME})
    target_sources(${ADD_TEST_VARIANT_ARG_TARGET_NAME} PRIVATE ${ADD_TEST_VARIANT_ARG_SOURCE_FILES})
    set_target_properties(${ADD_TEST_VARIANT_ARG_TARGET_NAME} PROPERTIES CONNECTION_IMPLEMENTATION "testing")

    # link CUnit
    include(FindPkgConfig)
    pkg_search_module(CUNIT REQUIRED IMPORTED_TARGET cunit)
    target_link_libraries(${ADD_TEST_VARIANT_ARG_TARGET_NAME} PkgConfig::CUNIT)

    # Currently we require that TLV and JSON is available in all tests
    target_compile_definitions(${ADD_TEST_VARIANT_ARG_TARGET_NAME} PRIVATE LWM2M_SUPPORT_TLV LWM2M_SUPPORT_JSON)
    target_compile_definitions(${ADD_TEST_VARIANT_ARG_TARGET_NAME} PRIVATE ${ADD_TEST_VARIANT_ARG_COMPILE_DEFINITIONS})

    # Our tests are designed for POSIX systems
    target_compile_definitions(${ADD_TEST_VARIANT_ARG_TARGET_NAME} PRIVATE _POSIX_C_SOURCE=200809)

    target_sources_wakaama(${ADD_TEST_VARIANT_ARG_TARGET_NAME})
    target_sources_shared(${ADD_TEST_VARIANT_ARG_TARGET_NAME})

    if(SANITIZER)
        target_compile_options(
            ${ADD_TEST_VARIANT_ARG_TARGET_NAME} PRIVATE -fsanitize=${SANITIZER} -fno-sanitize-recover=all
        )
        target_link_options(
            ${ADD_TEST_VARIANT_ARG_TARGET_NAME} PRIVATE -fsanitize=${SANITIZER} -fno-sanitize-recover=all
        )
    endif()

    if(COVERAGE)
        target_compile_options(${ADD_TEST_VARIANT_ARG_TARGET_NAME} PRIVATE --coverage)
        target_link_options(${ADD_TEST_VARIANT_ARG_TARGET_NAME} PRIVATE --coverage)
    endif()

    # Add our unit tests to the "test" target
    add_test(NAME ${ADD_TEST_VARIANT_ARG_TARGET_NAME}_test COMMAND ${ADD_TEST_VARIANT_ARG_TARGET_NAME})
endfunction()
