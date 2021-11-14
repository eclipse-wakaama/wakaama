#include "object_utils.h"
#include <string.h>

#ifdef LWM2M_CLIENT_MODE

int object_get_int(lwm2m_context_t *clientCtx, uint16_t objId, uint16_t instanceId, uint16_t resourceId, int *value) {
    lwm2m_object_t *obj = (lwm2m_object_t *)LWM2M_LIST_FIND(clientCtx->objectList, objId);
    int64_t i64 = 0;
    int ret = 0;
    if (obj == NULL) {
        return 0;
    }
    int numData = 1;
    lwm2m_data_t *data = lwm2m_data_new(1);
    if (data == NULL) {
        return 0;
    }
    data->id = resourceId;
    uint8_t coapRet = obj->readFunc(clientCtx, instanceId, &numData, &data, obj);
    if (coapRet == COAP_205_CONTENT) {
        if (lwm2m_data_decode_int(data, &i64) == 1) {
            *value = (int)i64;
            ret = 1;
        }
    }
    lwm2m_data_free(1, data);
    return ret;
}

int object_get_str(lwm2m_context_t *clientCtx, uint16_t objId, uint16_t instanceId, uint16_t resourceId, char **value) {
    lwm2m_object_t *obj = (lwm2m_object_t *)LWM2M_LIST_FIND(clientCtx->objectList, objId);
    int ret = 0;
    if (obj == NULL) {
        return 0;
    }
    int numData = 1;
    lwm2m_data_t *data = lwm2m_data_new(1);
    if (data == NULL) {
        return 0;
    }
    data->id = resourceId;
    uint8_t coapRet = obj->readFunc(clientCtx, instanceId, &numData, &data, obj);
    if (coapRet == COAP_205_CONTENT) {
        if (data->type == LWM2M_TYPE_STRING) {
            *value = lwm2m_malloc(data->value.asBuffer.length + 1);
            if (*value != NULL) {
                memset(*value, 0, data->value.asBuffer.length + 1);
                memcpy(*value, data->value.asBuffer.buffer, data->value.asBuffer.length);
                ret = 1;
            }
        }
    }
    lwm2m_data_free(1, data);
    return ret;
}

int object_get_opaque(lwm2m_context_t *clientCtx, uint16_t objId, uint16_t instanceId, uint16_t resourceId,
                      uint8_t **value, size_t *len) {
    lwm2m_object_t *obj = (lwm2m_object_t *)LWM2M_LIST_FIND(clientCtx->objectList, objId);
    int ret = 0;
    if (obj == NULL) {
        return 0;
    }
    int numData = 1;
    lwm2m_data_t *data = lwm2m_data_new(1);
    if (data == NULL) {
        return 0;
    }
    data->id = resourceId;
    uint8_t coapRet = obj->readFunc(clientCtx, instanceId, &numData, &data, obj);
    if (coapRet == COAP_205_CONTENT) {
        if (data->type == LWM2M_TYPE_OPAQUE || data->type == LWM2M_TYPE_STRING) {
            *value = data->value.asBuffer.buffer;
            *len = data->value.asBuffer.length;
            data->value.asBuffer.buffer = NULL;
            data->value.asBuffer.length = 0;
            ret = 1;
        }
    }
    lwm2m_data_free(1, data);
    return ret;
}

int security_get_psk(lwm2m_context_t *clientCtx, uint16_t securityInstanceId, uint8_t **psk, size_t *len) {
    return object_get_opaque(clientCtx, LWM2M_SECURITY_OBJECT_ID, securityInstanceId, LWM2M_SECURITY_SECRET_KEY_ID, psk,
                             len);
}

int security_get_psk_identity(lwm2m_context_t *clientCtx, uint16_t securityInstanceId, uint8_t **pskId, size_t *len) {
    return object_get_opaque(clientCtx, LWM2M_SECURITY_OBJECT_ID, securityInstanceId, LWM2M_SECURITY_PUBLIC_KEY_ID,
                             pskId, len);
}

int security_get_security_mode(lwm2m_context_t *clientCtx, uint16_t securityInstanceId, int *mode) {
    return object_get_int(clientCtx, LWM2M_SECURITY_OBJECT_ID, securityInstanceId, LWM2M_SECURITY_SECURITY_ID, mode);
}

#endif
