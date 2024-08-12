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
#ifndef WAKAAMA_DISCOVER_H
#define WAKAAMA_DISCOVER_H

int discover_serialize(lwm2m_context_t *contextP, lwm2m_uri_t *uriP, lwm2m_server_t *serverP, int size,
                       lwm2m_data_t *dataP, uint8_t **bufferP);

#endif /* WAKAAMA_DISCOVER_H */
