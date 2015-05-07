/*******************************************************************************
 *
 * Copyright (c) 2015 Bosch Software Innvoations GmbH and others.
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
#ifndef MEMTRACE_H_
#define MEMTRACE_H_

#ifdef MEMORY_TRACE
#include <string.h>
#include <stdlib.h>

char* trace_strdup(const char* str, const char* file, const char* function, int lineno);
void* trace_malloc(size_t size, const char* file, const char* function, int lineno);
void trace_free(void* mem, const char* file, const char* function, int lineno);
void trace_print(int loops, int level);
void trace_status(int* blocks, size_t* size);

#define lwm2m_strdup(S) trace_strdup(S, __FILE__, __FUNCTION__, __LINE__)
#define lwm2m_malloc(S) trace_malloc(S, __FILE__, __FUNCTION__, __LINE__)
#define lwm2m_free(M) trace_free(M, __FILE__, __FUNCTION__, __LINE__)
#define strdup(S) trace_strdup(S, __FILE__, __FUNCTION__, __LINE__)
#define malloc(S) trace_malloc(S, __FILE__, __FUNCTION__, __LINE__)
#define free(M) trace_free(M, __FILE__, __FUNCTION__, __LINE__)

#endif

#endif /* MEMTRACE_H_ */
