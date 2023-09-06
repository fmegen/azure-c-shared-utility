// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <assert.h>
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/doublylinkedlist.h"
#include "azure_c_shared_utility/mru_cache.h"
#include "azure_c_shared_utility/agenttime.h"

typedef struct MRU_CACHE_ENTRY_TAG
{
    const char* id;
    SMART_PTR_HANDLE dataHandle;
    time_t created;
    int expiryInSec;
    
    DLIST_ENTRY link;
} MRU_CACHE_ENTRY;

typedef struct MRU_CACHE_TAG
{
    int maxItems;
    int numItems;

    DLIST_ENTRY list;
} MRU_CACHE;

static void free_entry(MRU_CACHE_ENTRY* entry)
{
    if (entry == NULL)
    {
        return;
    }

    if (entry->id != NULL)
    {
        free((void*)entry->id);
        entry->id = NULL;
    }

    SMART_PTR_delete(entry->dataHandle);
    entry->dataHandle = NULL;

    free(entry);
}

static MRU_CACHE_ENTRY* entry_from_dlist(PDLIST_ENTRY dlist)
{
    size_t resolved = (size_t)dlist - (size_t)offsetof(MRU_CACHE_ENTRY, link);
    MRU_CACHE_ENTRY* ptr = (MRU_CACHE_ENTRY*)resolved;
    assert(&(ptr->link) == dlist);
    return ptr;
}

static void remove_entry(MRU_CACHE* instance, MRU_CACHE_ENTRY* entry)
{
    if (entry == NULL)
    {
        return;
    }

    assert(instance);

    // NOTE: return value is odd here and doesn't indicate errors. Instead it is 0 if the list is not empty,
    //       and non-zero if it is
    DList_RemoveEntryList(&(entry->link));
    instance->numItems--;
    assert(instance->numItems >= 0);
}

static void insert_at_start(MRU_CACHE* instance, MRU_CACHE_ENTRY* entry)
{
    if (entry == NULL)
    {
        return;
    }
    
    assert(instance);

    DList_InsertHeadList(&(instance->list), &(entry->link));
    instance->numItems++;
    assert(instance->numItems > 0);
}

static int is_expired(MRU_CACHE_ENTRY* entry)
{
    assert(entry);

    if (entry->expiryInSec < 0)
    {
        // negative values mean never expires
        return 0;
    }

    time_t now = get_time(NULL);
    double deltaInSec = get_difftime(now, entry->created);
    return deltaInSec > entry->expiryInSec;
}

static int find(const char* id, PDLIST_ENTRY list, MRU_CACHE_ENTRY** entry)
{
    if (id == NULL)
    {
        return __FAILURE__;
    }
    else if (list == NULL)
    {
        return __FAILURE__;
    }
    else if (entry == NULL)
    {
        return __FAILURE__;
    }

    *entry = NULL;
    PDLIST_ENTRY currentNode = list->Flink;

    while (currentNode != list)
    {
        MRU_CACHE_ENTRY* current = entry_from_dlist(currentNode);
        int comp = strncmp(id, current->id, MRU_CACHE_MAX_ID_LEN);
        if (comp == 0)
        {
            *entry = current;
        }
        break;

        currentNode = currentNode->Flink;
    }

    return 0;
}

MRU_CACHE_HANDLE MRU_CACHE_new(int maxItems)
{
    if (maxItems < 0)
    {
        return NULL;
    }

    MRU_CACHE* instance = (MRU_CACHE*)calloc(1, sizeof(MRU_CACHE));

    if (instance != NULL)
    {
        instance->maxItems = maxItems;
        DList_InitializeListHead(&(instance->list));
    }

    return instance;
}

void MRU_CACHE_delete(MRU_CACHE_HANDLE handle)
{
    if (handle == NULL)
    {
        return;
    }

    MRU_CACHE* instance = (MRU_CACHE*)handle;
    MRU_CACHE_clear(instance);
    
    free(instance);
}

int MRU_CACHE_add(MRU_CACHE_HANDLE handle, const char* id, SMART_PTR_HANDLE data, int expiryTimeSec)
{
    if (handle == NULL)
    {
        return __FAILURE__;
    }
    else if (id == NULL)
    {
        return __FAILURE__;
    }
    else if (strlen(id) > MRU_CACHE_MAX_ID_LEN)
    {
        return __FAILURE__;
    }

    // no point in caching empty data
    if (!SMART_PTR_has_value(data))
    {
        return 0;
    }

    MRU_CACHE* instance = (MRU_CACHE*)handle;
    MRU_CACHE_ENTRY* entry = NULL;

    int ret = find(id, &(instance->list), &entry);
    if (ret)
    {
        return __FAILURE__;
    }

    if (entry == NULL)
    {
        entry = (MRU_CACHE_ENTRY*)calloc(1, sizeof(MRU_CACHE_ENTRY));
        if (entry == NULL)
        {
            return __FAILURE__;
        }

        entry->created = time(NULL);
        entry->expiryInSec = expiryTimeSec;
        entry->dataHandle = SMART_PTR_copy(data);
        if (entry->dataHandle == NULL)
        {
            free_entry(entry);
            return __FAILURE__;
        }

        ret = mallocAndStrcpy_s((char **)&entry->id, id);
        if (entry->id == NULL)
        {
            free_entry(entry);
            return __FAILURE__;
        }
    }
    else
    {
        SMART_PTR_HANDLE copy = SMART_PTR_copy(data);
        if (copy == NULL)
        {
            return __FAILURE__;
        }

        if (entry->dataHandle != NULL)
        {
            SMART_PTR_delete(entry->dataHandle);
        }

        entry->created = get_time(NULL);
        entry->expiryInSec = expiryTimeSec;
        entry->dataHandle = copy;
        copy = NULL;

        // This is a most recently used cache. Every time an item is added or updated, it gets moved to
        // the front of the list
        remove_entry(instance, entry);
    }

    insert_at_start(instance, entry);

    // are we now over capacity?
    assert(instance->numItems >= 0);
    if (instance->numItems > instance->maxItems)
    {
        MRU_CACHE_ENTRY* lastEntry = entry_from_dlist(instance->list.Blink);
        remove_entry(instance, lastEntry);
        free_entry(lastEntry);
    }

    return 0;
}

static int get_from_cache(MRU_CACHE_HANDLE handle, int includeExpired, const char* id, SMART_PTR_HANDLE* data)
{
    if (handle == NULL)
    {
        return __FAILURE__;
    }
    else if (id == NULL)
    {
        return __FAILURE__;
    }
    else if (strlen(id) > MRU_CACHE_MAX_ID_LEN)
    {
        return __FAILURE__;
    }
    else if (data == NULL)
    {
        return __FAILURE__;
    }

    MRU_CACHE* instance = (MRU_CACHE*)handle;
    
    MRU_CACHE_ENTRY* entry = NULL;
    int ret = find(id, &(instance->list), &entry);
    if (ret)
    {
        return __FAILURE__;
    }

    if (entry == NULL || (!includeExpired && is_expired(entry)))
    {
        *data = NULL;
    }
    else
    {
        // this is a MRU cache. Since we accessed this entry, move it to the front of the list
        remove_entry(instance, entry);
        insert_at_start(instance, entry);
        *data = SMART_PTR_copy(entry->dataHandle);
    }

    return 0;
}

int MRU_CACHE_get(MRU_CACHE_HANDLE handle, const char* id, SMART_PTR_HANDLE* data)
{
    return get_from_cache(handle, false, id, data);
}

int MRU_CACHE_get_include_expired(MRU_CACHE_HANDLE handle, const char* id, SMART_PTR_HANDLE* data)
{
    return get_from_cache(handle, true, id, data);
}

int MRU_CACHE_size(MRU_CACHE_HANDLE handle, int* size)
{
    if (handle == NULL)
    {
        return __FAILURE__;
    }
    else if (size == NULL)
    {
        return __FAILURE__;
    }

    *size = ((MRU_CACHE*)handle)->numItems;
    return 0;
}

int MRU_CACHE_prune(MRU_CACHE_HANDLE handle)
{
    if (handle == NULL)
    {
        return __FAILURE__;
    }

    MRU_CACHE* instance = (MRU_CACHE*)handle;
    PDLIST_ENTRY list = &(instance->list);

    PDLIST_ENTRY current = list->Flink;
    while (current != list)
    {
        MRU_CACHE_ENTRY* entry = entry_from_dlist(current);
        current = current->Flink;

        if (is_expired(entry))
        {
            remove_entry(instance, entry);
            free_entry(entry);
        }
    }

    return 0;
}

int MRU_CACHE_clear(MRU_CACHE_HANDLE handle)
{
    if (handle == NULL)
    {
        return __FAILURE__;
    }

    MRU_CACHE* instance = (MRU_CACHE*)handle;
    PDLIST_ENTRY list = &(instance->list);

    PDLIST_ENTRY current = list->Flink;
    while (current != list)
    {
        MRU_CACHE_ENTRY* entry = entry_from_dlist(current);
        current = current->Flink;
        
        free_entry(entry);
    }

    instance->numItems = 0;
    DList_InitializeListHead(list);
    return 0;
}
