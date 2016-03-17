/*******************************************************************************
 *
 * Copyright (c) 2015 Intel Corporation and others.
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

#include "liblwm2m.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>


int main(int argc, char *argv[])
{
    int i;

    char * tests[]={"1", "-114" , "2", "0", "-2", "919293949596979899", "-98979969594939291", "999999999999999999999999999999", "1.2" , "0.134" , "432f.43" , "0.01", "1.00000000000002", NULL};
    int64_t ints[]={12, -114 , 1 , 134 , 43243 , 0, -215025, 0xFF};
    double floats[]={12, -114 , -30 , 1.02 , 134.000235 , 0.43243 , 0, -21.5025, -0.0925, 0.98765, 0xFF};

    printf("lwm2m_PlainTextToInt64():\r\n");
    for (i = 0 ; tests[i] != NULL ; i++)
    {
        int64_t res;

        printf("\"%s\" -> ", tests[i]);
        if (lwm2m_PlainTextToInt64(tests[i], strlen(tests[i]), &res) == 1)
        {
            printf("%" PRId64 "\r\n", res);
        }
        else
        {
            printf ("fail\r\n");
        }
    }

    printf("\r\n\nlwm2m_PlainTextToFloat64():\r\n");
    for (i = 0 ; tests[i] != NULL ; i++)
    {
        double res;

        printf("\"%s\" -> ", tests[i]);
        if (lwm2m_PlainTextToFloat64(tests[i], strlen(tests[i]), &res) == 1)
        {
            int j;
            printf("%.16g\r\n", res);
        }
        else
        {
            printf ("fail\r\n");
        }
    }

    printf("\r\n\nlwm2m_int64ToPlainText():\r\n");
    for (i = 0 ; ints[i] != 0xFF; i++)
    {
        char * res;
        int len;

        printf("%d -> ", ints[i]);
        len = lwm2m_int64ToPlainText(ints[i], &res);

        if (len > 0)
        {
            int j;

            printf("\"");
            for (j = 0 ; j < len ; j++)
                printf("%c", res[j]);
            printf("\" (%d chars)\r\n", len);
            lwm2m_free(res);
        }
        else
        {
            printf ("fail\r\n");
        }
    }

    printf("\r\n\nlwm2m_float64ToPlainText():\r\n");
    for (i = 0 ; floats[i] != 0xFF; i++)
    {
        char * res;
        int len;

        printf("%.16g -> ", floats[i]);
        len = lwm2m_float64ToPlainText(floats[i], &res);

        if (len > 0)
        {
            int j;

            printf("\"");
            for (j = 0 ; j < len ; j++)
                printf("%c", res[j]);
            printf("\" (%d chars)\r\n", len);
            lwm2m_free(res);
        }
        else
        {
            printf ("fail\r\n");
        }
    }
}

