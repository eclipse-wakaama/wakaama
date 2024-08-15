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
#ifndef WAKAAMA_TLV_H
#define WAKAAMA_TLV_H

#ifdef LWM2M_SUPPORT_TLV
int tlv_parse(const uint8_t *buffer, size_t bufferLen, lwm2m_data_t **dataP);
int tlv_serialize(bool isResourceInstance, int size, lwm2m_data_t *dataP, uint8_t **bufferP);
#endif

#endif /* WAKAAMA_TLV_H */
