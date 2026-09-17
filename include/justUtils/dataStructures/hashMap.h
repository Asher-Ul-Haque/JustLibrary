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
 * @file : hashMap.h 
 * @brief : Simple Hash map in C
 */

#pragma once 

#include <justUtils/defines.h>
#include <justUtils/core/asserts.h>
#include <justUtils/memory/linearAlloc.h>
#include <stddef.h>
#include <stdint.h>

#define JUST_MAP_DEFAULT_CAPACITY  16


#ifdef __cplusplus
extern "C" {
#endif

/** @brief : Custom hash function signature (if NULL, defaults to FNV-1a)
 * @param KEY : Hash key 
 * @param KEY_SIZE : The size of the key 
 * @return : index
*/
typedef uint64_t (*justHashFunction)(const void* KEY, size_t KEY_SIZE);

/** @brief : Custom key comparison function (if NULL, defaults to memcmp)
 * @param KEY_A : One key 
 * @param KEY_B : Another key 
 * @param KEY_SIZE : The size of the key 
 * @return : comparison result of the two keys
*/
typedef int32_t (*justKeyCompareFunction)(const void* KEY_A, const void* KEY_B, size_t KEY_SIZE);

/// @brief : What can an entry be
typedef enum justHashMapEntryState 
{
  JUST_MAP_EMPTY     = 0,  ///< Empty slot
  JUST_MAP_OCCUPIED  = 1,  ///< Occupied slot
  JUST_MAP_TOMBSTONE = 2   ///< Freed slot
} justHashMapEntryState;

/// @brief : Hashmap view
typedef struct justHashMap
{
  uint8_t*                slots;          ///< Interleaved flat array of slots
  size_t                  capacity;       ///< Always a power of 2
  size_t                  mask;           ///< capacity - 1
  size_t                  count;          ///< Active key-vale pairs
  size_t                  tombstoneCount; ///< Dead slots 
  size_t                  keySize;        ///< Key size in bytes
  size_t                  valueSize;      ///< Value size in bytes
  size_t                  slotStride;     ///< Total bytes per slot (aligned)
  size_t                  keyOffset;      ///< Byte offset of key inside slot
  size_t                  valueOffset;    ///< Byte offset of value inside slot
  justHashFunction       hashFunction;
  justKeyCompareFunction compareFunction;
  JustLinearAllocator*   allocator;
  const char*             tag;            ///< Why make this hashmap
} JustHashMap;

/**
 * @brief : Initializes a Hash Map.
 * 
 * @param MAP : Pointer to HashMap struct.
 * @param KEY_SIZE : Size of key in bytes (e.g. sizeof(int) or string pointer size).
 * @param VALUE_SIZE : Size of value in bytes.
 * @param INITIAL_CAPACITY : Initial capacity (rounded up to power of 2, minimum 16).
 * @param HASHER :  Custom hash function or NULL for default FNV-1a.
 * @param COMPARATOR : Custom key compare function or NULL for default memcmp.
 * @param ALLOCATOR : Pointer to linear allocator or NULL for global memory tracker.
 * @param TAG : For memory tracker
 * @return : true if initialized successfully, false otherwise.
 */
JUST_API bool justHashmapCreate(
  JustHashMap*           MAP,
  size_t                 KEY_SIZE,
  size_t                 VALUE_SIZE,
  size_t                 INITIAL_CAPACITY,
  justHashFunction       HASHER,
  justKeyCompareFunction COMPARATOR,
  JustLinearAllocator*   ALLOCATOR,
  const char*            TAG);

/**
 * @brief : Destroys the Hash Map and frees backing buffers.
 * @param MAP : Pointer to the map to be destroyed
*/
JUST_API void justHashmapDestroy(JustHashMap* MAP);

/**
 * @brief : Inserts or updates a key-value pair.
 * 
 * @param MAP : Pointer to HashMap.
 * @param KEY_PTR : Pointer to key bytes.
 * @param VALUE_PTR : Pointer to value bytes.
 * @return : true if inserted or updated successfully, false on allocation failure.
 */
JUST_API bool justHashmapSet(JustHashMap* MAP, const void* KEY_PTR, const void* VALUE_PTR);

/**
 * @brief : Retrieves a value pointer associated with the given key.
 * 
 * @param MAP : Pointer to HashMap.
 * @param KEY_PTR : Pointer to search key.
 * @return : Pointer to value data inside the map, or NULL if key is not found.
 */
JUST_API void* justHashmapGet(const JustHashMap* MAP, const void* KEY_PTR);

/**
 * @brief : Removes a key-value pair from the map.
 * 
 * @param MAP : Pointer to HashMap.
 * @param KEY_PTR : Pointer to key bytes to remove.
 * @return : true if key was found and removed, false if not found.
 */
JUST_API bool justHashmapRemove(JustHashMap* MAP, const void* KEY_PTR);

/**
 * @brief : Clears all entries without deallocating the underlying buffer.
 * @param MAP : Pointer to the hashmap to be cleared
 */
JUST_API void justHashmapClear(JustHashMap* MAP);

/**
 * @brief : Checks if a key exists in the Hash Map.
 * @param MAP : Pointer to the hash map 
 * @param KEY_PTR : Pointer to the key to be searched 
 * @return : true if the hashmap contains the key, false otherwise
 */
JUST_API static inline bool justHashmapContains(const JustHashMap* MAP, const void* KEY_PTR)
{
  JUST_ASSERT_DEBUG_MESSAGE(MAP != NULL, "[HASH MAP] : Cannot check in a NULL MAP");
  JUST_ASSERT_DEBUG_MESSAGE(KEY_PTR != NULL, "[HASH MAP] : Cannot check a NULL KEY_PTR");

  return (justHashmapGet(MAP, KEY_PTR) != NULL);
}

/**
 * @brief : Returns total active elements stored.
 * @param MAP : Pointer to the map 
 * @return : Number of elements stored
 */
JUST_API static inline size_t justHashmapSize(const JustHashMap* MAP) 
{ 
  JUST_ASSERT_DEBUG_MESSAGE(MAP != NULL, "[HASH MAP] : Cannot check in a NULL MAP");

  return MAP->count;
}

/**
 * @brief : Tells whether the map is empty or not
 * @param MAP : Pointer to the map
 * @return : True if empty, false otherwise
*/
JUST_API static inline bool justHashmapIsEmpty(const JustHashMap* MAP)
{
  JUST_ASSERT_DEBUG_MESSAGE(MAP != NULL, "[HASH MAP] : Cannot check in a NULL MAP");

  return (MAP->count == 0);
}


// - - - Ergonomic Macros - - -

#define JUST_HASHMAP_INIT_TAGGED(MAP_PTR, KEY_TYPE, VAL_TYPE, CAP, TAG) \
  justHashmapCreate((MAP_PTR), sizeof(KEY_TYPE), sizeof(VAL_TYPE), (CAP), NULL, NULL, (NULL), TAG)

#define JUST_HASHMAP_INIT(MAP_PTR, KEY_TYPE, VAL_TYPE, CAP) JUST_HASHMAP_INIT_TAGGED(MAP_PTR, KEY_TYPE, VAL_TYPE, CAP, "HASH MAP")

#define JUST_HASHMAP_GET(MAP_PTR, VAL_TYPE, KEY_PTR) \
  ((VAL_TYPE*) justHashmapGet((MAP_PTR), (KEY_PTR)))

#ifdef __cplusplus
}
#endif

