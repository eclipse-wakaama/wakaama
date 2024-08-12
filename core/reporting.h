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
#ifndef WAKAAMA_REPORTING_H
#define WAKAAMA_REPORTING_H

#if defined(LWM2M_SERVER_MODE) && !defined(LWM2M_VERSION_1_0)
uint8_t reporting_handleSend(lwm2m_context_t *contextP, void *fromSessionH, coap_packet_t *message);
#endif

#endif /* WAKAAMA_REPORTING_H */
