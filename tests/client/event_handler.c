/*******************************************************************************
 *
 * Copyright (c) 2014 Bosch Software Innovations GmbH, Germany.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Bosch Software Innovations GmbH - Please refer to git log
 *
 *******************************************************************************/
/*
 *  event_handler.c
 *
 *  Callback for value changes.
 *
 *  Created on: 21.01.2015
 *  Author: Achim Kraus
 *  Copyright (c) 2014 Bosch Software Innovations GmbH, Germany. All rights reserved.
 */
#include "liblwm2m.h"

#include <stdio.h>
#include <string.h>

void handle_valueChanged(lwm2m_context_t* lwm2mH, lwm2m_uri_t* uri, const char * value, size_t valueLength) {
    lwm2m_object_t *object = lwm2m_find_object(lwm2mH, uri->objectId);
    if (NULL != object) {
        if (object->writeFunc != NULL) {
            lwm2m_tlv_t * tlvP;

            tlvP = lwm2m_tlv_new(1);
            if (tlvP == NULL) {
                fprintf(stderr, "Internal allocation failure !\n");
                return;
            }
            tlvP->flags = LWM2M_TLV_FLAG_STATIC_DATA | LWM2M_TLV_FLAG_TEXT_FORMAT | LWM2M_TLV_FLAG_INTERNAL_WRITE;
            tlvP->id = uri->resourceId;
            tlvP->length = valueLength;
            tlvP->value = (uint8_t*) value;

            if (COAP_204_CHANGED != object->writeFunc(uri->instanceId, 1, tlvP, object)) {
                fprintf(stderr, "Failed to change value!\n");
            } else {
                fprintf(stderr, "value changed!\n");
                lwm2m_resource_value_changed(lwm2mH, uri);
            }
            lwm2m_tlv_free(1, tlvP);
            return;
        } else {
            fprintf(stderr, "write not supported for specified resource!\n");
        }
        return;
    } else {
        fprintf(stderr, "Object not found !\n");
    }
}

#ifdef LWM2M_EMBEDDED_MODE

static void prv_value_change(void* context, const char* uriPath, const char * value, size_t valueLength) {
    lwm2m_uri_t uri;
    if (lwm2m_stringToUri(uriPath, strlen(uriPath), &uri)) {
        handle_valueChanged(context, &uri, value, valueLength);
    }
}

void init_value_change(lwm2m_context_t * lwm2m) {
    system_setValueChangedHandler(lwm2m, prv_value_change);
}

#else
void init_value_change(lwm2m_context_t * lwm2m)
{
}
#endif
