/*******************************************************************************
 *
 * Copyright (c) 2024 GARDENA GmbH
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *   Lukas Woodtli, GARDENA GmbH - Please refer to git log
 *
 *******************************************************************************/
#ifndef WAKAAMA_SENML_COMMON_H
#define WAKAAMA_SENML_COMMON_H

#if defined(LWM2M_SUPPORT_JSON) || defined(LWM2M_SUPPORT_SENML_JSON) || defined(LWM2M_SUPPORT_SENML_CBOR)

typedef struct {
    uint16_t ids[4];
    lwm2m_data_t value; /* Any buffer will be within the parsed data */
    time_t time;
} senml_record_t;

typedef bool (*senml_convertValue)(const senml_record_t *recordP, lwm2m_data_t *targetP);

int senml_convert_records(const lwm2m_uri_t *uriP, senml_record_t *recordArray, int numRecords,
                          senml_convertValue convertValue, lwm2m_data_t **dataP);
lwm2m_data_t *senml_extendData(lwm2m_data_t *parentP, lwm2m_data_type_t type, uint16_t id);
int senml_dataStrip(int size, lwm2m_data_t *dataP, lwm2m_data_t **resultP);
lwm2m_data_t *senml_findDataItem(lwm2m_data_t *listP, size_t count, uint16_t id);
uri_depth_t senml_decreaseLevel(uri_depth_t level);
int senml_findAndCheckData(const lwm2m_uri_t *uriP, uri_depth_t baseLevel, size_t size, const lwm2m_data_t *tlvP,
                           lwm2m_data_t **targetP, uri_depth_t *targetLevelP);
#endif

#endif /* WAKAAMA_SENML_COMMON_H */
