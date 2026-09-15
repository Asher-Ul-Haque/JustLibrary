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
 * @file : queue.h 
 * @brief : Queue implementation using dynamic Array 
 */

#pragma once 
#include <justUtils/defines.h>
#include <justUtils/core/asserts.h>
#include <justUtils/memory/linearAlloc.h>
#include <justUtils/dataStructures/dynamicArray.h>
#include <stdint.h>

#define JUST_QUEUE_DEFAULT_GROWTH_CAPACITY 8

#ifdef __cplusplus
extern "C" {
#endif



/// @brief : The queue struct
typedef struct justQueue
{
  uint8_t*              data;         ///< Contigous element storage
  size_t                capacity;     ///< Allocated capacity (always a power of 2)
  size_t                mask;         ///< capacity - 1 for bitwise modulo
  size_t                size;         ///< Current number of active elements
  size_t                head;         ///< Index of oldest element
  size_t                tail;         ///< Next write index
  size_t                elementSize;  ///< sizeof(T)
  const char*           tag;          ///< WHy create this queue
  justLinearAllocator*  allocator;    ///< Optional linear allocator, NULL for system heap
} JustQueue;

/**
 * @brief : Creates a queue instance.
 * @param QUEUE : A pointer to the Queue to be created 
 * @param INITIAL_CAPACITY : How many elements at the start
 * @param ELEMENT_SIZE : What is the size of an element 
 * @param ALLOCATOR : Optional Linear allocator
 * @param TAG : Optional why create this queue
 * @return : True if successful, False if not
 */
JUST_API bool justQueueCreate(
  JustQueue*            QUEUE, 
  size_t                INITIAL_CAPACITY,
  size_t                ELEMENT_SIZE,
  justLinearAllocator*  ALLOCATOR,
  const char*           TAG);

/**
 * @brief : Destroys the queue.
 * @param QUEUE : Pointer to the queue to be destroyed
*/
JUST_API void justQueueDestroy(JustQueue* QUEUE);

/**
 * @brief : Sets queue size, growth and shrink both
 * @param QUEUE : The queue to be grown or shrunk
 * @param TARGET_CAPACITY : The new size of the queue
 * @return : True if succesful and false if not
 */
JUST_API bool justQueueReserve(JustQueue* QUEUE, size_t TARGET_CAPACITY);

/**
 * @brief : Reserves a slot at the tail and returns a pointer for direct
 * @param QUEUE : The queue to be emplaced
 * @return : Pointer to the unitialized element 
 * @warning : The element is not initialized, use the pointer to initialize it
 */
JUST_API static inline void* justQueueEmplace(JustQueue* QUEUE)
{
  JUST_ASSERT_DEBUG_MESSAGE(QUEUE != NULL, "[QUEUE] : Cannot emplace in a NULL QUEUE");

  if (QUEUE->size >= QUEUE->capacity)
  {
    size_t newCap = QUEUE->capacity ? (QUEUE->capacity * 2) : JUST_QUEUE_DEFAULT_GROWTH_CAPACITY;
    if (!justQueueReserve(QUEUE, newCap)) return NULL;
  }

  void* slot  = (uint8_t*) QUEUE->data + (QUEUE->tail * QUEUE->elementSize);
  QUEUE->tail = (QUEUE->tail + 1) & QUEUE->mask;
  QUEUE->size++;
  return slot;
}

/**
 * @brief : Enqueues an item to the back of the queue (O(1)).
 * @param QUEUE : A pointer to the queue 
 * @param VALUE_PTR : Pointer to the value to be enqueued 
 * @return : True if successful, False if not
 */
JUST_API static inline bool justQueueEnqueue(JustQueue* QUEUE, const void* VALUE_PTR)
{
  JUST_ASSERT_DEBUG_MESSAGE(QUEUE != NULL, "[QUEUE] : Cannot enqueue to a NULL QUEUE");
  JUST_ASSERT_DEBUG_MESSAGE(VALUE_PTR != NULL, "[QUEUE] : Cannot enqueue a NULL VALUE_PTR");

  void* slot = justQueueEmplace(QUEUE);
  if (!slot) return false;

  memcpy(slot, VALUE_PTR, QUEUE->elementSize);
  return true;
}

/**
 * @brief : Dequeues an item from the front of the queue in O(1) time.
 * @param QUEUE : A pointer to the queue 
 * @param OUT_VALUE_PTR : Optional pointer to store the dequeued value 
 * @return : True if successful, False if not
 */
JUST_API static inline bool justQueueDequeue(JustQueue* QUEUE, void* OUT_VALUE_PTR)
{
  JUST_ASSERT_DEBUG_MESSAGE(QUEUE != NULL, "[QUEUE] : Cannot dequeue from a NULL QUEUE");

  if (QUEUE->size == 0) return false;

  if (OUT_VALUE_PTR)
  {
    void* slot = (uint8_t*)QUEUE->data + (QUEUE->head * QUEUE->elementSize);
    memcpy(OUT_VALUE_PTR, slot, QUEUE->elementSize);
  }

  QUEUE->head = (QUEUE->head + 1) & QUEUE->mask;
  QUEUE->size--;

  return true;
}

/**
 * @brief : Returns pointer to item at the front without dequeuing.
 * @param QUEUE : The queue to peek from 
 * @return : Pointer to the head variable of the queue
 */
JUST_API static inline void* justQueuePeek(const JustQueue* QUEUE)
{
  JUST_ASSERT_DEBUG_MESSAGE(QUEUE != NULL, "[QUEUE] : Cannot peek into a NULL QUEUE");

  if (QUEUE->size == 0) return NULL;
  return (void*) ((uint8_t*)QUEUE->data + (QUEUE->head * QUEUE->elementSize));
}

/**
 * @brief : Returns active item count in queue.
 * @param QUEUE : Pointer to the queue whose size is to be measured 
 * @return : The size of the queue
 */
JUST_API static inline size_t justQueueSize(const JustQueue* QUEUE)
{
  JUST_ASSERT_DEBUG_MESSAGE(QUEUE != NULL, "[QUEUE] : Cannot check size of a NULL QUEUE");

  return QUEUE->size;
}

/**
 * @brief : Returns capacity of queue.
 * @param QUEUE : Pointer to the queu
 * @return : The capacity of the queue
 */
JUST_API static inline size_t justQueueCapacity(const JustQueue* QUEUE)
{
  JUST_ASSERT_DEBUG_MESSAGE(QUEUE != NULL, "[QUEUE] : Cannot check capacity of a NULL QUEUE");

  return QUEUE->capacity;
}

/**
 * @brief : Returns whether the queue is emptu
 * @param QUEUE : Pointer to the queu
 * @return : true if the queue is empty, false otherwise
 */
JUST_API static inline size_t justQueueIsEmpty(const JustQueue* QUEUE)
{
  JUST_ASSERT_DEBUG_MESSAGE(QUEUE != NULL, "[QUEUE] : Cannot check if a NULL QUEUE is empty");

  return (QUEUE->size == 0);
}

/**
 * @brief : Clears a queue 
 * @param QUEUE : The queue to be cleared
*/
JUST_API static inline void justQueueClear(JustQueue* QUEUE)
{
  JUST_ASSERT_DEBUG_MESSAGE(QUEUE != NULL, "[QUEUE] : Cannot clear a NULL QUEUE");

  QUEUE->size = 0;
  QUEUE->head = 0;
  QUEUE->tail = 0;
}

/** 
 * @brief : Trims capacity to the smallest power of two that fits current size 
 * @param QUEUE : The queue to be shrunk
 * @return : True if shrunk, false otherwise
*/
JUST_API static inline bool justQueueShrinkToFit(JustQueue* QUEUE)
{
  JUST_ASSERT_DEBUG_MESSAGE(QUEUE != NULL, "[QUEUE] : Cannot shrink a NULL QUEUE");

  return justQueueReserve(QUEUE, QUEUE->size);
}


// - - - Ergonomic Macros - - -

#define JUST_QUEUE_INIT_TAGGED(QUEUE_PTR, CAPACITY, TYPE, TAG) \
  justQueueCreate((QUEUE_PTR), (CAPACITY), sizeof(TYPE), NULL, (TAG))

#define JUST_QUEUE_INIT(QUEUE_PTR, CAPACITY, TYPE) JUST_QUEUE_INIT_TAGGED(QUEUE_PTR, CAPACITY, TYPE, "QUEUE")

#define JUST_QUEUE_EMPLACE(QUEUE_PTR, TYPE) \
  ((TYPE*) justQueueEmplace(QUEUE_PTR))

#define JUST_QUEUE_PEEK(QUEUE_PTR, TYPE) \
  ((TYPE*) justQueuePeek(QUEUE_PTR))

#define JUST_QUEUE_ENQUEUE_VAL(QUEUE_PTR, TYPE, VALUE)  \
  do                                                    \
  {                                                     \
    TYPE  _val  = (TYPE) VALUE;                         \
    TYPE* _slot = JUST_QUEUE_EMPLACE(QUEUE_PTR, TYPE);  \
    if (_slot) *_slot = _val;                           \
  } while(0)

#ifdef __cplusplus
}
#endif
