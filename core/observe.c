/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
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
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *    Tuve Nordius, Husqvarna Group - Please refer to git log
 *
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>

*/

#include "internals.h"
#include <stdio.h>


#ifdef LWM2M_CLIENT_MODE
static lwm2m_observed_t * prv_findObserved(lwm2m_context_t * contextP,
                                           lwm2m_uri_t * uriP)
{
    lwm2m_observed_t * targetP;

    targetP = contextP->observedList;
    while (targetP != NULL
        && ((LWM2M_URI_IS_SET_OBJECT(uriP) && targetP->uri.objectId != uriP->objectId)
         || (LWM2M_URI_IS_SET_INSTANCE(uriP) && targetP->uri.instanceId != uriP->instanceId)
         || (LWM2M_URI_IS_SET_RESOURCE(uriP) && targetP->uri.resourceId != uriP->resourceId)
#ifndef LWM2M_VERSION_1_0
         || (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP) && targetP->uri.resourceInstanceId != uriP->resourceInstanceId)
#endif
           ))
    {
        targetP = targetP->next;
    }

    return targetP;
}

static void prv_unlinkObserved(lwm2m_context_t * contextP,
                               lwm2m_observed_t * observedP)
{
    if (contextP->observedList == observedP)
    {
        contextP->observedList = contextP->observedList->next;
    }
    else
    {
        lwm2m_observed_t * parentP;

        parentP = contextP->observedList;
        while (parentP->next != NULL
            && parentP->next != observedP)
        {
            parentP = parentP->next;
        }
        if (parentP->next != NULL)
        {
            parentP->next = parentP->next->next;
        }
    }
}

static lwm2m_watcher_t * prv_findWatcher(lwm2m_observed_t * observedP,
                                         lwm2m_server_t * serverP)
{
    lwm2m_watcher_t * targetP;

    targetP = observedP->watcherList;
    while (targetP != NULL
        && targetP->server != serverP)
    {
        targetP = targetP->next;
    }

    return targetP;
}

static lwm2m_watcher_t * prv_getWatcher(lwm2m_context_t * contextP,
                                        lwm2m_uri_t * uriP,
                                        lwm2m_server_t * serverP)
{
    lwm2m_observed_t * observedP;
    bool allocatedObserver;
    lwm2m_watcher_t * watcherP;

    allocatedObserver = false;

    observedP = prv_findObserved(contextP, uriP);
    if (observedP == NULL)
    {
        observedP = (lwm2m_observed_t *)lwm2m_malloc(sizeof(lwm2m_observed_t));
        if (observedP == NULL) return NULL;
        allocatedObserver = true;
        memset(observedP, 0, sizeof(lwm2m_observed_t));
        memcpy(&(observedP->uri), uriP, sizeof(lwm2m_uri_t));
        observedP->next = contextP->observedList;
        contextP->observedList = observedP;
    }

    watcherP = prv_findWatcher(observedP, serverP);
    if (watcherP == NULL)
    {
        watcherP = (lwm2m_watcher_t *)lwm2m_malloc(sizeof(lwm2m_watcher_t));
        if (watcherP == NULL)
        {
            if (allocatedObserver == true)
            {
                lwm2m_free(observedP);
            }
            return NULL;
        }
        memset(watcherP, 0, sizeof(lwm2m_watcher_t));
        watcherP->active = false;
        watcherP->server = serverP;
        watcherP->next = observedP->watcherList;
        observedP->watcherList = watcherP;
    }

    return watcherP;
}

uint8_t observe_handleRequest(lwm2m_context_t * contextP,
                              lwm2m_uri_t * uriP,
                              lwm2m_server_t * serverP,
                              int size,
                              lwm2m_data_t * dataP,
                              coap_packet_t * message,
                              coap_packet_t * response)
{
    lwm2m_observed_t * observedP;
    lwm2m_watcher_t * watcherP;
    lwm2m_data_t * valueP;
    uint32_t count;

    (void) size; /* unused */

    LOG_ARG_DBG("Code: %02X, server status: %s", message->code, STR_STATUS(serverP->status));
    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));

    coap_get_header_observe(message, &count);

    switch (count)
    {
    case 0:
        if (!LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)) return COAP_400_BAD_REQUEST;
        if (message->token_len == 0) return COAP_400_BAD_REQUEST;

        watcherP = prv_getWatcher(contextP, uriP, serverP);
        if (watcherP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

        watcherP->tokenLen = message->token_len;
        memcpy(watcherP->token, message->token, message->token_len);
        watcherP->active = true;
        watcherP->lastTime = lwm2m_gettime();
        watcherP->lastMid = response->mid;
        watcherP->format = (lwm2m_media_type_t)response->content_type;

        valueP = dataP;
#ifndef LWM2M_VERSION_1_0
        if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)
         && dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE
         && dataP->value.asChildren.count == 1)
        {
            valueP = dataP->value.asChildren.array;
        }
#endif
        if (LWM2M_URI_IS_SET_RESOURCE(uriP))
        {
            switch (valueP->type)
            {
            case LWM2M_TYPE_INTEGER:
                if (1 != lwm2m_data_decode_int(valueP, &(watcherP->lastValue.asInteger))) return COAP_500_INTERNAL_SERVER_ERROR;
                break;
            case LWM2M_TYPE_UNSIGNED_INTEGER:
                if (1 != lwm2m_data_decode_uint(valueP, &(watcherP->lastValue.asUnsigned))) return COAP_500_INTERNAL_SERVER_ERROR;
                break;
            case LWM2M_TYPE_FLOAT:
                if (1 != lwm2m_data_decode_float(valueP, &(watcherP->lastValue.asFloat))) return COAP_500_INTERNAL_SERVER_ERROR;
                break;
            default:
                break;
            }
        }

        coap_set_header_observe(response, watcherP->counter++);

        return COAP_205_CONTENT;

    case 1:
        // cancellation
        observedP = prv_findObserved(contextP, uriP);
        if (observedP)
        {
            watcherP = prv_findWatcher(observedP, serverP);
            if (watcherP)
            {
                observe_cancel(contextP, watcherP->lastMid, serverP->sessionH);
            }
        }
        return COAP_205_CONTENT;

    default:
        return COAP_400_BAD_REQUEST;
    }
}

void observe_cancel(lwm2m_context_t * contextP,
                    uint16_t mid,
                    void * fromSessionH)
{
    lwm2m_observed_t * observedP;

    LOG_ARG_DBG("mid: %d", mid);

    for (observedP = contextP->observedList;
         observedP != NULL;
         observedP = observedP->next)
    {
        lwm2m_watcher_t * targetP = NULL;

        if (observedP->watcherList->lastMid == mid
         && lwm2m_session_is_equal(observedP->watcherList->server->sessionH, fromSessionH, contextP->userData))
        {
            targetP = observedP->watcherList;
            observedP->watcherList = observedP->watcherList->next;
        }
        else
        {
            lwm2m_watcher_t * parentP;

            parentP = observedP->watcherList;
            while (parentP->next != NULL
                && (parentP->next->lastMid != mid
                 || !lwm2m_session_is_equal(parentP->next->server->sessionH, fromSessionH, contextP->userData)))
            {
                parentP = parentP->next;
            }
            if (parentP->next != NULL)
            {
                targetP = parentP->next;
                parentP->next = parentP->next->next;
            }
        }
        if (targetP != NULL)
        {
            if (targetP->parameters != NULL) lwm2m_free(targetP->parameters);
            lwm2m_free(targetP);
            if (observedP->watcherList == NULL)
            {
                prv_unlinkObserved(contextP, observedP);
                lwm2m_free(observedP);
            }
            return;
        }
    }
}

void observe_clear(lwm2m_context_t * contextP,
                   lwm2m_uri_t * uriP)
{
    lwm2m_observed_t * observedP;

    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));

    observedP = contextP->observedList;
    while(observedP != NULL)
    {
        if (observedP->uri.objectId == uriP->objectId
            && (LWM2M_URI_IS_SET_INSTANCE(uriP) == false
                || observedP->uri.instanceId == uriP->instanceId))
        {
            lwm2m_observed_t * nextP;
            lwm2m_watcher_t * watcherP;

            nextP = observedP->next;

            for (watcherP = observedP->watcherList; watcherP != NULL; watcherP = watcherP->next)
            {
                if (watcherP->parameters != NULL) lwm2m_free(watcherP->parameters);
            }
            LWM2M_LIST_FREE(observedP->watcherList);

            prv_unlinkObserved(contextP, observedP);
            lwm2m_free(observedP);

            observedP = nextP;
        }
        else
        {
            observedP = observedP->next;
        }
    }
}

uint8_t observe_setParameters(lwm2m_context_t * contextP,
                              lwm2m_uri_t * uriP,
                              lwm2m_server_t * serverP,
                              lwm2m_attributes_t * attrP)
{
    uint8_t result;
    lwm2m_watcher_t * watcherP;

    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));
    LOG_ARG_DBG("toSet: %08X, toClear: %08X, minPeriod: %d, maxPeriod: %d, greaterThan: %f, lessThan: %f, step: %f",
                attrP->toSet, attrP->toClear, attrP->minPeriod, attrP->maxPeriod, attrP->greaterThan, attrP->lessThan,
                attrP->step);

    if (!LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)) return COAP_400_BAD_REQUEST;

    result = object_checkReadable(contextP, uriP, attrP);
    if (COAP_205_CONTENT != result) return result;

    watcherP = prv_getWatcher(contextP, uriP, serverP);
    if (watcherP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    // Check rule “lt” value + 2*”stp” values < “gt” value
    if ((((attrP->toSet | (watcherP->parameters?watcherP->parameters->toSet:0)) & ~attrP->toClear) & ATTR_FLAG_NUMERIC) == ATTR_FLAG_NUMERIC)
    {
        float gt;
        float lt;
        float stp;

        if (0 != (attrP->toSet & LWM2M_ATTR_FLAG_GREATER_THAN))
        {
            gt = attrP->greaterThan;
        }
        else
        {
            gt = watcherP->parameters->greaterThan;
        }
        if (0 != (attrP->toSet & LWM2M_ATTR_FLAG_LESS_THAN))
        {
            lt = attrP->lessThan;
        }
        else
        {
            lt = watcherP->parameters->lessThan;
        }
        if (0 != (attrP->toSet & LWM2M_ATTR_FLAG_STEP))
        {
            stp = attrP->step;
        }
        else
        {
            stp = watcherP->parameters->step;
        }

        if (lt + (2 * stp) >= gt) return COAP_400_BAD_REQUEST;
    }

    if (watcherP->parameters == NULL)
    {
        if (attrP->toSet != 0)
        {
            watcherP->parameters = (lwm2m_attributes_t *)lwm2m_malloc(sizeof(lwm2m_attributes_t));
            if (watcherP->parameters == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
            memcpy(watcherP->parameters, attrP, sizeof(lwm2m_attributes_t));
        }
        else
        {
            return COAP_204_CHANGED;
        }
    }
    else
    {
        watcherP->parameters->toSet &= ~attrP->toClear;
        if (attrP->toSet & LWM2M_ATTR_FLAG_MIN_PERIOD)
        {
            watcherP->parameters->minPeriod = attrP->minPeriod;
        }
        if (attrP->toSet & LWM2M_ATTR_FLAG_MAX_PERIOD)
        {
            watcherP->parameters->maxPeriod = attrP->maxPeriod;
        }
        if (attrP->toSet & LWM2M_ATTR_FLAG_GREATER_THAN)
        {
            watcherP->parameters->greaterThan = attrP->greaterThan;
        }
        if (attrP->toSet & LWM2M_ATTR_FLAG_LESS_THAN)
        {
            watcherP->parameters->lessThan = attrP->lessThan;
        }
        if (attrP->toSet & LWM2M_ATTR_FLAG_STEP)
        {
            watcherP->parameters->step = attrP->step;
        }
    }

    LOG_ARG_DBG("Final toSet: %08X, minPeriod: %d, maxPeriod: %d, greaterThan: %f, lessThan: %f, step: %f",
                watcherP->parameters->toSet, watcherP->parameters->minPeriod, watcherP->parameters->maxPeriod,
                watcherP->parameters->greaterThan, watcherP->parameters->lessThan, watcherP->parameters->step);

    return COAP_204_CHANGED;
}

lwm2m_observed_t * observe_findByUri(lwm2m_context_t * contextP,
                                     lwm2m_uri_t * uriP)
{
    lwm2m_observed_t * targetP;

    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));
    targetP = contextP->observedList;
    while (targetP != NULL)
    {
        if (targetP->uri.objectId == uriP->objectId)
        {
            if ((!LWM2M_URI_IS_SET_INSTANCE(uriP) && !LWM2M_URI_IS_SET_INSTANCE(&(targetP->uri)))
             || (LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_INSTANCE(&(targetP->uri)) && (uriP->instanceId == targetP->uri.instanceId)))
             {
                 if ((!LWM2M_URI_IS_SET_RESOURCE(uriP) && !LWM2M_URI_IS_SET_RESOURCE(&(targetP->uri)))
                     || (LWM2M_URI_IS_SET_RESOURCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(&(targetP->uri)) && (uriP->resourceId == targetP->uri.resourceId)))
                 {
#ifndef LWM2M_VERSION_1_0
                     if ((!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP) && !LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&(targetP->uri)))
                      || (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&(targetP->uri)) && (uriP->resourceInstanceId == targetP->uri.resourceInstanceId)))
#endif
                     {
                         LOG_ARG_DBG("Found one with%s observers.", targetP->watcherList ? "" : " no");
                         LOG_ARG_DBG("%s", LOG_URI_TO_STRING(&(targetP->uri)));
                         return targetP;
                     }
                 }
             }
        }
        targetP = targetP->next;
    }

    LOG_DBG("Found nothing");
    return NULL;
}

void lwm2m_resource_value_changed(lwm2m_context_t * contextP,
                                  lwm2m_uri_t * uriP)
{
    lwm2m_observed_t * targetP;

    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));
    targetP = contextP->observedList;
    while (targetP != NULL)
    {
        if (targetP->uri.objectId == uriP->objectId)
        {
            if (!LWM2M_URI_IS_SET_INSTANCE(uriP)
             || !LWM2M_URI_IS_SET_INSTANCE(&targetP->uri)
             || uriP->instanceId == targetP->uri.instanceId)
            {
                if (!LWM2M_URI_IS_SET_RESOURCE(uriP)
                 || !LWM2M_URI_IS_SET_RESOURCE(&targetP->uri)
                 || uriP->resourceId == targetP->uri.resourceId)
                {
#ifndef LWM2M_VERSION_1_0
                    if (!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)
                     || !LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&targetP->uri)
                     || uriP->resourceInstanceId == targetP->uri.resourceInstanceId)
#endif
                    {
                        lwm2m_watcher_t * watcherP;

                        LOG_DBG("Found an observation");
                        LOG_ARG_DBG("%s", LOG_URI_TO_STRING(&(targetP->uri)));

                        for (watcherP = targetP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
                        {
                            if (watcherP->active == true)
                            {
                                LOG_DBG("Tagging a watcher");
                                watcherP->update = true;
                            }
                        }
                    }
                }
            }
        }
        targetP = targetP->next;
    }
}

void observe_step(lwm2m_context_t * contextP,
                  time_t currentTime,
                  time_t * timeoutP)
{
    lwm2m_observed_t * targetP;

    LOG_DBG("Entering");
    for (targetP = contextP->observedList ; targetP != NULL ; targetP = targetP->next)
    {
        lwm2m_watcher_t * watcherP;
        uint8_t * buffer = NULL;
        size_t length = 0;
        lwm2m_data_t * dataP = NULL;
        lwm2m_data_type_t dataType = LWM2M_TYPE_UNDEFINED;
        int size = 0;
        double floatValue = 0;
        int64_t integerValue = 0;
        uint64_t unsignedValue = 0;
        bool storeValue = false;
        coap_packet_t message[1];
        time_t interval;

        // TODO: handle resource instances

        LOG_ARG_DBG("%s", LOG_URI_TO_STRING(&(targetP->uri)));
        if (LWM2M_URI_IS_SET_RESOURCE(&targetP->uri))
        {
            lwm2m_data_t *valueP;

            if (COAP_205_CONTENT != object_readData(contextP, &targetP->uri, &size, &dataP)) continue;
            valueP = dataP;
#ifndef LWM2M_VERSION_1_0
            if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(&targetP->uri)
             && dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE
             && dataP->value.asChildren.count == 1)
            {
                valueP = dataP->value.asChildren.array;
            }
#endif
            dataType = valueP->type;
            switch (dataType)
            {
            case LWM2M_TYPE_INTEGER:
                if (1 != lwm2m_data_decode_int(valueP, &integerValue))
                {
                    lwm2m_data_free(size, dataP);
                    continue;
                }
                storeValue = true;
                break;
            case LWM2M_TYPE_UNSIGNED_INTEGER:
                if (1 != lwm2m_data_decode_uint(valueP, &unsignedValue))
                {
                    lwm2m_data_free(size, dataP);
                    continue;
                }
                storeValue = true;
                break;
            case LWM2M_TYPE_FLOAT:
                if (1 != lwm2m_data_decode_float(valueP, &floatValue))
                {
                    lwm2m_data_free(size, dataP);
                    continue;
                }
                storeValue = true;
                break;
            default:
                break;
            }
        }
        for (watcherP = targetP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
        {
            if (watcherP->active == true)
            {
                bool notify = false;

                if (watcherP->update == true)
                {
                    // value changed, should we notify the server ?

                    if (watcherP->parameters == NULL || watcherP->parameters->toSet == 0)
                    {
                        // no conditions
                        notify = true;
                        LOG_DBG("Notify with no conditions");
                        LOG_ARG_DBG("%s", LOG_URI_TO_STRING(&(targetP->uri)));
                    }

                    if (notify == false
                     && watcherP->parameters != NULL
                     && (watcherP->parameters->toSet & ATTR_FLAG_NUMERIC) != 0)
                    {
                        if ((watcherP->parameters->toSet & LWM2M_ATTR_FLAG_LESS_THAN) != 0)
                        {
                            LOG_DBG("Checking lower threshold");
                            // Did we cross the lower threshold ?
                            switch (dataType)
                            {
                            case LWM2M_TYPE_INTEGER:
                                if ((integerValue < watcherP->parameters->lessThan
                                  && watcherP->lastValue.asInteger > watcherP->parameters->lessThan)
                                 || (integerValue > watcherP->parameters->lessThan
                                  && watcherP->lastValue.asInteger < watcherP->parameters->lessThan))
                                {
                                    LOG_DBG("Notify on lower threshold crossing");
                                    notify = true;
                                }
                                break;
                            case LWM2M_TYPE_UNSIGNED_INTEGER:
                                if ((unsignedValue < watcherP->parameters->lessThan
                                  && watcherP->lastValue.asUnsigned > watcherP->parameters->lessThan)
                                 || (unsignedValue > watcherP->parameters->lessThan
                                  && watcherP->lastValue.asUnsigned < watcherP->parameters->lessThan))
                                {
                                    LOG_DBG("Notify on lower threshold crossing");
                                    notify = true;
                                }
                                break;
                            case LWM2M_TYPE_FLOAT:
                                if ((floatValue < watcherP->parameters->lessThan
                                  && watcherP->lastValue.asFloat > watcherP->parameters->lessThan)
                                 || (floatValue > watcherP->parameters->lessThan
                                  && watcherP->lastValue.asFloat < watcherP->parameters->lessThan))
                                {
                                    LOG_DBG("Notify on lower threshold crossing");
                                    notify = true;
                                }
                                break;
                            default:
                                break;
                            }
                        }
                        if ((watcherP->parameters->toSet & LWM2M_ATTR_FLAG_GREATER_THAN) != 0)
                        {
                            LOG_DBG("Checking upper threshold");
                            // Did we cross the upper threshold ?
                            switch (dataType)
                            {
                            case LWM2M_TYPE_INTEGER:
                                if ((integerValue < watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asInteger > watcherP->parameters->greaterThan)
                                 || (integerValue > watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asInteger < watcherP->parameters->greaterThan))
                                {
                                    LOG_DBG("Notify on lower upper crossing");
                                    notify = true;
                                }
                                break;
                            case LWM2M_TYPE_UNSIGNED_INTEGER:
                                if ((unsignedValue < watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asUnsigned > watcherP->parameters->greaterThan)
                                 || (unsignedValue > watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asUnsigned < watcherP->parameters->greaterThan))
                                {
                                    LOG_DBG("Notify on lower upper crossing");
                                    notify = true;
                                }
                                break;
                            case LWM2M_TYPE_FLOAT:
                                if ((floatValue < watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asFloat > watcherP->parameters->greaterThan)
                                 || (floatValue > watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asFloat < watcherP->parameters->greaterThan))
                                {
                                    LOG_DBG("Notify on lower upper crossing");
                                    notify = true;
                                }
                                break;
                            default:
                                break;
                            }
                        }
                        if ((watcherP->parameters->toSet & LWM2M_ATTR_FLAG_STEP) != 0)
                        {
                            LOG_DBG("Checking step");

                            switch (dataType)
                            {
                            case LWM2M_TYPE_INTEGER:
                            {
                                int64_t diff;

                                diff = integerValue - watcherP->lastValue.asInteger;
                                if ((diff < 0 && (0 - diff) >= watcherP->parameters->step)
                                 || (diff >= 0 && diff >= watcherP->parameters->step))
                                {
                                    LOG_DBG("Notify on step condition");
                                    notify = true;
                                }
                            }
                                break;
                            case LWM2M_TYPE_UNSIGNED_INTEGER:
                            {
                                uint64_t diff;

                                if (unsignedValue >= watcherP->lastValue.asUnsigned)
                                {
                                    diff = unsignedValue - watcherP->lastValue.asUnsigned;
                                }
                                else
                                {
                                    diff = watcherP->lastValue.asUnsigned - unsignedValue;
                                }
                                if (diff >= watcherP->parameters->step)
                                {
                                    LOG_DBG("Notify on step condition");
                                    notify = true;
                                }
                            }
                                break;
                            case LWM2M_TYPE_FLOAT:
                            {
                                double diff;

                                diff = floatValue - watcherP->lastValue.asFloat;
                                if ((diff < 0 && (0 - diff) >= watcherP->parameters->step)
                                 || (diff >= 0 && diff >= watcherP->parameters->step))
                                {
                                    LOG_DBG("Notify on step condition");
                                    notify = true;
                                }
                            }
                                break;
                            default:
                                break;
                            }
                        }
                    }

                    if (watcherP->parameters != NULL
                     && (watcherP->parameters->toSet & LWM2M_ATTR_FLAG_MIN_PERIOD) != 0)
                    {
                        LOG_ARG_DBG("Checking minimal period (%d s)", watcherP->parameters->minPeriod);

                        if ((time_t)(watcherP->lastTime + watcherP->parameters->minPeriod) > currentTime) {
                            // Minimum Period did not elapse yet
                            interval = watcherP->lastTime + watcherP->parameters->minPeriod - currentTime;
                            if (*timeoutP > interval) *timeoutP = interval;
                            notify = false;
                        } else {
                            LOG_DBG("Notify on minimal period");
                            notify = true;
                        }
                    }
                }

                // Is the Maximum Period reached ?
                if (notify == false
                 && watcherP->parameters != NULL
                 && (watcherP->parameters->toSet & LWM2M_ATTR_FLAG_MAX_PERIOD) != 0)
                {
                    LOG_ARG_DBG("Checking maximal period (%d s)", watcherP->parameters->maxPeriod);

                    if ((time_t)(watcherP->lastTime + watcherP->parameters->maxPeriod) <= currentTime) {
                        LOG_DBG("Notify on maximal period");
                        notify = true;
                    }
                }

                if (notify == true)
                {
                    if (buffer == NULL)
                    {
                        if (dataP != NULL)
                        {
                            int res;

                            res = lwm2m_data_serialize(&targetP->uri, size, dataP, &(watcherP->format), &buffer);
                            if (res < 0)
                            {
                                break;
                            }
                            else
                            {
                                length = (size_t)res;
                            }

                        }
                        else
                        {
                            if (COAP_205_CONTENT != object_read(contextP, &targetP->uri, NULL, 0, &(watcherP->format), &buffer, &length))
                            {
                                buffer = NULL;
                                break;
                            }
                        }
                        coap_init_message(message, COAP_TYPE_NON, COAP_205_CONTENT, 0);
                        coap_set_header_content_type(message, watcherP->format);
                        coap_set_payload(message, buffer, length);
                    }
                    watcherP->lastTime = currentTime;
                    watcherP->lastMid = contextP->nextMID++;
                    message->mid = watcherP->lastMid;
                    coap_set_header_token(message, watcherP->token, watcherP->tokenLen);
                    coap_set_header_observe(message, watcherP->counter++);
                    (void)message_send(contextP, message, watcherP->server->sessionH);
                    watcherP->update = false;
                }

                // Store this value
                if (notify == true && storeValue == true)
                {
                    switch (dataType)
                    {
                    case LWM2M_TYPE_INTEGER:
                        watcherP->lastValue.asInteger = integerValue;
                        break;
                    case LWM2M_TYPE_UNSIGNED_INTEGER:
                        watcherP->lastValue.asUnsigned = unsignedValue;
                        break;
                    case LWM2M_TYPE_FLOAT:
                        watcherP->lastValue.asFloat = floatValue;
                        break;
                    default:
                        break;
                    }
                }

                if (watcherP->parameters != NULL && (watcherP->parameters->toSet & LWM2M_ATTR_FLAG_MAX_PERIOD) != 0)
                {
                    // update timers
                    interval = watcherP->lastTime + watcherP->parameters->maxPeriod - currentTime;
                    if (*timeoutP > interval) *timeoutP = interval;
                }
            }
        }
        if (dataP != NULL) lwm2m_data_free(size, dataP);
        if (buffer != NULL) lwm2m_free(buffer);
    }
}

#ifndef LWM2M_VERSION_1_0
int lwm2m_send(lwm2m_context_t *contextP, uint16_t shortServerID, lwm2m_uri_t *urisP, size_t numUris,
               lwm2m_transaction_callback_t callback, void *userData) {
#if defined(LWM2M_SUPPORT_SENML_CBOR) || defined(LWM2M_SUPPORT_SENML_JSON)
    lwm2m_transaction_t *transactionP;
    lwm2m_server_t *targetP;
    lwm2m_data_t *dataP = NULL;
#ifdef LWM2M_SUPPORT_SENML_CBOR
    lwm2m_media_type_t format = LWM2M_CONTENT_SENML_CBOR;
#else
    lwm2m_media_type_t format = LWM2M_CONTENT_SENML_JSON;
#endif
    lwm2m_uri_t uri;
    int ret;
    int size = 0;
    uint8_t *buffer = NULL;
    int length;
    size_t i;
    bool oneGood = false;

    LOG_ARG_DBG("shortServerID: %d", shortServerID);
    for (i = 0; i < numUris; i++) {
        LOG_ARG_DBG("%s", LOG_URI_TO_STRING(urisP + i));
    }

    for (i = 0; i < numUris; i++) {
        if (!LWM2M_URI_IS_SET_OBJECT(urisP + i))
            return COAP_400_BAD_REQUEST;
        if (!LWM2M_URI_IS_SET_INSTANCE(urisP + i) && LWM2M_URI_IS_SET_RESOURCE(urisP + i))
            return COAP_400_BAD_REQUEST;
    }

    ret = object_readCompositeData(contextP, urisP, numUris, &size, &dataP);
    if (ret != COAP_205_CONTENT)
        return ret;

    LWM2M_URI_RESET(&uri);
    if (size == 1) {
        uri.objectId = dataP->id;
        if (dataP->value.asChildren.count == 1) {
            uri.instanceId = dataP->value.asChildren.array->id;
        }
    }
    ret = lwm2m_data_serialize(&uri, size, dataP, &format, &buffer);
    lwm2m_data_free(size, dataP);
    if (ret < 0) {
        return COAP_500_INTERNAL_SERVER_ERROR;
    } else {
        length = ret;
    }

    if (shortServerID == 0 && contextP->serverList != NULL && contextP->serverList->next == NULL) {
        // Only 1 server
        shortServerID = contextP->serverList->shortID;
    }

    ret = COAP_404_NOT_FOUND;
    for (targetP = contextP->serverList; targetP != NULL; targetP = targetP->next) {
        bool mute = true;
        lwm2m_data_t *muteDataP;
        int muteSize;

        if (shortServerID != 0 && shortServerID != targetP->shortID)
            continue;
        if (targetP->sessionH == NULL ||
            (targetP->status != STATE_REGISTERED && targetP->status != STATE_REG_UPDATE_PENDING &&
             targetP->status != STATE_REG_UPDATE_NEEDED && targetP->status != STATE_REG_FULL_UPDATE_NEEDED)) {
            if (ret == COAP_404_NOT_FOUND)
                ret = COAP_405_METHOD_NOT_ALLOWED;
            if (shortServerID == 0)
                continue;
            break;
        }

        LWM2M_URI_RESET(&uri);
        uri.objectId = LWM2M_SERVER_OBJECT_ID;
        uri.instanceId = targetP->servObjInstID;
        uri.resourceId = LWM2M_SERVER_MUTE_SEND_ID;
        if (COAP_205_CONTENT == object_readData(contextP, &uri, &muteSize, &muteDataP)) {
            lwm2m_data_decode_bool(muteDataP, &mute);
            lwm2m_data_free(muteSize, muteDataP);
        }
        if (mute) {
            if (ret == COAP_404_NOT_FOUND)
                ret = COAP_405_METHOD_NOT_ALLOWED;
            if (shortServerID == 0)
                continue;
            break;
        }

        LWM2M_URI_RESET(&uri);
        transactionP = transaction_new(targetP->sessionH, COAP_POST, NULL, &uri, contextP->nextMID++, 0, NULL);
        if (transactionP == NULL) {
            ret = COAP_500_INTERNAL_SERVER_ERROR;
            // Going to the next server likely won't fix this, just get out.
            break;
        }

        coap_set_header_uri_path(transactionP->message, "/" URI_SEND_SEGMENT);
        coap_set_header_content_type(transactionP->message, format);
        if (!transaction_set_payload(transactionP, buffer, length)) {
            transaction_free(transactionP);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        transactionP->callback = callback;
        transactionP->userData = userData;

        contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transactionP);

        ret = transaction_send(contextP, transactionP);
        if (ret == NO_ERROR) {
            oneGood = true;
        } else {
            LOG_ARG_DBG("transaction_send failed for %d: 0x%02X!", targetP->shortID, ret);
        }
        coap_set_payload(transactionP->message, NULL, 0); // Clear the payload
        if (shortServerID != 0)
            break;
    }
    if (buffer) {
        lwm2m_free(buffer);
    }
    if (oneGood)
        ret = NO_ERROR;
    return ret;
#else
    /* Unused parameters */
    (void)contextP;
    (void)shortServerID;
    (void)urisP;
    (void)numUris;
    (void)callback;
    (void)userData;
    return COAP_415_UNSUPPORTED_CONTENT_FORMAT;
#endif
}
#endif

#endif

#ifdef LWM2M_SERVER_MODE

typedef struct
{
    uint16_t                        client;
    lwm2m_uri_t                     uri;
    lwm2m_result_callback_t         callbackP;
    void *                          userDataP;
    lwm2m_context_t *               contextP;
} cancellation_data_t;

static lwm2m_observation_t * prv_findObservationByURI(lwm2m_client_t * clientP,
                                                      lwm2m_uri_t * uriP)
{
    lwm2m_observation_t * targetP;

    targetP = clientP->observationList;
    while (targetP != NULL)
    {
        if (targetP->uri.objectId == uriP->objectId
         && targetP->uri.instanceId == uriP->instanceId
         && targetP->uri.resourceId == uriP->resourceId
#ifndef LWM2M_VERSION_1_0
         && targetP->uri.resourceInstanceId == uriP->resourceInstanceId
#endif
           )
        {
            return targetP;
        }

        targetP = targetP->next;
    }

    return targetP;
}

void observe_remove(lwm2m_observation_t * observationP)
{
    LOG_DBG("Entering");
    observationP->clientP->observationList = (lwm2m_observation_t *) LWM2M_LIST_RM(observationP->clientP->observationList, observationP->id, NULL);
    lwm2m_free(observationP);
}

static void prv_obsRequestCallback(lwm2m_context_t * contextP,
                                   lwm2m_transaction_t * transacP,
                                   void * message)
{
    lwm2m_observation_t * observationP = NULL;
    observation_data_t * observationData = (observation_data_t *)transacP->userData;
    coap_packet_t * packet = (coap_packet_t *)message;
    uint8_t code;
    lwm2m_client_t * clientP;
    lwm2m_uri_t * uriP = & observationData->uri;
    uint32_t block_num = 0;
    uint16_t block_size = 0;
    uint8_t block_more = 0;
    block_info_t block_info;

    (void)contextP; /* unused */

    clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)observationData->contextP->clientList,
                                                observationData->client);
    if (clientP == NULL) {
        // No client matching this notification, inform request callback with an error code.
        observationData->callback(contextP, observationData->client, &observationData->uri,
                                  COAP_500_INTERNAL_SERVER_ERROR, //?
                                  NULL, LWM2M_CONTENT_TEXT, NULL, 0, observationData->userData);
        transaction_free_userData(contextP, transacP);
        return;
    }

    observationP = prv_findObservationByURI(clientP, uriP);

    // Fail it if the latest user intention is cancellation
    if(observationP && observationP->status == STATE_DEREG_PENDING)
    {
        code = COAP_400_BAD_REQUEST;
    }
    else if (message == NULL)
    {
        code = COAP_503_SERVICE_UNAVAILABLE;
    }
    else if (packet->code == COAP_205_CONTENT
            && !IS_OPTION(packet, COAP_OPTION_OBSERVE))
    {
        code = COAP_405_METHOD_NOT_ALLOWED;
    }
    else
    {
        code = packet->code;
    }

    if (code != COAP_205_CONTENT) {
        // Some kind of error occurred, call the request callback with an error code
        observationData->callback(contextP, observationData->client, &observationData->uri,
                                  code, //?
                                  NULL, LWM2M_CONTENT_TEXT, NULL, 0, observationData->userData);
    } else if (IS_OPTION(packet, COAP_OPTION_BLOCK2) && packet->block2_more) {
        // Call request callback with partial block2 content.
        observationData->callback(contextP, observationData->client, &observationData->uri,
                                  code, //?
                                  NULL, utils_convertMediaType(packet->content_type), packet->payload,
                                  packet->payload_len, observationData->userData);
    } else {
        int has_block2 = coap_get_header_block2(packet, &block_num, &block_more, &block_size, NULL);
        if (has_block2) {
            block_info.block_num = block_num;
            block_info.block_size = block_size;
            block_info.block_more = block_more;
        }

        if (observationP == NULL) {
            observationP = (lwm2m_observation_t *)lwm2m_malloc(sizeof(*observationP));
            if (observationP == NULL) {
                transaction_free_userData(contextP, transacP);
                return;
            }
            memset(observationP, 0, sizeof(*observationP));
        } else {
            observationP->clientP->observationList = (lwm2m_observation_t *) LWM2M_LIST_RM(observationP->clientP->observationList, observationP->id, NULL);

            // give the user chance to free previous observation userData
            // indicator: COAP_202_DELETED and (Length ==0)
            observationData->callback(contextP,
                                      observationData->client,
                                      &observationData->uri,
                                      COAP_202_DELETED,
                                      NULL,
                                      LWM2M_CONTENT_TEXT, NULL, 0,
                                      observationData->userData);
        }

        observationP->id = observationData->id;
        observationP->clientP = clientP;

        observationP->callback = observationData->callback;
        observationP->userData = observationData->userData;
        observationP->status = STATE_REGISTERED;
        memcpy(&observationP->uri, uriP, sizeof(lwm2m_uri_t));

        observationP->clientP->observationList = (lwm2m_observation_t *)LWM2M_LIST_ADD(observationP->clientP->observationList, observationP);

        const int status = 0;

        if (has_block2) {
            observationData->callback(contextP,
                                      observationData->client,
                                      &observationData->uri,
                                      status,
                                      &block_info,
                                      utils_convertMediaType(packet->content_type),
                                      packet->payload,
                                      packet->payload_len,
                                      observationData->userData);
        } else {
            observationData->callback(contextP,
                                      observationData->client,
                                      &observationData->uri,
                                      status,
                                      NULL,
                                      utils_convertMediaType(packet->content_type),
                                      packet->payload,
                                      packet->payload_len,
                                      observationData->userData);
        }
    }
    transaction_free_userData(contextP, transacP);
}

static void prv_obsCancelRequestCallback(lwm2m_context_t * contextP,
                                         lwm2m_transaction_t * transacP,
                                         void * message)
{
    cancellation_data_t * cancelP = (cancellation_data_t *)transacP->userData;
    coap_packet_t * packet = (coap_packet_t *)message;
    uint8_t code;
    uint32_t block_num = 0;
    uint16_t block_size = 0;
    uint8_t block_more = 0;
    block_info_t block_info;

    (void)contextP; /* unused */

    lwm2m_client_t *clientP =
        (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)cancelP->contextP->clientList, cancelP->client);
    if (clientP == NULL)
    {
        cancelP->callbackP(contextP, cancelP->client, &cancelP->uri,
                           COAP_500_INTERNAL_SERVER_ERROR, //?
                           NULL, LWM2M_CONTENT_TEXT, NULL, 0, cancelP->userDataP);
        transaction_free_userData(contextP, transacP);
        return;
    }

    lwm2m_observation_t * observationP = prv_findObservationByURI(clientP, &cancelP->uri);

    if (message == NULL)
    {
        code = COAP_503_SERVICE_UNAVAILABLE;
    }
    else
    {
        code = packet->code;
    }

    if (code != COAP_205_CONTENT)
    {
        cancelP->callbackP(contextP, cancelP->client, &cancelP->uri,
                           code, //?
                           NULL, LWM2M_CONTENT_TEXT, NULL, 0, cancelP->userDataP);
        transaction_free_userData(contextP, transacP);
        return;
    }
    else
    {
        const int status = 0;
        int has_block2 = coap_get_header_block2(message, &block_num, &block_more, &block_size, NULL);
        if (has_block2) {
            block_info.block_num = block_num;
            block_info.block_size = block_size;
            block_info.block_more = block_more;
        }
        if (has_block2)
        {
            cancelP->callbackP(contextP,
                               cancelP->client,
                               &cancelP->uri,
                               status,  //?
                               &block_info,
                               utils_convertMediaType(packet->content_type),
                               packet->payload,
                               packet->payload_len,
                               cancelP->userDataP);
        }
        else
        {
            cancelP->callbackP(contextP,
                               cancelP->client,
                               &cancelP->uri,
                               status,  //?
                               NULL,
                               utils_convertMediaType(packet->content_type),
                               packet->payload,
                               packet->payload_len,
                               cancelP->userDataP);
        }

        if (!has_block2 || !block_info.block_more) {
            // Remove observation only if there is no block transfer or if
            // its the last block.
            observe_remove(observationP);
        }
    }
    transaction_free_userData(contextP, transacP);
}

static
int prv_lwm2m_observe(lwm2m_context_t * contextP,
        uint16_t clientID,
        lwm2m_uri_t * uriP,
        lwm2m_result_callback_t callback,
        void * userData)
{
    lwm2m_client_t * clientP;
    lwm2m_transaction_t * transactionP;
    observation_data_t * observationData;
    lwm2m_observation_t * observationP;
    uint8_t token[4];

    LOG_ARG_DBG("clientID: %d", clientID);
    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));

    if (!LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)) return COAP_400_BAD_REQUEST;

    clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
    if (clientP == NULL) return COAP_404_NOT_FOUND;

    observationP = prv_findObservationByURI(clientP, uriP);

    observationData = (observation_data_t *)lwm2m_malloc(sizeof(observation_data_t));
    if (observationData == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(observationData, 0, sizeof(observation_data_t));

    observationData->id = ++clientP->observationId;

    // observationId can overflow. ensure new ID is not already present
    if(lwm2m_list_find((lwm2m_list_t *)clientP->observationList, observationData->id))
    {
        LOG_DBG("Can't get available observation ID. Request failed.\n");
        lwm2m_free(observationData);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    memcpy(&observationData->uri, uriP, sizeof(lwm2m_uri_t));

    // don't hold refer to the clientP
    observationData->client = clientP->internalID;
    observationData->callback = callback;
    observationData->userData = userData;
    observationData->contextP = contextP;

    token[0] = clientP->internalID >> 8;
    token[1] = clientP->internalID & 0xFF;
    token[2] = observationData->id >> 8;
    token[3] = observationData->id & 0xFF;

    transactionP = transaction_new(clientP->sessionH, COAP_GET, clientP->altPath, uriP, contextP->nextMID++, 4, token);
    if (transactionP == NULL)
    {
        lwm2m_free(observationData);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    coap_set_header_observe(transactionP->message, 0);
    coap_set_header_accept(transactionP->message, clientP->format);

    transactionP->callback = prv_obsRequestCallback;
    transactionP->userData = (void *)observationData;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transactionP);

    // update the user latest intention
    if(observationP) observationP->status = STATE_REG_PENDING;

    int ret = transaction_send(contextP, transactionP);
    if (ret != 0)
    {
        LOG_DBG("transaction_send failed!");
        lwm2m_free(observationData);
    }
    return ret;
}

int lwm2m_observe(lwm2m_context_t * contextP,
        uint16_t clientID,
        lwm2m_uri_t * uriP,
        lwm2m_result_callback_t callback,
        void * userData)
{
    return prv_lwm2m_observe(contextP,
                             clientID,
                             uriP,
                             callback,
                             userData);
}

static
int prv_lwm2m_observe_cancel(lwm2m_context_t * contextP,
        uint16_t clientID,
        lwm2m_uri_t * uriP,
        lwm2m_result_callback_t callback,
        void * userData)
{
    lwm2m_client_t * clientP;
    lwm2m_observation_t * observationP;
    int ret;

    LOG_ARG_DBG("clientID: %d", clientID);
    LOG_ARG_DBG("%s", LOG_URI_TO_STRING(uriP));

    clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
    if (clientP == NULL) return COAP_404_NOT_FOUND;

    observationP = prv_findObservationByURI(clientP, uriP);
    if (observationP == NULL) return COAP_404_NOT_FOUND;

    switch (observationP->status)
    {
    case STATE_REGISTERED:
    {
        lwm2m_transaction_t * transactionP;
        cancellation_data_t * cancelP;
        uint8_t token[4];

        token[0] = clientP->internalID >> 8;
        token[1] = clientP->internalID & 0xFF;
        token[2] = observationP->id >> 8;
        token[3] = observationP->id & 0xFF;

        transactionP = transaction_new(clientP->sessionH, COAP_GET, clientP->altPath, uriP, contextP->nextMID++, 4, token);
        if (transactionP == NULL)
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        cancelP = (cancellation_data_t *)lwm2m_malloc(sizeof(cancellation_data_t));
        if (cancelP == NULL)
        {
            lwm2m_free(transactionP);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        coap_set_header_observe(transactionP->message, 1);

        // don't hold refer to the clientP
        cancelP->client = clientP->internalID;
        memcpy(&cancelP->uri, uriP, sizeof(lwm2m_uri_t));
        cancelP->callbackP = callback;
        cancelP->userDataP = userData;
        cancelP->contextP = contextP;

        transactionP->callback = prv_obsCancelRequestCallback;
        transactionP->userData = (void *)cancelP;

        contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transactionP);

        observationP->status = STATE_DEREG_PENDING;

        ret = transaction_send(contextP, transactionP);
        if (ret != 0) lwm2m_free(cancelP);
        return ret;
    }

    case STATE_REG_PENDING:
        observationP->status = STATE_DEREG_PENDING;
        ret = COAP_204_CHANGED;
        break;

    default:
        // Should not happen
        ret = COAP_IGNORE;
        break;
    }

    // no other chance to remove the observationP since not sending a transaction
    observe_remove(observationP);

    // need to give a indicator (non-zero) to user for properly freeing the userData
    return ret;
}


int lwm2m_observe_cancel(lwm2m_context_t * contextP,
        uint16_t clientID,
        lwm2m_uri_t * uriP,
        lwm2m_result_callback_t callback,
        void * userData)
{
    return prv_lwm2m_observe_cancel(contextP, clientID, uriP, callback, userData);
}

bool observe_handleNotify(lwm2m_context_t * contextP,
                           void * fromSessionH,
                           coap_packet_t * message,
        				   coap_packet_t * response)
{
    uint8_t * tokenP;
    int token_len;
    uint16_t clientID;
    uint16_t obsID;
    lwm2m_client_t * clientP;
    lwm2m_observation_t * observationP;
    uint32_t count;

    LOG_DBG("Entering");
    token_len = coap_get_header_token(message, &tokenP);
    if (token_len != sizeof(uint32_t)) return false;

    if (1 != coap_get_header_observe(message, &count)) return false;

    clientID = (tokenP[0] << 8) | tokenP[1];
    obsID = (tokenP[2] << 8) | tokenP[3];

    clientP = (lwm2m_client_t *)lwm2m_list_find((lwm2m_list_t *)contextP->clientList, clientID);
    if (clientP == NULL) return false;

    observationP = (lwm2m_observation_t *)lwm2m_list_find((lwm2m_list_t *)clientP->observationList, obsID);
    if (observationP == NULL)
    {
        coap_init_message(response, COAP_TYPE_RST, 0, message->mid);
        message_send(contextP, response, fromSessionH);
    }
    else
    {
        if (message->type == COAP_TYPE_CON ) {
            coap_init_message(response, COAP_TYPE_ACK, 0, message->mid);
            message_send(contextP, response, fromSessionH);
        }

        /*
         * Handle notify callback
         * TODO: status is misused by notify counter value. Issue #521
         */
        uint32_t block_num = 0;
        uint16_t block_size = 0;
        uint8_t block_more = 0;

        if (coap_get_header_block2(message, &block_num, &block_more, &block_size, NULL)) {
            block_info_t block_info;
            block_info.block_num = block_num;
            block_info.block_size = block_size;
            block_info.block_more = block_more;
            observationP->callback(contextP,
                                   clientID,
                                   &observationP->uri,
                                   (int)count,
                                   &block_info,
                                   utils_convertMediaType(message->content_type), message->payload, message->payload_len,
                                   observationP->userData);
        } else {
            observationP->callback(contextP,
                                   clientID,
                                   &observationP->uri,
                                   (int)count,
                                   NULL,
                                   utils_convertMediaType(message->content_type), message->payload, message->payload_len,
                                   observationP->userData);
        }
    }
    return true;
}
#endif
