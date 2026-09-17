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
 * @file objectPool.h 
 * @brief Great memory management tool for objects of the same size
*/

#pragma once

#include <justUtils/defines.h>
#include <justUtils/dataStructures/bitset.h>
#include <justUtils/core/asserts.h>
#include <stdbool.h>
#include <stddef.h>

#define JUST_POOL_END_OF_LIST ((size_t)-1)

#ifdef __cplusplus
extern "C" {
#endif


/// @brief ObjectPool metadata
typedef struct justObjectPool
{
  size_t       stride;          ///< Actual slot stride including alignment padding
  size_t       objectSize;      ///< Raw requested object size in bytes
  size_t       capacity;        ///< Maximum objects pool can track
  size_t       freeListOffset;  ///< Byte offset to head of free list (-1 if empty)
  size_t       freeCount;       ///< Number of currently free slots
  void*        memory;          ///< Backing memory block
  bool         ownsMemory;      ///< True if pool allocated memory itself
#ifdef DEBUG
  JustBitset  allocatedBits;   ///< Tracks active allocations to trap double-returns
#endif
} JustObjectPool;

/**
 * @brief : ObjectPool creation function, the struct itself is the config, set fields before calling this function 
 * @param POOL : a pointer to the pool to be initialized, also acts as the config 
 * @param CAPACITY : how many objects should be in the pool 
 * @param OBJECT_SIZE : size of an object in buyes 
 * @param USER_MEMORY : the backing memory, pass NULL if you want the pool to handle memory itself
 * @param TAG : Tracking tag for memory tracker (ignored if userMemory is provided)
 * @return : True on success, false on fail
*/
JUST_API bool justObjectPoolCreate(
  JustObjectPool*  POOL,
  size_t            CAPACITY,
  size_t            OBJECT_SIZE,
  size_t            ALIGNMENT,
  void*             USER_MEMORY,
  const char*       TAG);

/**
 * @brief : Takes an object from object pool 
 * @param POOL : a pointer to the pool from which the object is to be taken 
 * @return : Pointer to object, or NULL if full and cannot resize
*/
JUST_API void* justObjectPoolTakeObject(JustObjectPool* POOL);

/**
 * @brief : Returns an object back to the pool for reuse 
 * @param POOL : the pool to be returned to 
 * @param OBJECT : the object to be returned
*/
JUST_API void justObjectPoolReturnObject(JustObjectPool* POOL, void* OBJECT);

/**
 * @brief : Destroys the Object Pool and frees backing memory if owned 
 * @param POOL : Pointer to the pool to be destroyed 
*/
JUST_API void justObjectPoolDestroy(JustObjectPool* POOL);

/**
 * @brief : Returns true if no objects are currently taken from the pool.
 * @param POOL : Pointer to the pool
 * @return : whether the pool is empty
 */
JUST_API static inline bool justObjectPoolIsEmpty(const JustObjectPool* POOL)
{
  JUST_ASSERT_DEBUG_MESSAGE(POOL != NULL, "[OBJECT POOL] : Cannot check if a NULL POOL is empty");
  return POOL->freeCount == POOL->capacity;
}

/**
 * @brief : Returns true if the pool is completely out of free objects.
 * @param POOL : Pointer to the pool
 * @return : whether the pool is full
 */
JUST_API static inline bool justObjectPoolIsFull(const JustObjectPool* POOL)
{
  JUST_ASSERT_DEBUG_MESSAGE(POOL != NULL, "[OBJECT POOL] : Cannot check if a NULL POOL is full");
  return POOL->freeCount == 0;
}
/**
 * @brief : debug pritns the object pool in debug mode, does nothing in release mode 
 * @param POOL : a pointer to the pool to be visualized
*/
JUST_API void justObjectPoolDebugPrint(const JustObjectPool* POOL);


#ifdef __cplusplus
}
#endif
