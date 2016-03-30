/*******************************************************************************
 *
 * Copyright (c) 2015 Bosch Software Innovations GmbH, Germany.
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

#include <gtest/gtest.h>

#include "internals.h"
#include "liblwm2m.h"
#include "memtest.h"


TEST(urltests,test_uri_decode)
{
    lwm2m_uri_t* uri;
    multi_option_t extraID = { .next = NULL, .is_static = 1, .len = 3, .data = (uint8_t *) "555" };
    multi_option_t rID = { .next = NULL, .is_static = 1, .len = 1, .data = (uint8_t *) "0" };
    multi_option_t iID = { .next = &rID, .is_static = 1, .len = 2, .data = (uint8_t *) "11" };
    multi_option_t oID = { .next = &iID, .is_static = 1, .len = 4, .data = (uint8_t *) "9050" };
    multi_option_t location = { .next = NULL, .is_static = 1, .len = 4, .data = (uint8_t *) "5a3f" };
    multi_option_t locationDecimal = { .next = NULL, .is_static = 1, .len = 4, .data = (uint8_t *) "5312" };
    multi_option_t reg = { .next = NULL, .is_static = 1, .len = 2, .data = (uint8_t *) "rd" };
    multi_option_t boot = { .next = NULL, .is_static = 1, .len = 2, .data = (uint8_t *) "bs" };

    MEMORY_TRACE_BEFORE;

    /* "/rd" */
    uri = uri_decode(NULL, &reg);
    ASSERT_TRUE(uri);
    ASSERT_EQ(uri->flag, LWM2M_URI_FLAG_REGISTRATION);
    lwm2m_free(uri);

    /* "/rd/5a3f" */
    reg.next = &location;
    uri = uri_decode(NULL, &reg);
    /* should not fail, error in uri_parse */
    /* EXPECT_TRUE(uri); */
    lwm2m_free(uri);

    /* "/rd/5312" */
    reg.next = &locationDecimal;
    uri = uri_decode(NULL, &reg);
    ASSERT_TRUE(uri);
    ASSERT_EQ(uri->flag, LWM2M_URI_FLAG_REGISTRATION | LWM2M_URI_FLAG_OBJECT_ID);
    ASSERT_EQ(uri->objectId, 5312);
    lwm2m_free(uri);

    /* "/bs" */
    uri = uri_decode(NULL, &boot);
    ASSERT_TRUE(uri);
    ASSERT_EQ(uri->flag, LWM2M_URI_FLAG_BOOTSTRAP);
    lwm2m_free(uri);

    /* "/bs/5a3f" */
    boot.next = &location;
    uri = uri_decode(NULL, &boot);
    ASSERT_TRUE(uri);
    lwm2m_free(uri);

    /* "/9050/11/0" */
    uri = uri_decode(NULL, &oID);
    ASSERT_TRUE(uri);
    ASSERT_EQ(uri->flag, LWM2M_URI_FLAG_DM | LWM2M_URI_FLAG_OBJECT_ID | LWM2M_URI_FLAG_INSTANCE_ID | LWM2M_URI_FLAG_RESOURCE_ID);
    ASSERT_EQ(uri->objectId, 9050);
    ASSERT_EQ(uri->instanceId, 11);
    ASSERT_EQ(uri->resourceId, 0);
    lwm2m_free(uri);

    /* "/11/0" */
    uri = uri_decode(NULL, &iID);
    ASSERT_TRUE(uri);
    ASSERT_EQ(uri->flag, LWM2M_URI_FLAG_DM | LWM2M_URI_FLAG_OBJECT_ID | LWM2M_URI_FLAG_INSTANCE_ID);
    ASSERT_EQ(uri->objectId, 11);
    ASSERT_EQ(uri->instanceId, 0);
    lwm2m_free(uri);

    /* "/0" */
    uri = uri_decode(NULL, &rID);
    ASSERT_TRUE(uri);
    ASSERT_EQ(uri->flag, LWM2M_URI_FLAG_DM | LWM2M_URI_FLAG_OBJECT_ID);
    ASSERT_EQ(uri->objectId, 0);
    lwm2m_free(uri);

    /* "/9050/11/0/555" */
    rID.next = &extraID;
    uri = uri_decode(NULL, &oID);
    ASSERT_TRUE(uri);
    lwm2m_free(uri);

    /* "/0/5a3f" */
    rID.next = &location;
    uri = uri_decode(NULL, &rID);
    ASSERT_TRUE(uri);
    lwm2m_free(uri);

    MEMORY_TRACE_AFTER_EQ;
}


TEST(urltests,test_string_to_uri)
{
    int result;
    lwm2m_uri_t uri;
    MEMORY_TRACE_BEFORE;
    result = lwm2m_stringToUri("", 0, &uri);
    ASSERT_EQ(result, 0);
    result = lwm2m_stringToUri("no_uri", 6, &uri);
    ASSERT_EQ(result, 0);
    result = lwm2m_stringToUri("/1", 2, &uri);
    ASSERT_EQ(result, 2);
    ASSERT_EQ((uri.flag & LWM2M_URI_FLAG_OBJECT_ID), LWM2M_URI_FLAG_OBJECT_ID);
    ASSERT_EQ(uri.objectId, 1);
    ASSERT_EQ((uri.flag & LWM2M_URI_FLAG_INSTANCE_ID), 0);
    ASSERT_EQ((uri.flag & LWM2M_URI_FLAG_RESOURCE_ID), 0);

    result = lwm2m_stringToUri("/1/2", 4, &uri);
    ASSERT_EQ(result, 4);
    ASSERT_EQ((uri.flag & LWM2M_URI_FLAG_OBJECT_ID), LWM2M_URI_FLAG_OBJECT_ID);
    ASSERT_EQ(uri.objectId, 1);
    ASSERT_EQ((uri.flag & LWM2M_URI_FLAG_INSTANCE_ID), LWM2M_URI_FLAG_INSTANCE_ID);
    ASSERT_EQ(uri.instanceId, 2);
    ASSERT_EQ((uri.flag & LWM2M_URI_FLAG_RESOURCE_ID), 0);

    result = lwm2m_stringToUri("/1/2/3", 6, &uri);
    ASSERT_EQ(result, 6);
    ASSERT_EQ((uri.flag & LWM2M_URI_FLAG_OBJECT_ID), LWM2M_URI_FLAG_OBJECT_ID);
    ASSERT_EQ(uri.objectId, 1);
    ASSERT_EQ((uri.flag & LWM2M_URI_FLAG_INSTANCE_ID), LWM2M_URI_FLAG_INSTANCE_ID);
    ASSERT_EQ(uri.instanceId, 2);
    ASSERT_EQ((uri.flag & LWM2M_URI_FLAG_RESOURCE_ID), LWM2M_URI_FLAG_RESOURCE_ID);
    ASSERT_EQ(uri.resourceId, 3);

    MEMORY_TRACE_AFTER_EQ;
}

