/*
 * Copyright (C) [2026] [Asher-Ul-Haque aka Just Somebody]
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file linearAlloc.h 
 * @brief Dead-simple fixed-capacity linear/bump allocator in C11.
 *        No reallocations, stable pointers, O(1) resets and marker rollbacks.
 */

#pragma once

#include <justUtils/defines.h>
#include <justUtils/core/asserts.h>
#include <justUtils/core/logger.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEFAULT_ALIGNMENT_BYTES  
  #define DEFAULT_ALIGNMENT_BYTES alignof(max_align_t)
#endif

/// @brief : Marker representing a rollback point in the allocator
typedef size_t justLinearMarker;

/// @brief Linear Allocator struct 
typedef struct justLinearAllocator 
{
  void*  memory;      ///< Base pointer to contiguous memory buffer
  size_t totalSize;   ///< Total buffer capacity in bytes
  size_t allocated;   ///< Current bump offset
  bool   ownsMemory;  ///< True if memory was allocated by this struct and must be freed
} justLinearAllocator;

/**
 * @brief : Initializes a fixed-capacity linear allocator.
 * @param ALLOCATOR : Pointer to LinearAllocator struct.
 * @param TOTAL_SIZE : Total capacity in bytes.
 * @param USER_BUFFER : Optional user-supplied pre-allocated buffer (or NULL to allocate from heap).
 * @param TAG : Tracking tag used when allocating heap memory (ignored if USER_BUFFER is provided).
 * @return : true if initialized successfully, false otherwise.
 */
JUST_API bool justLinearAllocCreate(
  justLinearAllocator* ALLOCATOR,
  size_t                TOTAL_SIZE,
  void*                 USER_BUFFER,
  const char*           TAG);

/**
 * @brief : deletes a linear allocator 
 * @param ALLOCATOR : a pointer to the linear allocator to destroy
*/
JUST_API void justLinearAllocDestroy(justLinearAllocator* ALLOCATOR);

/**
 * @brief : Alloactes memory from the allocator and returns it 
 * @param ALLOACTOR : a pointer to the linear allocator from where memory is to be allocated 
 * @param SIZE : how much to allocate 
 * @param ALIGNMENT : ALIGNMENT Required alignment in bytes (must be a power of two, or 0 for default). 
 * @return : a pointer to the memory if successful, NULL if fail (for example not being able to resize)
*/
JUST_API void* justLinearAllocAllocate(
  justLinearAllocator* ALLOCATOR,
  size_t                SIZE,
  size_t                ALIGNMENT);

/**
 * @brief : Resets the entire allocator back to zero. Memory is retained for reuse.
 * @param ALLOCATOR :Pointer to the allocator.
 */
JUST_API static inline void justLinearAllocReset(justLinearAllocator* ALLOCATOR)
{
  JUST_ASSERT_DEBUG_MESSAGE(ALLOCATOR != NULL, "[LINEAR ALLOCATOR] : Cannot reset a NULL ALLOCATOR");
  if (ALLOCATOR) ALLOCATOR->allocated = 0;
}

/**
 * @brief : Retrieves the current allocation marker for scoped lifetime rollbacks.
 * @param ALLOCATOR : Pointer to the allocator.
 * @return : Marker offset.
 */
JUST_API static inline justLinearMarker justLinearAllocGetMarker(const justLinearAllocator* ALLOCATOR)
{
  JUST_ASSERT_DEBUG_MESSAGE(ALLOCATOR != NULL, "[LINEAR ALLOCATOR] : Cannot get a marker from NULL ALLOCATOR");
  return ALLOCATOR->allocated;
}

/**
 * @brief : Rolls back the allocator to a previously saved marker.
 * @param ALLOCATOR : Pointer to the allocator.
 * @param MARKER : Previously captured marker.
 */
JUST_API static inline void justLinearAllocRewind(justLinearAllocator* ALLOCATOR, justLinearMarker MARKER)
{
  JUST_ASSERT_DEBUG_MESSAGE(ALLOCATOR != NULL, "[LINEAR ALLOCATOR] : Cannot rewind on a NULL MARKER");

  if (MARKER <= ALLOCATOR->allocated)
  {
    ALLOCATOR->allocated = MARKER;
  }
  else
  {
    JUST_LOG_ERROR("[LINEAR ALLOCATOR] : Cannot rewind if MARKER (%zu) is greater than what is currently allocated (%zu)", MARKER, ALLOCATOR->allocated);
  }
}

/**
 * @brief : Returns total active bytes allocated.
 * @param ALLOCATOR : Pointer to the allocator
 * @return : how many bytes are used
 */
JUST_API static inline size_t justLinearAllocGetUsed(const justLinearAllocator* ALLOCATOR)
{
  JUST_ASSERT_DEBUG_MESSAGE(ALLOCATOR != NULL, "[LINEAR ALLOCATOR] : Cannot get used on NULL ALLOCATOR");

  return ALLOCATOR->allocated;
}

/**
 * @brief : Returns remaining capacity in bytes.
 * @param ALLOCATOR : Pointer to the allocator
 * @return : How many bytes can still be allocatoed here
 */
JUST_API static inline size_t justLinearAllocGetRemaining(const justLinearAllocator* ALLOCATOR)
{
  JUST_ASSERT_DEBUG_MESSAGE(ALLOCATOR != NULL, "[LINEAR ALLOCATOR] : Cannot get remaining on NULL ALLOCATOR");

  return ALLOCATOR->totalSize - ALLOCATOR->allocated;
}
/**
 * @brief : prints debug info on the allocator in debug mode, does nothing in release mode 
 * @param ALLOCATOR : a pointer to the allocator to be visualized
*/
JUST_API void justLinearAllocDebugPrint(justLinearAllocator* ALLOCATOR);

#ifdef __cplusplus
}
#endif
