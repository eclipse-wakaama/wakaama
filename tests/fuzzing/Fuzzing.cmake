if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(FUZZING_MAX_TOTAL_TIME
        180
        CACHE STRING "Maximal time (seconds) to run one fuzzing test"
    )

    add_custom_target(run_all_fuzzing_tests COMMENT "Run all fuzzing tests")
    add_custom_target(run_all_fuzzing_reg_tests COMMENT "Run all fuzzing regression tests")

    # ! add_fuzzing_test : Add a fuzzing test that uses `libfuzzer`
    # ~~~
    # TARGET_NAME: The name of the test
    # SOURCE_FILES: A list with all source files needed for the test
    # COMPILE_DEFINITIONS (optional): Compile definitions for the target
    # CORPUS_DIRS (optional): A list with directories containing corpora for initializing the test runs
    # MAX_LEN (optional): Maximum size (bytes) of generated test input
    # CRASH_FILES_DIR (optional): Directory containing crash files for regression tests
    # ~~~
    function(add_fuzzing_test)

        set(oneValueArgs TARGET_NAME MAX_LEN CRASH_FILES_DIR)
        set(multiValueArgs SOURCE_FILES CORPUS_DIRS COMPILE_DEFINITIONS)
        cmake_parse_arguments(FUZZING "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        add_executable(${FUZZING_TARGET_NAME})
        target_sources(${FUZZING_TARGET_NAME} PRIVATE ${FUZZING_SOURCE_FILES})
        target_compile_definitions(${FUZZING_TARGET_NAME} PRIVATE ${FUZZING_COMPILE_DEFINITIONS})

        target_include_directories(
            ${FUZZING_TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/coap ${CMAKE_SOURCE_DIR}/core
                                           ${CMAKE_SOURCE_DIR}/include
        )

        target_compile_options(
            ${FUZZING_TARGET_NAME} PRIVATE -O1 -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all
        )
        target_link_options(
            ${FUZZING_TARGET_NAME} PRIVATE -O1 -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all
        )

        set(max_len 65535)
        if(FUZZING_MAX_LEN)
            set(max_len ${FUZZING_MAX_LEN})
        endif()

        set(run_target run_${FUZZING_TARGET_NAME}_fuzzing)
        add_custom_target(
            ${run_target}
            COMMAND ${FUZZING_TARGET_NAME} -max_total_time=${FUZZING_MAX_TOTAL_TIME} -max_len=${max_len}
                    -artifact_prefix=${CMAKE_CURRENT_LIST_DIR}/ ${FUZZING_CORPUS_DIRS}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            COMMENT "Run a fuzzing test"
        )
        add_dependencies(${run_target} ${FUZZING_TARGET_NAME})
        add_dependencies(run_all_fuzzing_tests ${run_target})

        if(FUZZING_CRASH_FILES_DIR)
            set(reg_test_target run_${FUZZING_TARGET_NAME}_fuzzing_reg_test)
            file(
                GLOB_RECURSE CRASH_FILES
                LIST_DIRECTORIES false
                CONFIGURE_DEPENDS "${FUZZING_CRASH_FILES_DIR}/*"
            )
            add_custom_target(
                ${reg_test_target}
                COMMAND ${FUZZING_TARGET_NAME} -artifact_prefix=${CMAKE_CURRENT_LIST_DIR}/ ${CRASH_FILES}
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                COMMENT "Run fuzzing regression test"
            )
            add_dependencies(${reg_test_target} ${FUZZING_TARGET_NAME})
            add_dependencies(run_all_fuzzing_reg_tests ${reg_test_target})

        endif()
    endfunction()
else()
    # ! add_fuzzing_test : Dummy function that does nothing. This is needed when not compiling with `clang` and
    # therefore `libfuzzer` is not available.
    function(add_fuzzing_test)
        # do nothing if we are not using clang
    endfunction()
endif()
