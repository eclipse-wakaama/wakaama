cmake_minimum_required(VERSION 3.21)

project(lwm2mclient C)

include(../../wakaama.cmake)

if(WAKAAMA_ENABLE_EXAMPLES
   AND WAKAAMA_MODE_CLIENT
   AND WAKAAMA_CLIENT_INITIATED_BOOTSTRAP
   AND WAKAAMA_DATA_SENML_JSON
)

    set(SOURCES
        lwm2mclient.c
        lwm2mclient.h
        object_access_control.c
        object_connectivity_moni.c
        object_connectivity_stat.c
        object_device.c
        object_firmware.c
        object_location.c
        object_security.c
        object_server.c
        object_test.c
        system_api.c
    )

    # Client without DTLS support
    add_executable(lwm2mclient ${SOURCES})
    target_compile_definitions(lwm2mclient PRIVATE LWM2M_CLIENT_MODE LWM2M_BOOTSTRAP LWM2M_SUPPORT_SENML_JSON)
    target_sources_wakaama(lwm2mclient)
    target_sources_shared(lwm2mclient)

    if(WAKAAMA_COAP_RAW_BLOCK1_REQUESTS)
        # Client with raw block 1 requests and without DTLS support
        add_executable(lwm2mclient_raw_block1 ${SOURCES})
        target_compile_definitions(
            lwm2mclient_raw_block1 PRIVATE LWM2M_CLIENT_MODE LWM2M_BOOTSTRAP LWM2M_SUPPORT_SENML_JSON
                                           LWM2M_RAW_BLOCK1_REQUESTS
        )
        target_sources_wakaama(lwm2mclient_raw_block1)
        target_sources_shared(lwm2mclient_raw_block1)
    endif()

    # Client with DTLS support provided by tinydtls
    add_executable(lwm2mclient_tinydtls ${SOURCES})
    set_target_properties(lwm2mclient_tinydtls PROPERTIES CONNECTION_IMPLEMENTATION "tinydtls")
    target_compile_definitions(lwm2mclient_tinydtls PRIVATE LWM2M_CLIENT_MODE LWM2M_BOOTSTRAP LWM2M_SUPPORT_SENML_JSON)
    target_sources_wakaama(lwm2mclient_tinydtls)
    target_sources_shared(lwm2mclient_tinydtls)
endif()
