#ifndef _LWM2M_CLIENT_H_
#define _LWM2M_CLIENT_H_

#include <stdint.h>

#define LWM2M_URI_NO_ID       ((uint16_t)0xFFFF)
#define LWM2M_URI_NO_INSTANCE ((uint8_t)0xFF)

typedef struct
{
    uint16_t    objID;
    uint8_t     objInstance;
    uint16_t    resID;
    uint8_t     resInstance;
} lwm2m_uri_t;

#endif
