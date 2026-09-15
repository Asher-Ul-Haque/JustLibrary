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
 * @file : tracker.h 
 * @brief : Overwrites of malloc, realloc and free to have memory tracking in debug mode, with tagging and allocation limits
*/

#pragma once 

#include <justUtils/defines.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef JUST_MEMORY_TAG_LIMIT
  #define JUST_MEMORY_TAG_LIMIT 64
#endif

#ifndef JUST_MEMORY_ALLOC_DEFAULT_LIMIT
  #define JUST_MEMORY_ALLOC_DEFAULT_LIMIT 1024ULL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief : Allocates memory with surrounding magic canary guards and tracking metadata.
 * @param SIZE : how much to allocate 
 * @param FILE : which file is it allocated in 
 * @param FUNCTION : Which function is it allocated in
 * @param LINE : Which line is it allocated in
 * @param TAG : Why are you allocating
 */
void* justTrackedMalloc(
  size_t      SIZE,
  const char* FILE,
  const char* FUNC,
  int32_t     LINE,
  const char* TAG);

/**
 * @brief : Reallocates memory, updating canary guards and size tracking.
 * @param PTR : Ptr to realloc 
 * @param NEW_SIZE : new allocation size
 * @param FILE : What file is the reallocate in 
 * @param FUNCTION : What function is the reallocate in 
 * @param LINE : What line is the reallocate in
 * @warning : Make sure pointer is not null, realloc != malloc
*/
JUST_API void* justTrackedRealloc(
  void*       PTR,
  size_t      NEW_SIZE,
  const char* FILE,
  const char* FUNCTION,
  int32_t     LINE);

/**
 * @brief : Allocates zero-initialized memory with canary guards and tracking metadata.
 * @param COUNT : How many objects 
 * @param SIZE : Size of one object 
 * @param FILE : What file is the callocate in 
 * @param FUNCTION : What function is the callocate in 
 * @param LINE : What line is the callocate in
 * @param TAG : Why are you allocating
 */
JUST_API void* justTrackedCalloc(
  size_t      COUNT,
  size_t      SIZE,
  const char* FILE,
  const char* FUNC,
  int32_t     LINE,
  const char* TAG);

/**
 * @brief : Frees memory and verifies Canary safety bounds.
 * @param PTR : What ptr to free 
 * @param FILE : What file is free called in 
 * @param FUNCTION : What function is the free in 
 * @param LINE : What line is the free in
 */
JUST_API void justTrackedFree(
  void*       PTR,
  const char* FILE,
  const char* FUNCTION,
  int32_t     LINE);

/**
 * @brief : Validates all active allocations against canary corruption.
 * @return : true if all allocations are intact, false if corruption detected.
 */
JUST_API bool justMemoryCheckBounds(void);

/// @brief Reports all active allocations that haven't been freed (Memory Leaks).
JUST_API void justMemoryReportLeaks(void);

/**
 * @brief : Returns total active allocated bytes currently in use.
 * @param TAG : The tag for which you want to check memory, set it to MEMORY_TAG_COUNT to get all
 * @return : total active allocated bytes in use
 */
JUST_API size_t memoryGetActiveBytes(const char* TAG);

/**
 * @brief : Returns the memory allocated as a string,
 * @param VERBOSE : If true, then every information is printed, where did each allocation happen, line, file and function
 * @warning : just for debugging, not for actually parsing memory usage
 * @see : memoryGetActiveBytes for better usage API
*/
JUST_API void memoryLogUsageStr(bool VERBOSE);

/**
 * @brief : Sets a limit on the memory allocation of a particular type
 * @warning : If you exceed the limit, the allocations will return NULL
 * @warning : TAG must be valid
 * @param LIMIT : The limit you want to set in bytes
 * @param TAG : What do you want to set the limit for
*/
JUST_API void memorySetLimit(size_t LIMIT, const char* TAG);

/**
 * @brief : Returns the memory allocation limit
 * @warning : TAG must be valid
*/
JUST_API size_t memoryGetLimit(const char* TAG);

// - - - Optional Macro Overrides for Debug Mode
#ifdef DEBUG
  #define JUST_MALLOC_TAGGED(size, tag)        justTrackedMalloc((size), __FILE__, __func__, __LINE__, (tag))
  #define JUST_CALLOC_TAGGED(count, size, tag) justTrackedCalloc((count), (size), __FILE__, __func__, __LINE__, (tag))
  #define JUST_MALLOC(size)                    JUST_MALLOC_TAGGED(size, "UNKNOWN")
  #define JUST_CALLOC(count, size)             JUST_CALLOC_TAGGED(count, size, "UNKNOWN")
  #define JUST_REALLOC(ptr, size)              justTrackedRealloc((ptr), (size), __FILE__, __func__, __LINE__)
  #define JUST_FREE(ptr)                       justTrackedFree((ptr), __FILE__, __func__, __LINE__)
#else
  #define JUST_MALLOC_TAGGED(size, tag)        malloc((size))
  #define JUST_CALLOC_TAGGED(count, size, tag) calloc((count), (size))
  #define JUST_MALLOC(size, tag)               malloc((size))
  #define JUST_CALLOC(count, size, tag)        calloc((count), (size))
  #define JUST_REALLOC(ptr, size)              realloc((ptr), (size))
  #define JUST_FREE(ptr)                       free((ptr))
#endif

#ifdef __cplusplus
}
#endif
