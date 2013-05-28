#ifndef _LWM2M_CLIENT_H_
#define _LWM2M_CLIENT_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


/*
 *  Ressource URI
 */

#define LWM2M_URI_NO_ID       ((uint16_t)0xFFFF)
#define LWM2M_URI_NO_INSTANCE ((uint8_t)0xFF)

typedef struct
{
    uint16_t    objID;
    uint8_t     objInstance;
    uint16_t    resID;
    uint8_t     resInstance;
} lwm2m_uri_t;

// defined in uri.c
lwm2m_uri_t * lwm2m_decode_uri(const char * uriString, size_t uriLength);


/*
 *  Ressource values
 */

typedef enum
{
    LWM2M_OPAQUE_TYPE = 0,
    LWM2M_STRING_TYPE,
    LWM2M_INTEGER_TYPE,
    LWM2M_FLOAT_TYPE,
    LWM2M_BOOLEAN_TYPE,
    LWM2M_TIME_TYPE
} lwm2m_data_type_t;

typedef struct
{
    lwm2m_data_type_t   type;
    size_t              length;
    uint8_t             buffer[8];
} lwm2m_value_t;

// defined in value.c
lwm2m_value_t * lwm2m_opaqueToValue(uint8_t * buffer, size_t length);
lwm2m_value_t * lwm2m_stringToValue(char * stringP, size_t length);
lwm2m_value_t * lwm2m_int8ToValue(int8_t data);
lwm2m_value_t * lwm2m_int16ToValue(int16_t data);
lwm2m_value_t * lwm2m_int32ToValue(int32_t data);
lwm2m_value_t * lwm2m_int64ToValue(int64_t data);
lwm2m_value_t * lwm2m_float32ToValue(float data);
lwm2m_value_t * lwm2m_float64ToValue(double data);
lwm2m_value_t * lwm2m_boolToValue(bool data);
lwm2m_value_t * lwm2m_timeToValue(int64_t data);


/*
 * LWM2M Objects
 */
typedef uint8_t (*lwm2m_read_callback_t) (lwm2m_uri_t * uriP, lwm2m_value_t ** valueP, void * userData);
typedef uint8_t (*lwm2m_write_callback_t) (lwm2m_uri_t * uriP, lwm2m_value_t * valueP, void * userData);
typedef uint8_t (*lwm2m_execute_callback_t) (lwm2m_uri_t * uriP, void * userData);

typedef struct
{
    uint16_t                 objID;
    lwm2m_read_callback_t    readFunc;
    lwm2m_write_callback_t   writeFunc;
    lwm2m_execute_callback_t executeFunc;
    void *                   userData;
} lwm2m_object_t;

#endif
