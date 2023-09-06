// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <assert.h>
#include "azure_c_shared_utility/smart_ptr.h"
#include "azure_c_shared_utility/lock.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SMART_PTR_REF_TAG
{
    int refCount;
    void* data;
    SMART_PTR_FREE_FUNC freeFunc;
} SMART_PTR_REF;

typedef struct SMART_PTR_TAG
{
    LOCK_HANDLE lock;
    SMART_PTR_REF* ref;
} SMART_PTR;

static void free_ref(SMART_PTR_REF* ref)
{
    if (ref == NULL)
    {
        return;
    }

    assert(ref->refCount == 0);
    assert(ref->freeFunc);

    ref->freeFunc(ref->data);
    ref->data = NULL;

    free(ref);
}

SMART_PTR_HANDLE SMART_PTR_new()
{
    SMART_PTR_HANDLE handle = SMART_PTR_create(NULL, free);

    // sanity checks
    assert(handle->lock);
    assert(handle->ref);
    return handle;
}

SMART_PTR_HANDLE SMART_PTR_create(void* ptr, SMART_PTR_FREE_FUNC freeFunc)
{
    if (freeFunc == NULL)
    {
        return NULL;
    }

    SMART_PTR_HANDLE handle = (SMART_PTR_HANDLE)calloc(1, sizeof(SMART_PTR));
    if (handle == NULL)
    {
        return NULL;
    }
    
    handle->lock = Lock_Init();
    if (handle->lock == NULL)
    {
        SMART_PTR_delete(handle);
        return NULL;
    }

    handle->ref = (SMART_PTR_REF*)malloc(sizeof(SMART_PTR_REF));
    if (handle->ref == NULL)
    {
        SMART_PTR_delete(handle);
        return NULL;
    }

    handle->ref->refCount = 1;
    handle->ref->data = ptr;
    handle->ref->freeFunc = freeFunc;

    // sanity checks
    assert(handle->lock);
    assert(handle->ref);
    return handle;
}

SMART_PTR_HANDLE SMART_PTR_copy(SMART_PTR_HANDLE handle)
{
    // TODO optimize to simply update the reference count and return the same handle?
    if (handle == NULL)
    {
        return NULL;
    }

    assert(handle->lock);

    SMART_PTR_HANDLE copy = (SMART_PTR_HANDLE)malloc(sizeof(SMART_PTR));
    if (copy == NULL)
    {
        return NULL;
    }

    copy->lock = handle->lock;
    copy->ref = handle->ref;

    // update ref count
    int lockResult = Lock(handle->lock);
    if (lockResult != LOCK_OK)
    {
        free(copy);
        return NULL;
    }
    else
    {
        // running under lock
        copy->ref->refCount++;
    }

    Unlock(handle->lock);
    
    // sanity check
    assert(copy->lock);
    assert(copy->ref);
    return copy;
}

void SMART_PTR_delete(SMART_PTR_HANDLE handle)
{
    if (handle == NULL)
    {
        return;
    }

    if (handle->lock != NULL)
    {
        Lock(handle->lock);
    }

    int refCount = 0;

    if (handle->ref != NULL)
    {
        // decrement the reference count
        refCount = --handle->ref->refCount;
        assert(refCount >= 0);

        // is this the last reference?
        if (refCount == 0)
        {
            free_ref(handle->ref);
            handle->ref == NULL;
        }
    }
    
    if (handle->lock)
    {
        Unlock(handle->lock);

        if (refCount == 0)
        {
            Lock_Deinit(handle->lock);
            handle->lock = NULL;
        }
    }

    free(handle);
}

void* SMART_PTR_get(SMART_PTR_HANDLE handle)
{
    if (handle == NULL)
    {
        return NULL;
    }

    int ret = Lock(handle->lock);
    if (ret != LOCK_OK)
    {
        return NULL;
    }

    void* ptr = handle->ref->data;
    
    ret = Unlock(handle->lock);
    if (ret != LOCK_OK)
    {
        return NULL;
    }

    return ptr;
}

int SMART_PTR_has_value(SMART_PTR_HANDLE handle)
{
    return SMART_PTR_get(handle) != NULL;
}

#ifdef __cplusplus
}
#endif