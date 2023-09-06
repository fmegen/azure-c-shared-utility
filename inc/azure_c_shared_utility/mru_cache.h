// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MRU_CACHE_H
#define MRU_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/smart_ptr.h"

#define MRU_CACHE_MAX_ID_LEN 300
typedef struct MRU_CACHE_TAG* MRU_CACHE_HANDLE;

/**
 * @brief               Creates a new most recently used (MRU) cache
 * @param maxItems      The maximum number of items the cache can contain
 * @returns             A MRU_CACHE_HANDLE if no failures occur, or NULL otherwise
 */
MOCKABLE_FUNCTION(, MRU_CACHE_HANDLE, MRU_CACHE_new, int, maxItems);

/**
 * @brief               Deletes the MRU cache and frees up all memory. The handle is no longer valid once
 *                      this function returns
 * @param handle        The MRU cache handle
 */
MOCKABLE_FUNCTION(, void, MRU_CACHE_delete, MRU_CACHE_HANDLE, handle);

/**
 * @brief               Adds an entry to the MRU cache. If there is already an existing entry, it will be
 *                      updated, and then moved to the front of the list (becoming most recently used one)
 * @param handle        The MRU cache handle
 * @param id            The unique identifier for this cache entry
 * @param data          The smart pointer handle for the data to be stored
 * @param expiryTimeSec The number of seconds this cache entry will be valid for. Set to -1 to always be valid
 * @returns             0 on success, or an error code on failure. In the case of errors, the MRU cache
 *                      will not be updated
 */
MOCKABLE_FUNCTION(, int, MRU_CACHE_add, MRU_CACHE_HANDLE, handle, const char*, id, SMART_PTR_HANDLE, data, int, expriryTimeSec);

/**
 * @brief               Gets an entry from the MRU cache. If there is an entry that is NOT expired, it will be
 *                      moved to the front of the list thereby becoming the most recently used entry
 * @param handle        The MRU cache handle
 * @param id            The unique identifier for this cache entry
 * @param data          The smart pointer handle to set to the stored data
 * @returns             0 if the MRU cache was successfully searched. In the case there was an entry, the data
 *                      will be set to a copy of the smart pointer handle. In the case there was no such entry,
 *                      data will set to NULL. An non-zero error code is returned in the case of errors (e.g.
 *                      bad arguments passed)
 */
MOCKABLE_FUNCTION(, int, MRU_CACHE_get, MRU_CACHE_HANDLE, handle, const char*, id, SMART_PTR_HANDLE*, data);

/**
 * @brief               Gets an entry from the MRU cache including any expired items. If there is an entry, it
 *                      will be moved to the front of the list thereby becoming the most recently used entry
 * @param handle        The MRU cache handle
 * @param id            The unique identifier for this cache entry
 * @param data          The smart pointer handle to set to the stored data
 * @returns             0 if the MRU cache was successfully searched. In the case there was an entry, the data
 *                      will be set to a copy of the smart pointer handle. In the case there was no such entry,
 *                      data will set to NULL. An non-zero error code is returned in the case of errors (e.g.
 *                      bad arguments passed)
 */
MOCKABLE_FUNCTION(, int, MRU_CACHE_get_include_expired, MRU_CACHE_HANDLE, handle, const char*, id, SMART_PTR_HANDLE*, data);

/**
 * @brief               Gets the number of entries in the MRU cache
 * @param handle        The MRU cache handle
 * @param size          The pointer to size variable to set
 * @returns             0 on success in which case size will be set to the number of entries in the list.
 *                      A non-zero value will be returned in the case of errors (e.g. bad arguments)
 *                      case of errors (e.g. bad arguments passed)
 */
MOCKABLE_FUNCTION(, int, MRU_CACHE_size, MRU_CACHE_HANDLE, handle, int*, size);

/**
 * @brief               Removes all expired entries from the MRU cache
 * @param handle        The MRU cache handle
 * @returns             0 on success (or if the cache was already empty). A non-zero value will be returned in
 *                      the case of errors (e.g. bad arguments)
 */
MOCKABLE_FUNCTION(, int, MRU_CACHE_prune, MRU_CACHE_HANDLE, handle);

/**
 * @brief               Removes all entries from the MRU cache
 * @param handle        The MRU cache handle
 * @returns             0 on success (or if the cache was already empty). A non-zero value will be returned in
 *                      the case of errors (e.g. bad arguments)
 */
MOCKABLE_FUNCTION(, int, MRU_CACHE_clear, MRU_CACHE_HANDLE, handle);

#ifdef __cplusplus
}
#endif

#endif  /* MRU_CACHE_H */