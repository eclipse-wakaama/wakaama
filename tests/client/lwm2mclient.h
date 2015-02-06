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
 * lwm2mclient.h
 *
 *  General functions of lwm2m test client.
 *
 *  Created on: 22.01.2015
 *  Author: Achim Kraus
 *  Copyright (c) 2015 Bosch Software Innovations GmbH, Germany. All rights reserved.
 */

#ifndef LWM2MCLIENT_H_
#define LWM2MCLIENT_H_

#include "liblwm2m.h"

extern int g_reboot;

/*
 * object_device.c
 */
extern lwm2m_object_t * get_object_device();
uint8_t device_change(lwm2m_tlv_t * dataArray, lwm2m_object_t * objectP);
/*
 * object_firmware.c
 */
extern lwm2m_object_t * get_object_firmware();
/*
 * object_location.c
 */
extern lwm2m_object_t * get_object_location();
/*
 * object_test.c
 */
extern lwm2m_object_t * get_test_object();
/*
 * object_server.c
 */
extern lwm2m_object_t * get_server_object(int serverId, const char* binding, int lifetime, bool storing);
/*
 * object_connectivity_moni.c
 */
extern lwm2m_object_t * get_object_conn_m();
uint8_t connectivity_moni_change(lwm2m_tlv_t * dataArray, lwm2m_object_t * objectP);

/*
 * lwm2mclient.c
 */
extern void handle_value_changed(lwm2m_context_t* lwm2mH, lwm2m_uri_t* uri, const char * value, size_t valueLength);
/*
 * system_api.c
 */
extern void init_value_change(lwm2m_context_t * lwm2m);
extern void system_reboot();

/*
 * object_security.c
 */
extern lwm2m_object_t * get_security_object();
extern char * get_server_uri(lwm2m_object_t * objectP, uint16_t serverID);

#endif /* LWM2MCLIENT_H_ */
