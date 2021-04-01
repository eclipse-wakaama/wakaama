# Provides DATA_SOURCES_DIR and DATA_HEADERS_DIR variables.
# Add LWM2M_WITH_LOGS to compile definitions to enable logging.

set(DATA_SOURCES_DIR ${CMAKE_CURRENT_LIST_DIR})
set(DATA_HEADERS_DIR ${CMAKE_CURRENT_LIST_DIR})

set(DATA_SOURCES
    ${DATA_SOURCES_DIR}/data.c
    ${DATA_SOURCES_DIR}/tlv.c
    ${DATA_SOURCES_DIR}/json.c
    ${DATA_SOURCES_DIR}/senml_json.c
    ${DATA_SOURCES_DIR}/json_common.c
)
