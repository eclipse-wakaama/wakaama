/*******************************************************************************
 *
 * Copyright (c) 2013 Intel Corporation and others.
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
 *    David Navarro, Intel Corporation - initial API and implementation
 *    
 *******************************************************************************/

#include <stdio.h>
#include "liblwm2m.h"

void print_indent(FILE * stream, int num);
void output_buffer(FILE * stream, const uint8_t * buffer, int length, int indent);
void output_tlv(FILE * stream, const uint8_t * buffer, size_t buffer_len, int indent);
void dump_data_t(FILE * stream, int size, const lwm2m_data_t * dataP, int indent);
void output_data(FILE * stream, lwm2m_media_type_t format, const uint8_t * buffer, int length, int indent);
void print_status(FILE * stream, uint8_t status);
