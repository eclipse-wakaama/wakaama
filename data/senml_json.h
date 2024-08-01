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
#ifndef WAKAAMA_SENML_JSON_H
#define WAKAAMA_SENML_JSON_H

#ifdef LWM2M_SUPPORT_SENML_JSON
int senml_json_parse(const lwm2m_uri_t *uriP, const uint8_t *buffer, size_t bufferLen, lwm2m_data_t **dataP);
int senml_json_serialize(const lwm2m_uri_t *uriP, int size, const lwm2m_data_t *tlvP, uint8_t **bufferP);
#endif

#endif /* WAKAAMA_SENML_JSON_H */
