cmake_minimum_required(VERSION 3.13)

project(lwm2mclient C)

include(../../wakaama.cmake)

if(WAKAAMA_ENABLE_EXAMPLES AND WAKAAMA_MODE_CLIENT)

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

    # Client without raw block 1 requests DTLS support
    add_executable(lwm2mclient_raw_block1 ${SOURCES})
    target_compile_definitions(
        lwm2mclient_raw_block1 PRIVATE LWM2M_CLIENT_MODE LWM2M_BOOTSTRAP LWM2M_SUPPORT_SENML_JSON
                                       LWM2M_RAW_BLOCK1_REQUESTS
    )
    target_sources_wakaama(lwm2mclient_raw_block1)
    target_sources_shared(lwm2mclient_raw_block1)

    # Client with DTLS support provided by tinydtls
    add_executable(lwm2mclient_tinydtls ${SOURCES})
    set_target_properties(lwm2mclient_tinydtls PROPERTIES CONNECTION_IMPLEMENTATION "tinydtls")
    target_compile_definitions(lwm2mclient_tinydtls PRIVATE LWM2M_CLIENT_MODE LWM2M_BOOTSTRAP LWM2M_SUPPORT_SENML_JSON)
    target_sources_wakaama(lwm2mclient_tinydtls)
    target_sources_shared(lwm2mclient_tinydtls)
endif()
