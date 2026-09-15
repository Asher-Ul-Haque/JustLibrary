#include <justUtils/core/logger.h>
#include <justUtils/memory/linearAlloc.h>
#include <justUtils/dataStructures/queue.h>
#include <justUtils/memory/tracker.h>
#include <justUtils/core/asserts.h>
#include <stdint.h>
#include <string.h>

static inline size_t justQueueRoundToPowerOfTwo(size_t n)
{
  if (n == 0) return 0;
  if (n < JUST_QUEUE_DEFAULT_GROWTH_CAPACITY) return JUST_QUEUE_DEFAULT_GROWTH_CAPACITY;
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
#if UINTPTR_MAX > 0xFFFFFFFF
  n |= n >> 32;
#endif
  n++;
  return n;
}

JUST_API bool justQueueCreate(
  JustQueue*           QUEUE,
  size_t                INITIAL_CAPACITY,
  size_t                ELEMENT_SIZE,
  justLinearAllocator* ALLOCATOR,
  const char*           TAG)
{
  JUST_ASSERT_DEBUG_MESSAGE(QUEUE != NULL, "[QUEUE] : Cannot create a NULL queue");
  JUST_ASSERT_DEBUG_MESSAGE(ELEMENT_SIZE > 0, "[QUEUE] : Cannot create a queue with 0 ELEMENT_SIZE");

  QUEUE->head         = 0;
  QUEUE->tail         = 0;
  QUEUE->size         = 0;
  QUEUE->capacity     = justQueueRoundToPowerOfTwo(INITIAL_CAPACITY);
  QUEUE->elementSize  = ELEMENT_SIZE;
  QUEUE->mask         = QUEUE->capacity - 1;
  QUEUE->tag          = TAG,
  QUEUE->allocator    = ALLOCATOR;
  QUEUE->data         = NULL;

  if (QUEUE->capacity > 0)
  {
    size_t totalBytes = QUEUE->capacity * QUEUE->elementSize;

    if (QUEUE->allocator)
    {
      QUEUE->data = (uint8_t*)justLinearAllocAllocate(QUEUE->allocator, totalBytes, 0);
    }
    else
    {
      QUEUE->data = (uint8_t*) JUST_MALLOC_TAGGED(totalBytes, QUEUE->tag);
    }

    if (!QUEUE->data)
    {
      JUST_LOG_ERROR("[QUEUE] : Failed to allocate initial buffer for %zu slots", QUEUE->capacity);
      QUEUE->capacity = 0;
      QUEUE->mask     = 0;
      return false;
    }
  }

  return true;
}

JUST_API void justQueueDestroy(JustQueue* QUEUE)
{
  JUST_ASSERT_DEBUG_MESSAGE(QUEUE != NULL, "[QUEUE] : Cannot destroy a NULL queue");

  if (QUEUE->data && !QUEUE->allocator)
  {
    JUST_FREE(QUEUE->data);
  }

  QUEUE->data = NULL;
  QUEUE->capacity    = 0;
  QUEUE->mask        = 0;
  QUEUE->size        = 0;
  QUEUE->head        = 0;
  QUEUE->tail        = 0;
  QUEUE->elementSize = 0;
  QUEUE->allocator   = NULL;
}

JUST_API bool justQueueReserve(JustQueue* QUEUE, size_t TARGET_CAPACITY)
{
  JUST_ASSERT_DEBUG_MESSAGE(QUEUE != NULL, "[QUEUE] : Cannot reserve size in a NULL QUEUE");

  size_t newCap = justQueueRoundToPowerOfTwo(TARGET_CAPACITY);

  if (newCap < QUEUE->size)                             newCap = justQueueRoundToPowerOfTwo(QUEUE->size);
  if (newCap == QUEUE->capacity && QUEUE->data != NULL) return true;

  if (newCap == 0)
  {
    if (QUEUE->data && !QUEUE->allocator)
    {
      if (QUEUE->data && !QUEUE->allocator) JUST_FREE(QUEUE->data);
    }

    QUEUE->data     = NULL;
    QUEUE->capacity = 0;
    QUEUE->mask     = 0;
    QUEUE->head     = 0;
    QUEUE->tail     = 0;

    return true;
  }

  size_t    totalBytes  = newCap * QUEUE->elementSize;
  uint8_t*  newData     = QUEUE->allocator
                          ? (uint8_t*) justLinearAllocAllocate(QUEUE->allocator, totalBytes, 0)
                          : (uint8_t*) JUST_MALLOC_TAGGED(totalBytes, QUEUE->tag);

  if (!newData)
  {
    JUST_LOG_ERROR("[QUEUE] : Failed to allcoate buffer for capacity %zu", newCap);
    return false;
  }

  // - - - Linearize existing ring buffer into [0 ... size]
  if (QUEUE->data && QUEUE->size > 0)
  {
    size_t firstPartCount = QUEUE->capacity - QUEUE->head;
    if (QUEUE->size <= firstPartCount)
    {
      memcpy(newData, QUEUE->data + (QUEUE->head * QUEUE->elementSize), QUEUE->size * QUEUE->elementSize);
    }
    else
    {
      size_t firstPartBytes  = firstPartCount * QUEUE->elementSize;
      size_t secondPartBytes = (QUEUE->size - firstPartCount) * QUEUE->elementSize;

      memcpy(newData, QUEUE->data + (QUEUE->head * QUEUE->elementSize), firstPartBytes);
      memcpy(newData + firstPartBytes, QUEUE->data, secondPartBytes);
    }

    if (!QUEUE->allocator)
    {
      JUST_FREE(QUEUE->data);
    }
  }

  QUEUE->data     = newData;
  QUEUE->head     = 0;
  QUEUE->tail     = QUEUE->size;
  QUEUE->capacity = newCap;
  QUEUE->mask     = newCap - 1;

  return true;
}
