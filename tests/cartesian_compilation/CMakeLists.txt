cmake_minimum_required(VERSION 3.13)

# Test multiple combinatorial variants of Wakaamas defines

project(cartesian_compilation C)

# Very much not elegant, but could not find a better solution for producing a Cartesian product.
# Not definig LWM2M_COAP_DEFAULT_BLOCK_SIZE because it gets set unconditionally by wakaama.cmake
set(target_number 0)
foreach(LOG IN ITEMS "LWM2M_WITH_LOGS" "")
    foreach(MODE IN ITEMS
            "LWM2M_CLIENT_MODE"
            "LWM2M_SERVER_MODE"
            "LWM2M_BOOTSTRAP_SERVER_MODE"
            "LWM2M_CLIENT_MODE;LWM2M_SERVER_MODE;LWM2M_BOOTSTRAP_SERVER_MODE")
        foreach(VERSION IN ITEMS "LWM2M_VERSION_1_0" "")
            foreach(BOOTSTRAP IN ITEMS "LWM2M_BOOTSTRAP" "")
                # LWM2M_BOOTSTRAP can be used only in client mode.
                if (BOOTSTRAP STREQUAL "LWM2M_BOOTSTRAP" AND NOT MODE STREQUAL "LWM2M_CLIENT_MODE")
                    continue()
                endif()
                foreach(FORMAT IN ITEMS
                        "LWM2M_SUPPORT_SENML_JSON"
                        "LWM2M_SUPPORT_JSON" "LWM2M_SUPPORT_TLV"
                        "LWM2M_SUPPORT_SENML_JSON;LWM2M_SUPPORT_JSON;LWM2M_SUPPORT_TLV")
                    # LWM2M_SUPPORT_SENML_JSON is valid only for LWM2M version 1.1
                    if (FORMAT MATCHES "LWM2M_SUPPORT_SENML_JSON" AND VERSION STREQUAL "LWM2M_VERSION_1_0" )
                        continue()
                    endif()
                    add_executable(cartesian_compilation_${target_number}
                                   ${COAP_SOURCES}
                                   ${DATA_SOURCES}
                                   ${SHARED_SOURCES}
                                   ${WAKAAMA_SOURCES}
                                   main.c)
                    foreach(definition IN LISTS BLOCK_SIZE BOOTSTRAP FORMAT LOG MODE VERSION)
                        target_compile_definitions(cartesian_compilation_${target_number} PRIVATE ${definition})
                    endforeach()
                    math(EXPR target_number "${target_number} + 1")
                endforeach()
            endforeach()
        endforeach()
    endforeach()
endforeach()
