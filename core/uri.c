#include "core/liblwm2m.h"
#include <stdlib.h>
#include <string.h>


static int prv_get_number(const char * uriString,
                          size_t uriLength,
                          int * headP)
{
    int result = 0;
    int mul = 0;

    while (*headP < uriLength && uriString[*headP] != '/')
    {
        if ('0' <= uriString[*headP] && uriString[*headP] <= '9')
        {
            if (0 == mul)
            {
                mul = 10;
            }
            else
            {
                result *= mul;
                mul *= 10;
            }
            result += uriString[*headP] - '0';
        }
        else
        {
            return -1;
        }
        *headP += 1;
    }

    if (uriString[*headP] == '/')
        *headP += 1;

    return result;
}


lwm2m_uri_t * lwm2m_decode_uri(const char * uriString,
                               size_t uriLength)
{
    lwm2m_uri_t * uriP;
    int head = 0;
    int readNum;

    if (NULL == uriString || 0 == uriLength) return NULL;

    uriP = (lwm2m_uri_t *)malloc(sizeof(lwm2m_uri_t));
    if (NULL == uriP) return NULL;

    memset(uriP, 0xFF, sizeof(lwm2m_uri_t));

    // Read object ID
    readNum = prv_get_number(uriString, uriLength, &head);
    if (readNum < 0 || readNum >= LWM2M_URI_NOT_DEFINED) goto error;
    uriP->objID = (uint16_t)readNum;
    if (head >= uriLength) return uriP;

    // Read object instance
    if (uriString[head] == '/')
    {
        // no instance
        head++;
    }
    else
    {
        readNum = prv_get_number(uriString, uriLength, &head);
        if (readNum < 0 || readNum >= LWM2M_URI_NOT_DEFINED) goto error;
        uriP->objInstance = (uint16_t)readNum;
    }
    if (head >= uriLength) return uriP;

    // Read ressource ID
    if (uriString[head] == '/')
    {
        // no ID
        head++;
    }
    else
    {
        readNum = prv_get_number(uriString, uriLength, &head);
        if (readNum < 0 || readNum >= LWM2M_URI_NOT_DEFINED) goto error;
        uriP->resID = (uint16_t)readNum;
    }
    if (head >= uriLength) return uriP;

    // Read ressource instance
    if (uriString[head] == '/')
    {
        // no instance
        head++;
    }
    else
    {
        if (LWM2M_URI_NOT_DEFINED == uriP->resID) goto error;
        readNum = prv_get_number(uriString, uriLength, &head);
        if (readNum < 0 || readNum >= LWM2M_URI_NOT_DEFINED) goto error;
        uriP->resInstance = (uint16_t)readNum;
    }
    if (head < uriLength) goto error;

    return uriP;

error:
    free(uriP);
    return NULL;
}
