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
 * @file : justBitset.h
 * @brief : Lightweight dynamic justBitset
*/

#pragma once

#include <justUtils/defines.h>
#include <justUtils/core/asserts.h>
#include <memory.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
  extern "C" {
#endif

#define JUST_BITS_PER_WORD (sizeof(uint64_t) * 8)

/// @brief : justBitset struct
typedef struct justBitset
{
  size_t    capacity;   ///< Total trackable entity count
  size_t    wordCount;  ///< Number of 64-bit backing words
  uint64_t* words;      ///< Backing memory buffer
  bool      ownsMemory; ///< True if allocated internally, false if passed in
} justBitset;


// - - - Internal Tail Helper
JUST_API static inline uint64_t justBitsetGetTailMask(size_t CAPACITY)
{
  size_t rem = CAPACITY % JUST_BITS_PER_WORD;
  return (rem == 0) ? ~(uint64_t)0 : (((uint64_t)1 << rem) - 1);
}

/**
 * @brief : Initializes a justBitset
 * @param SET : Pointer to the justBitset struct to initialize.
 * @param CAPACITY : Number of bits / entities to track
 * @param USER BUFFER : Optional pre-allocaed memory buffer or NULL to allocate
 * @param TAG : Tracking tag used if allocated intenrally
 * @warning : MEMORY must be aligned to sizeof(size_t)
 * @return : true on success, false on failure
*/
JUST_API bool justBitsetCreate(
  justBitset*   SET,
  size_t        CAPACITY,
  void*         USER_BUFFER,
  const char*   TAG);

/**
 * @brief : Destroys the justBitset and frees backing memory if owned
 * @param SET: Pointer to the justBitset to destroy
*/
JUST_API void justBitsetDestroy(justBitset* SET);

/**
 * @brief : Sets a bit at index to 1
 * @param SET : Pointer to the justBitset
 * @param INDEX : Which index to set
*/
JUST_API static inline void justBitsetSet(justBitset* SET, size_t INDEX)
{
  JUST_ASSERT_DEBUG_MESSAGE(SET != NULL, "[BITSET] : Cannot set a bit in a NULL SET");
  JUST_ASSERT_DEBUG_MESSAGE(INDEX < SET->capacity, "[BITSET] : Index out of bounds");

  SET->words[INDEX / JUST_BITS_PER_WORD] |= ((uint64_t)1 << (INDEX % JUST_BITS_PER_WORD));
}

/**
 * @brief : Sets a bit at index to 0
 * @param SET : Pointer to the justBitset
 * @param INDEX : Which index to clear
*/
JUST_API static inline void justBitsetClear(justBitset* SET, size_t INDEX)
{
  JUST_ASSERT_DEBUG_MESSAGE(SET != NULL, "[BITSET] : Cannot set a bit in a NULL SET");
  JUST_ASSERT_DEBUG_MESSAGE(INDEX < SET->capacity, "[BITSET] : Index out of bounds");

  SET->words[INDEX / JUST_BITS_PER_WORD] &= ~((uint64_t)1 << (INDEX % JUST_BITS_PER_WORD));
}

/**
 * @brief : Toggles a bit at index
 * @param SET : Pointer to the justBitset
 * @param INDEX : Which index to toggle
*/
JUST_API static inline void justBitsetToggle(justBitset* SET, size_t INDEX)
{
  JUST_ASSERT_DEBUG_MESSAGE(SET != NULL, "[BITSET] : Cannot toggle a bit in a NULL SET");
  JUST_ASSERT_DEBUG_MESSAGE(INDEX < SET->capacity, "[BITSET] : Index out of bounds");

  SET->words[INDEX / JUST_BITS_PER_WORD] ^= ((uint64_t)1 << (INDEX % JUST_BITS_PER_WORD));
}

/**
 * @brief : Gets a bit at index
 * @param SET : Pointer to the justBitset
 * @param INDEX : Which index to set
 * @return : True if the bit is set, false otherwise
*/
JUST_API static inline bool justBitsetGet(justBitset* SET, size_t INDEX)
{
  JUST_ASSERT_DEBUG_MESSAGE(SET != NULL, "[BITSET] : Cannot get a bit in a NULL SET");
  JUST_ASSERT_DEBUG_MESSAGE(INDEX < SET->capacity, "[BITSET] : Index out of bounds");

  return (SET->words[INDEX / JUST_BITS_PER_WORD] & ((uint64_t)1 << (INDEX % JUST_BITS_PER_WORD))) != 0;
}

/**
 * @brief : Clears all bits to 0
 * @param SET : Pointer to the justBitset
*/
JUST_API static inline void justBitsetClearAll(justBitset* SET)
{
  JUST_ASSERT_DEBUG_MESSAGE(SET != NULL, "[BITSET] : Cannot clear all in a NULL SET");

  memset(SET->words, 0, SET->wordCount * sizeof(uint64_t));
}

/**
 * @brief : Sets all bits to 1
 * @param SET : Pointer to the justBitset
*/
JUST_API static inline void justBitsetSetAll(justBitset* SET)
{
  JUST_ASSERT_DEBUG_MESSAGE(SET != NULL, "[BITSET] : Cannot set all in a NULL SET");

  memset(SET->words, 0xFF, SET->wordCount * sizeof(uint64_t));
  if (SET->wordCount > 0)
  {
    SET->words[SET->wordCount - 1] &= justBitsetGetTailMask(SET->capacity);
  }
}

/**
 * @brief : Check capacity of a bitset
 * @param SET : Pointer to the justBitset
*/
JUST_API static inline size_t justBitsetCapacity(const justBitset* SET)
{
  JUST_ASSERT_DEBUG_MESSAGE(SET != NULL, "[BITSET] : Cannot check capacity of a NULL SET");

  return SET->capacity;
}

/**
 * @brief : Computes in -place union : DST = DST | SRC
 * @warning : The smaller capacity is used
 * @warning : There should be no memory overlap
 * @param DST : The destination justBitset
 * @param SRC : The source justBitset
*/
JUST_API void justBitsetUnion(justBitset* DST, const justBitset* SRC);

/**
 * @brief : Computes in -place intersection : DST = DST & SRC
 * @warning : The smaller capacity is used
 * @warning : There should be no memory overlap
 * @param DST : The destination justBitset
 * @param SRC : The source justBitset
*/
JUST_API void justBitsetIntersection(justBitset* DST, const justBitset* SRC);

/**
 * @brief : Computes in -place difference : DST = DST & ~SRC
 * @warning : The smaller capacity is used
 * @warning : There should be no memory overlap
 * @param DST : The destination justBitset
 * @param SRC : The source justBitset
*/
JUST_API void justBitsetDifference(justBitset* DST, const justBitset* SRC);

/**
 * @brief : Compares two justBitsets for equality
 * @param A : One justBitset
 * @param B : Second justBitset
 * @return : True if identical in capacity and set bits, false otherwise
 */
JUST_API bool justBitsetEquals(const justBitset* A, const justBitset* B);

#ifdef __cplusplus
}
#endif
