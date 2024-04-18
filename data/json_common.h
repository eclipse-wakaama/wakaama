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
#ifndef WAKAAMA_JSON_COMMON_H
#define WAKAAMA_JSON_COMMON_H

#if defined(LWM2M_SUPPORT_JSON) || defined(LWM2M_SUPPORT_SENML_JSON)
size_t json_skipSpace(const uint8_t *buffer, size_t bufferLen);
int json_split(const uint8_t *buffer, size_t bufferLen, size_t *tokenStartP, size_t *tokenLenP, size_t *valueStartP,
               size_t *valueLenP);
int json_itemLength(const uint8_t *buffer, size_t bufferLen);
int json_countItems(const uint8_t *buffer, size_t bufferLen);
int json_convertNumeric(const uint8_t *value, size_t valueLen, lwm2m_data_t *targetP);
int json_convertTime(const uint8_t *valueStart, size_t valueLen, time_t *t);
size_t json_unescapeString(uint8_t *dst, const uint8_t *src, size_t len);
size_t json_escapeString(uint8_t *dst, size_t dstLen, const uint8_t *src, size_t srcLen);
lwm2m_data_t *json_extendData(lwm2m_data_t *parentP);
int json_dataStrip(int size, lwm2m_data_t *dataP, lwm2m_data_t **resultP);
lwm2m_data_t *json_findDataItem(lwm2m_data_t *listP, size_t count, uint16_t id);
uri_depth_t json_decreaseLevel(uri_depth_t level);
int json_findAndCheckData(const lwm2m_uri_t *uriP, uri_depth_t baseLevel, size_t size, const lwm2m_data_t *tlvP,
                          lwm2m_data_t **targetP, uri_depth_t *targetLevelP);
#endif

#endif /* WAKAAMA_JSON_COMMON_H */
