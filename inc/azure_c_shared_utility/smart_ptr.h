// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SMART_PTR_H
#define SMART_PTR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "azure_c_shared_utility/umock_c_prod.h"

/**
 * @brief A handle to a smart pointer. This provides ref counted to allocated memory, and will free
 *        the memory when the last reference is gone
 */
typedef struct SMART_PTR_TAG* SMART_PTR_HANDLE;

/**
* @brief                Function passed to SMART_PTR_create that is called whenever the last reference
*                       to the item is removed to free up memory. This must handle being called with NULL
* @param data           Pointer to the data being freed
*/
typedef void (*SMART_PTR_FREE_FUNC)(void* data);

/**
 * @brief               Creates a new smart pointer
 * @returns             A SMART_PTR_HANDLE if no failures occur, or NULL otherwise
 */
MOCKABLE_FUNCTION(, SMART_PTR_HANDLE, SMART_PTR_new);

/**
 * @brief               Creates a new smart pointer
 * @param ptr           The pointer to wrap
 * @param freeFunc      The function called to free the data once the last reference is removed. Cannot be
 *                      NULL
 * @returns             A SMART_PTR_HANDLE if no failures occur, or NULL otherwise
 */
MOCKABLE_FUNCTION(, SMART_PTR_HANDLE, SMART_PTR_create, void*, ptr, SMART_PTR_FREE_FUNC, freeFunc);

/**
 * @brief               Creates a copy of the existing smart pointer, incrementing the reference count
 * @param handle        The smart pointer handle to copy
 * @returns             A SMART_PTR_HANDLE if no failures occur, or NULL otherwise
 */
MOCKABLE_FUNCTION(, SMART_PTR_HANDLE, SMART_PTR_copy, SMART_PTR_HANDLE, handle);

/**
 * @brief               Deletes a smart pointer, decrementing the reference count
 * @param handle        The smart pointer handle to delete
 * @returns             A SMART_PTR_HANDLE if no failures occur, or NULL otherwise
 */
MOCKABLE_FUNCTION(, void, SMART_PTR_delete, SMART_PTR_HANDLE, handle);

/**
 * @brief               Deletes a smart pointer, decrementing the reference count
 * @param handle        The smart pointer handle
 * @returns             The stored raw pointer, or NULL on failure
 */
MOCKABLE_FUNCTION(, void*, SMART_PTR_get, SMART_PTR_HANDLE, handle);

/**
 * @brief               Checks if the smart pointer points to a NULL value
 * @param handle        The smart pointer handle
 * @returns             0 if the smart pointer wraps a NULL value, non-zero otherwise
 */
MOCKABLE_FUNCTION(, int, SMART_PTR_has_value, SMART_PTR_HANDLE, handle);


#ifdef __cplusplus
}
#endif

#endif  /* SMART_PTR_H */
