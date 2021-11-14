#ifndef OBJECT_UTILS_H_
#define OBJECT_UTILS_H_

#include "liblwm2m.h"

#ifdef LWM2M_CLIENT_MODE

// generic helper functions
int object_get_int(lwm2m_context_t *clientCtx, uint16_t objId, uint16_t instanceId, uint16_t resourceId, int *value);
int object_get_str(lwm2m_context_t *clientCtx, uint16_t objId, uint16_t instanceId, uint16_t resourceId, char **value);
int object_get_opaque(lwm2m_context_t *clientCtx, uint16_t objId, uint16_t instanceId, uint16_t resourceId,
                      uint8_t **value, size_t *len);

int security_get_psk_identity(lwm2m_context_t *clientCtx, uint16_t securityInstanceId, uint8_t **pskId, size_t *len);
int security_get_psk(lwm2m_context_t *clientCtx, uint16_t securityInstanceId, uint8_t **psk, size_t *len);
int security_get_security_mode(lwm2m_context_t *clientCtx, uint16_t securityInstanceId, int *mode);

#endif

#endif
