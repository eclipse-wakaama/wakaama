/*******************************************************************************
 *
 * Copyright (c) 2013, 2014, 2015 Intel Corporation and others.
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
 *    David Navarro, Intel Corporation - initial API and implementation
 *******************************************************************************/

#include <liblwm2m.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#ifndef LWM2M_MEMORY_TRACE

void * lwm2m_malloc(size_t s)
{
    return malloc(s);
}

void lwm2m_free(void * p)
{
    free(p);
}

char * lwm2m_strdup(const char * str)
{
    if (!str) {
      return NULL;
    }

    const int len = strlen(str) + 1;
    char * const buf = lwm2m_malloc(len);

    if (buf) {
      memset(buf, 0, len);
      memcpy(buf, str, len - 1);
    }

    return buf;
}

#endif

int lwm2m_strncmp(const char * s1,
                     const char * s2,
                     size_t n)
{
    return strncmp(s1, s2, n);
}

int lwm2m_strcasecmp(const char * str1, const char * str2) {
    return strcasecmp(str1, str2);
}

time_t lwm2m_gettime(void)
{
    return time(NULL);
}

void lwm2m_printf(const char * format, ...)
{
    va_list ap;

    va_start(ap, format);

    vfprintf(stderr, format, ap);

    va_end(ap);
}
