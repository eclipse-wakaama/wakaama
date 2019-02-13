/*******************************************************************************
 *
 * Copyright (c) 2015 Bosch Software Innovations GmbH, Germany.
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
 *    Bosch Software Innovations GmbH - Please refer to git log
 *
 *******************************************************************************/

#include "tests.h"
#include "CUnit/Basic.h"
#include "internals.h"
#include "memtest.h"


static void test_uri_decode(void)
{
    lwm2m_uri_t* uri;
    multi_option_t extraID = { .next = NULL, .is_static = 1, .len = 3, .data = (uint8_t *) "555" };
#ifndef LWM2M_VERSION_1_0
    multi_option_t riID = { .next = NULL, .is_static = 1, .len = 2, .data = (uint8_t *) "12" };
#endif
    multi_option_t rID = { .next = NULL, .is_static = 1, .len = 1, .data = (uint8_t *) "0" };
    multi_option_t iID = { .next = &rID, .is_static = 1, .len = 2, .data = (uint8_t *) "11" };
    multi_option_t oID = { .next = &iID, .is_static = 1, .len = 4, .data = (uint8_t *) "9050" };
    multi_option_t location = { .next = NULL, .is_static = 1, .len = 4, .data = (uint8_t *) "5a3f" };
    multi_option_t locationDecimal = { .next = NULL, .is_static = 1, .len = 4, .data = (uint8_t *) "5312" };
    multi_option_t reg = { .next = NULL, .is_static = 1, .len = 2, .data = (uint8_t *) "rd" };
    multi_option_t boot = { .next = NULL, .is_static = 1, .len = 2, .data = (uint8_t *) "bs" };

    MEMORY_TRACE_BEFORE;

#ifndef LWM2M_VERSION_1_0
    rID.next = &riID;
#endif

    /* "/rd" */
    uri = uri_decode(NULL, &reg);
    CU_ASSERT_PTR_NOT_NULL_FATAL(uri);
    CU_ASSERT_EQUAL(uri->flag, LWM2M_URI_FLAG_REGISTRATION);
    lwm2m_free(uri);

    /* "/rd/5a3f" */
    reg.next = &location;
    uri = uri_decode(NULL, &reg);
    /* should not fail, error in uri_parse */
    /* CU_ASSERT_PTR_NOT_NULL(uri); */
    lwm2m_free(uri);

    /* "/rd/5312" */
    reg.next = &locationDecimal;
    uri = uri_decode(NULL, &reg);
    CU_ASSERT_PTR_NOT_NULL_FATAL(uri);
    CU_ASSERT_EQUAL(uri->flag, LWM2M_URI_FLAG_REGISTRATION | LWM2M_URI_FLAG_OBJECT_ID);
    CU_ASSERT_EQUAL(uri->objectId, 5312);
    lwm2m_free(uri);

    /* "/bs" */
    uri = uri_decode(NULL, &boot);
    CU_ASSERT_PTR_NOT_NULL_FATAL(uri);
    CU_ASSERT_EQUAL(uri->flag, LWM2M_URI_FLAG_BOOTSTRAP);
    lwm2m_free(uri);

    /* "/bs/5a3f" */
    boot.next = &location;
    uri = uri_decode(NULL, &boot);
    CU_ASSERT_PTR_NULL(uri);
    lwm2m_free(uri);

    /* "/9050/11/0" */
    uri = uri_decode(NULL, &oID);
    CU_ASSERT_PTR_NOT_NULL_FATAL(uri);
#ifdef LWM2M_VERSION_1_0
    CU_ASSERT_EQUAL(uri->flag, LWM2M_URI_FLAG_DM | LWM2M_URI_FLAG_OBJECT_ID | LWM2M_URI_FLAG_INSTANCE_ID | LWM2M_URI_FLAG_RESOURCE_ID);
#else
    CU_ASSERT_EQUAL(uri->flag, LWM2M_URI_FLAG_DM | LWM2M_URI_FLAG_OBJECT_ID | LWM2M_URI_FLAG_INSTANCE_ID | LWM2M_URI_FLAG_RESOURCE_ID | LWM2M_URI_FLAG_RESOURCE_INSTANCE_ID);
    CU_ASSERT_EQUAL(uri->resourceInstanceId, 12);
#endif
    CU_ASSERT_EQUAL(uri->objectId, 9050);
    CU_ASSERT_EQUAL(uri->instanceId, 11);
    CU_ASSERT_EQUAL(uri->resourceId, 0);
    lwm2m_free(uri);

    /* "/11/0" */
    uri = uri_decode(NULL, &iID);
    CU_ASSERT_PTR_NOT_NULL_FATAL(uri);
#ifdef LWM2M_VERSION_1_0
    CU_ASSERT_EQUAL(uri->flag, LWM2M_URI_FLAG_DM | LWM2M_URI_FLAG_OBJECT_ID | LWM2M_URI_FLAG_INSTANCE_ID);
#else
    CU_ASSERT_EQUAL(uri->flag, LWM2M_URI_FLAG_DM | LWM2M_URI_FLAG_OBJECT_ID | LWM2M_URI_FLAG_INSTANCE_ID | LWM2M_URI_FLAG_RESOURCE_ID);
    CU_ASSERT_EQUAL(uri->resourceId, 12);
#endif
    CU_ASSERT_EQUAL(uri->objectId, 11);
    CU_ASSERT_EQUAL(uri->instanceId, 0);
    lwm2m_free(uri);

    /* "/0" */
    uri = uri_decode(NULL, &rID);
    CU_ASSERT_PTR_NOT_NULL_FATAL(uri);
#ifdef LWM2M_VERSION_1_0
    CU_ASSERT_EQUAL(uri->flag, LWM2M_URI_FLAG_DM | LWM2M_URI_FLAG_OBJECT_ID);
#else
    CU_ASSERT_EQUAL(uri->flag, LWM2M_URI_FLAG_DM | LWM2M_URI_FLAG_OBJECT_ID | LWM2M_URI_FLAG_INSTANCE_ID);
    CU_ASSERT_EQUAL(uri->instanceId, 12);
#endif
    CU_ASSERT_EQUAL(uri->objectId, 0);
    lwm2m_free(uri);

#ifndef LWM2M_VERSION_1_0
    /* "/12" */
    uri = uri_decode(NULL, &riID);
    CU_ASSERT_PTR_NOT_NULL_FATAL(uri);
    CU_ASSERT_EQUAL(uri->flag, LWM2M_URI_FLAG_DM | LWM2M_URI_FLAG_OBJECT_ID);
    CU_ASSERT_EQUAL(uri->objectId, 12);
    lwm2m_free(uri);
#endif

    /* "/9050/11/0/555" */
#ifdef LWM2M_VERSION_1_0
    rID.next = &extraID;
#else
    riID.next = &extraID;
#endif
    uri = uri_decode(NULL, &oID);
    CU_ASSERT_PTR_NULL(uri);
    lwm2m_free(uri);

    /* "/0/5a3f" */
    rID.next = &location;
    uri = uri_decode(NULL, &rID);
    CU_ASSERT_PTR_NULL(uri);
    lwm2m_free(uri);

    MEMORY_TRACE_AFTER_EQ;
}


static void test_string_to_uri(void)
{
    int result;
    lwm2m_uri_t uri;
    MEMORY_TRACE_BEFORE;
    result = lwm2m_stringToUri("", 0, &uri);
    CU_ASSERT_EQUAL(result, 0);
    result = lwm2m_stringToUri("no_uri", 6, &uri);
    CU_ASSERT_EQUAL(result, 0);
    result = lwm2m_stringToUri("/1", 2, &uri);
    CU_ASSERT_EQUAL(result, 2);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_OBJECT_ID), LWM2M_URI_FLAG_OBJECT_ID);
    CU_ASSERT_EQUAL(uri.objectId, 1);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_INSTANCE_ID), 0);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_RESOURCE_ID), 0);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_RESOURCE_INSTANCE_ID), 0);

    result = lwm2m_stringToUri("/1/2", 4, &uri);
    CU_ASSERT_EQUAL(result, 4);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_OBJECT_ID), LWM2M_URI_FLAG_OBJECT_ID);
    CU_ASSERT_EQUAL(uri.objectId, 1);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_INSTANCE_ID), LWM2M_URI_FLAG_INSTANCE_ID);
    CU_ASSERT_EQUAL(uri.instanceId, 2);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_RESOURCE_ID), 0);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_RESOURCE_INSTANCE_ID), 0);

    result = lwm2m_stringToUri("/1/2/3", 6, &uri);
    CU_ASSERT_EQUAL(result, 6);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_OBJECT_ID), LWM2M_URI_FLAG_OBJECT_ID);
    CU_ASSERT_EQUAL(uri.objectId, 1);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_INSTANCE_ID), LWM2M_URI_FLAG_INSTANCE_ID);
    CU_ASSERT_EQUAL(uri.instanceId, 2);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_RESOURCE_ID), LWM2M_URI_FLAG_RESOURCE_ID);
    CU_ASSERT_EQUAL(uri.resourceId, 3);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_RESOURCE_INSTANCE_ID), 0);

    result = lwm2m_stringToUri("/1/2/3/4", 8, &uri);
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT_EQUAL(result, 8);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_OBJECT_ID), LWM2M_URI_FLAG_OBJECT_ID);
    CU_ASSERT_EQUAL(uri.objectId, 1);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_INSTANCE_ID), LWM2M_URI_FLAG_INSTANCE_ID);
    CU_ASSERT_EQUAL(uri.instanceId, 2);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_RESOURCE_ID), LWM2M_URI_FLAG_RESOURCE_ID);
    CU_ASSERT_EQUAL(uri.resourceId, 3);
    CU_ASSERT_EQUAL((uri.flag & LWM2M_URI_FLAG_RESOURCE_INSTANCE_ID), LWM2M_URI_FLAG_RESOURCE_INSTANCE_ID);
    CU_ASSERT_EQUAL(uri.resourceInstanceId, 4);

    result = lwm2m_stringToUri("/1/2/3/4/5", 10, &uri);
#endif
    CU_ASSERT_EQUAL(result, 0);

    MEMORY_TRACE_AFTER_EQ;
}

static void test_uri_to_string(void)
{
    lwm2m_uri_t uri;
    lwm2m_uri_t uri2;
    uri_depth_t depth;
    int result;
    char buffer[URI_MAX_STRING_LEN];

    MEMORY_TRACE_BEFORE;

    uri.objectId = 1;
    uri.instanceId = 2;
    uri.resourceId = 3;
#ifndef LWM2M_VERSION_1_0
    uri.resourceInstanceId = 4;
#endif

    result = uri_toString(NULL, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(depth, URI_DEPTH_NONE);
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);

    uri.flag = 0;
    result = uri_toString(&uri, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(result, 1);
    CU_ASSERT_EQUAL(depth, URI_DEPTH_NONE);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/", result);
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);
    CU_ASSERT_EQUAL(uri2.flag, uri.flag);

    uri.flag |= LWM2M_URI_FLAG_OBJECT_ID;
    result = uri_toString(&uri, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(result, 2);
    CU_ASSERT_EQUAL(depth, URI_DEPTH_OBJECT);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/1", result);
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);
    CU_ASSERT_EQUAL(uri2.flag, uri.flag);
    CU_ASSERT_EQUAL(uri2.objectId, uri.objectId);

    uri.flag |= LWM2M_URI_FLAG_INSTANCE_ID;
    result = uri_toString(&uri, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(result, 4);
    CU_ASSERT_EQUAL(depth, URI_DEPTH_OBJECT_INSTANCE);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/1/2", result);
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);
    CU_ASSERT_EQUAL(uri2.flag, uri.flag);
    CU_ASSERT_EQUAL(uri2.objectId, uri.objectId);
    CU_ASSERT_EQUAL(uri2.instanceId, uri.instanceId);

    uri.flag |= LWM2M_URI_FLAG_RESOURCE_ID;
    result = uri_toString(&uri, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(result, 6);
    CU_ASSERT_EQUAL(depth, URI_DEPTH_RESOURCE);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/1/2/3", result);
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);
    CU_ASSERT_EQUAL(uri2.flag, uri.flag);
    CU_ASSERT_EQUAL(uri2.objectId, uri.objectId);
    CU_ASSERT_EQUAL(uri2.instanceId, uri.instanceId);
    CU_ASSERT_EQUAL(uri2.resourceId, uri.resourceId);

#ifndef LWM2M_VERSION_1_0
    uri.flag |= LWM2M_URI_FLAG_RESOURCE_INSTANCE_ID;
    result = uri_toString(&uri, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(depth, URI_DEPTH_RESOURCE_INSTANCE);
    CU_ASSERT_EQUAL(result, 8);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/1/2/3/4", result);
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);
    CU_ASSERT_EQUAL(uri2.flag, uri.flag);
    CU_ASSERT_EQUAL(uri2.objectId, uri.objectId);
    CU_ASSERT_EQUAL(uri2.instanceId, uri.instanceId);
    CU_ASSERT_EQUAL(uri2.resourceId, uri.resourceId);
    CU_ASSERT_EQUAL(uri2.resourceInstanceId, uri.resourceInstanceId);
#endif

    memset(&uri, 0xFF, sizeof(uri));
    uri.flag = LWM2M_URI_MASK_ID;
    uri.objectId = LWM2M_MAX_ID - 1;
    uri.instanceId = LWM2M_MAX_ID - 1;
    uri.resourceId = LWM2M_MAX_ID - 1;
#ifndef LWM2M_VERSION_1_0
    uri.resourceInstanceId = LWM2M_MAX_ID - 1;
#endif
    result = uri_toString(&uri, (uint8_t*)buffer, sizeof(buffer), &depth);
    CU_ASSERT_EQUAL(result, URI_MAX_STRING_LEN);
#ifdef LWM2M_VERSION_1_0
    CU_ASSERT_EQUAL(depth, URI_DEPTH_RESOURCE);
    CU_ASSERT_EQUAL(URI_MAX_STRING_LEN, 3*6);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/65534/65534/65534", result);
#else
    CU_ASSERT_EQUAL(depth, URI_DEPTH_RESOURCE_INSTANCE);
    CU_ASSERT_EQUAL(URI_MAX_STRING_LEN, 4*6);
    CU_ASSERT_NSTRING_EQUAL(buffer, "/65534/65534/65534/65534", result);
#endif
    CU_ASSERT_EQUAL(lwm2m_stringToUri(buffer, result, &uri2), result);
    CU_ASSERT_EQUAL(uri2.flag, uri.flag);
    CU_ASSERT_EQUAL(uri2.objectId, uri.objectId);
    CU_ASSERT_EQUAL(uri2.instanceId, uri.instanceId);
    CU_ASSERT_EQUAL(uri2.resourceId, uri.resourceId);
#ifndef LWM2M_VERSION_1_0
    CU_ASSERT_EQUAL(uri2.resourceInstanceId, uri.resourceInstanceId);
#endif

    MEMORY_TRACE_AFTER_EQ;
}

static struct TestTable table[] = {
        { "test of uri_decode()", test_uri_decode },
        { "test of lwm2m_stringToUri()", test_string_to_uri },
        { "test of uri_toString()", test_uri_to_string },
        { NULL, NULL },
};

CU_ErrorCode create_uri_suit()
{
   CU_pSuite pSuite = NULL;

   pSuite = CU_add_suite("Suite_URI", NULL, NULL);
   if (NULL == pSuite) {
      return CU_get_error();
   }

   return add_tests(pSuite, table);
}

