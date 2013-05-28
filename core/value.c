#include "core/liblwm2m.h"
#include <stdlib.h>
#include <string.h>


lwm2m_value_t * lwm2m_opaqueToValue(uint8_t * buffer, size_t length)
{
    lwm2m_value_t * valueP;

    if (length > 8)
    {
        valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t) + length - 8);
    }
    else
    {
        valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    }

    if (NULL != valueP)
    {
        valueP->type = LWM2M_OPAQUE_TYPE;
        valueP->length = length;
        memcpy(valueP->buffer, buffer, length);
    }

    return valueP;
}


lwm2m_value_t * lwm2m_stringToValue(char * stringP, size_t length)
{
    lwm2m_value_t * valueP;

    valueP = lwm2m_opaqueToValue((uint8_t *)stringP, length);

    if (NULL != valueP)
    {
        valueP->type = LWM2M_STRING_TYPE;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_int8ToValue(int8_t data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_INTEGER_TYPE;
        valueP->length = 1;
        *((int8_t *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_int16ToValue(int16_t data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_INTEGER_TYPE;
        valueP->length = 2;
        *((int16_t *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_int32ToValue(int32_t data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_INTEGER_TYPE;
        valueP->length = 4;
        *((int32_t *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_int64ToValue(int64_t data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_INTEGER_TYPE;
        valueP->length = 8;
        *((int64_t *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_float32ToValue(float data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_FLOAT_TYPE;
        valueP->length = 4;
        *((float *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_float64ToValue(double data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_FLOAT_TYPE;
        valueP->length = 8;
        *((double *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_boolToValue(bool data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_BOOLEAN_TYPE;
        valueP->length = sizeof(bool);
        *((bool *)valueP->buffer) = data;
    }

    return valueP;
}


lwm2m_value_t * lwm2m_timeToValue(int64_t data)
{
    lwm2m_value_t * valueP;

    valueP = (lwm2m_value_t *)malloc(sizeof(lwm2m_value_t));
    if (NULL != valueP)
    {
        valueP->type = LWM2M_TIME_TYPE;
        valueP->length = 8;
        *((int64_t *)valueP->buffer) = data;
    }

    return valueP;
}
