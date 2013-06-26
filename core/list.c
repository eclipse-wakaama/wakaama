/*
Copyright (c) 2013, Intel Corporation

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


lwm2m_list_t * lwm2m_list_add(lwm2m_list_t * head,
                              lwm2m_list_t * node)
{
    lwm2m_list_t * target;

    if (NULL == head) return node;

    if (head->id > node->id)
    {
        node->next = head;
        return node;
    }

    target = head;
    while (NULL != target->next && target->next->id < node->id)
    {
        target = target->next;
    }

    node->next = target->next;
    target->next = node;

    return head;
}


lwm2m_list_t * lwm2m_list_find(lwm2m_list_t * head,
                               uint16_t id)
{
    while (NULL != head && head->id < id)
    {
        head = head->next;
    }

    if (NULL != head && head->id == id) return head;

    return NULL;
}


lwm2m_list_t * lwm2m_list_remove(lwm2m_list_t * head,
                                 uint16_t id,
                                 lwm2m_list_t ** nodeP)
{
    lwm2m_list_t * target;

    if (head->id == id)
    {
        *nodeP = head;
        return head->next;
    }

    target = head;
    while (NULL != target->next && target->next->id < id)
    {
        target = target->next;
    }

    if (NULL != target->next && target->next->id == id)
    {
        *nodeP = target->next;
        target->next = target->next->next;
    }
    else
    {
        *nodeP = NULL;
    }

    return head;
}
