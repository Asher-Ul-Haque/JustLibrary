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
 * @file : ringBuffer.h 
 * @brief : Cache-friendly power-of-two circular ring buffer
 */

#pragma once 

#include <justUtils/defines.h>
#include <justUtils/core/logger.h>
#include <justUtils/core/asserts.h>
#include <justUtils/memory/linearAlloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


/// @brief : Ring Buffer structure
typedef struct justRingBuffer 
{
  uint8_t*              data;           ///< Contiguous ring memory
  size_t                capacity;       ///< Total element capacity (power of 2)
  size_t                mask;           ///< capacity - 1 for fast bitwise modulo
  size_t                elementSize;    ///< Size of each element in bytes
  size_t                head;           ///< Next write index
  size_t                tail;           ///< Next read index
  size_t                count;          ///< Active item count
  bool                  allowOverwrite; ///< Overwrite oldest item when full
  const char*           tag;            ///< Memory tracking tag
  justLinearAllocator* allocator;      ///< Optional linear allocator (NULL for heap)
} JustRingBuffer;

/**
 * @brief : Initializes a fixed-capacity Ring Buffer.
 * @param RING : Pointer to RingBuffer struct.
 * @param CAPACITY : Maximum element count (rounded up to nearest power of 2).
 * @param ELEMENT_SIZE : Size of an individual element in bytes.
 * @param ALLOW_OVERWRITE : If true, pushing to a full buffer drops the oldest item.
 * @param ALLOCATOR : Optional linear allocator, or NULL for managed heap.
 * @param TAG : Tracking tag for memory tracker.
 * @return : true on success, false on allocation failure.
 */
JUST_API bool justRingBufferCreate(
  JustRingBuffer*      RING,
  size_t               CAPACITY,
  size_t               ELEMENT_SIZE,
  bool                 ALLOW_OVERWRITE,
  justLinearAllocator* ALLOCATOR,
  const char*          TAG);

JUST_API void justRingBufferDestroy(JustRingBuffer* RING);


/**
 * @brief : Reserves a slot at the head for writing and returns its pointer.
 * @param RING : Point to the ring buffer
 * @warning : This does not initialize the element
 * @return : pointer to the uniitialized element
 */
JUST_API static inline void* justRingBufferEmplace(JustRingBuffer* RING)
{
  JUST_ASSERT_DEBUG_MESSAGE(RING != NULL, "[RING BUFFER] : Cannot emplace in a NULL RING buffer");

  if (RING->count == RING->capacity)
  {
    if (!RING->allowOverwrite)
    {
      return NULL;
    }

    // - - - Overwrite oldest item: advance tail
    RING->tail = (RING->tail + 1) & RING->mask;
    RING->count--;
  }

  void* slot = RING->data + (RING->head * RING->elementSize);
  RING->head = (RING->head + 1) & RING->mask;
  RING->count++;

  return slot;
}

/**
 * @brief : Writes/pushes an element into the ring buffer.
 * @param RING : Pointer to the ring buffer
 * @param ITEM_PTR : Pointer to the item to be pushed
 * @return : true if success, false otherwise
 */
JUST_API static inline bool justRingBufferPush(JustRingBuffer* RING, const void* ITEM_PTR)
{
  JUST_ASSERT_DEBUG_MESSAGE(RING != NULL, "[RING BUFFER] : Cannot push to a NULL RING");
  JUST_ASSERT_DEBUG_MESSAGE(ITEM_PTR != NULL, "[RING BUFFER] : Cannot push a NULL ITEM_PTR");

  void* slot = justRingBufferEmplace(RING);
  if (!slot) return false;

  memcpy(slot, ITEM_PTR, RING->elementSize);
  return true;
}

/**
 * @brief : Reads/pops an element from the ring buffer.
 * @param RING : Pointer to the ring buffer
 * @param OUT_ITEM_PTR : Optional pointer to store the popped element
 * @return : true if success, false otherwise
 */
JUST_API static inline bool justRingBufferPop(JustRingBuffer* RING, void* OUT_ITEM_PTR)
{
  JUST_ASSERT_DEBUG_MESSAGE(RING != NULL, "[RING BUFFER] : Cannot pop from a NULL RING");

  if (RING->count == 0) return false;

  if (OUT_ITEM_PTR)
  {
    const void* source = RING->data + (RING->tail * RING->elementSize);
    memcpy(OUT_ITEM_PTR, source, RING->elementSize);
  }

  RING->tail = (RING->tail + 1) & RING->mask;
  RING->count--;

  return true;
}

/**
 * @brief : Peeks at the oldest element without removing it. Returns NULL if empty.
 * @param RING : Pointer to the ring buffer
 * @return : Pointer to the oldest element
 */
JUST_API static inline void* justRingBufferPeek(const JustRingBuffer* RING)
{
  JUST_ASSERT_DEBUG_MESSAGE(RING != NULL, "[RING BUFFER] : Cannot peek in a NULL RING");

  if (RING->count == 0) return NULL;
  return (void*)(RING->data + (RING->tail * RING->elementSize));
}

/**
 * @brief : Clears the entire buffer
 * @param RING : Pointer to the ring buffer
*/
JUST_API static inline void justRingBufferClear(JustRingBuffer* RING)
{
  JUST_ASSERT_DEBUG_MESSAGE(RING != NULL, "[RING BUFFER] : Cannot clear a NULL RING");

  RING->head  = 0;
  RING->tail  = 0;
  RING->count = 0;
}

/**
 * @brief : Tells whether the buffer is empty
 * @param RING : Pointer to the ring
 * @return : true if the buffer is empty
*/
JUST_API static inline bool justRingBufferIsEmpty(const JustRingBuffer* RING)
{
  JUST_ASSERT_DEBUG_MESSAGE(RING != NULL, "[RING BUFFER] : Cannot check if a NULL RING is empty");
  return (RING->count == 0); 
}

/**
 * @brief : Tells whether the buffer is full
 * @param RING : Pointer to the ring
 * @return : true if the buffer is full
*/
JUST_API static inline bool justRingBufferIsFull(const JustRingBuffer* RING)
{
  JUST_ASSERT_DEBUG_MESSAGE(RING != NULL, "[RING BUFFER] : Cannot check if a NULL RING is full");

  return (RING->count == RING->capacity); 
}

/**
 * @brief : Tells the size of a buffer
 * @param RING : POinter to the ring buffer
 * @return : The size of the buffer in elements 
*/
JUST_API static inline size_t justRingBufferSize(const JustRingBuffer* RING) 
{
  JUST_ASSERT_DEBUG_MESSAGE(RING != NULL, "[RING BUFFER] : Cannot check size of a RING");

  return RING->count; 
}

/**
 * @brief : Tells the capacity of a buffer
 * @param RING : Pointer to the ring buffer
 * @return : The capacity of the buffer in elements
*/
JUST_API static inline size_t justRingBufferCapacity(const JustRingBuffer* RING) 
{
  JUST_ASSERT_DEBUG_MESSAGE(RING != NULL, "[RING BUFFER] : Cannot check capacity of a RING");

  return RING->capacity; 
}


// - - - Ergonomic Macros - - -

#define JUST_RING_INIT(RING_PTR, CAPACITY, TYPE, ALLOW_OVERWRITE) \
  justRingBufferCreate((RING_PTR), (CAPACITY), sizeof(TYPE), (ALLOW_OVERWRITE), NULL, "RING BUFFER")

#define JUST_RING_INIT_TAGGED(RING_PTR, CAPACITY, TYPE, ALLOW_OVERWRITE, TAG) \
  justRingBufferCreate((RING_PTR), (CAPACITY), sizeof(TYPE), (ALLOW_OVERWRITE), NULL, (TAG))

#define JUST_RING_EMPLACE(RING_PTR, TYPE) \
  ((TYPE*)justRingBufferEmplace(RING_PTR))

#define JUST_RING_PEEK(RING_PTR, TYPE) \
  ((TYPE*)justRingBufferPeek(RING_PTR))

#define JUST_RING_PUSH_VAL(RING_PTR, TYPE, VALUE)         \
  do                                                      \
  {                                                       \
    TYPE  _val  = (VALUE);                                \
    TYPE* _slot = (TYPE*)justRingBufferEmplace(RING_PTR); \
    if (_slot) *_slot = _val;                             \
  } while(0)

#ifdef __cplusplus
}
#endif
