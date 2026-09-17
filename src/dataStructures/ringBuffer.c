#include <justUtils/dataStructures/ringBuffer.h>
#include <justUtils/memory/tracker.h>
#include <justUtils/core/asserts.h>
#include <justUtils/core/logger.h>
#include <stdint.h>
#include <stdlib.h>

static inline size_t justRingNextPowerOfTwo(size_t NUM)
{
  if (NUM <= 1) return 1;
  NUM--;
  NUM |= NUM >> 1;
  NUM |= NUM >> 2;
  NUM |= NUM >> 4;
  NUM |= NUM >> 8;
  NUM |= NUM >> 16;
#if UINTPTR_MAX > 0xFFFFFFFF
  NUM |= NUM >> 32;
#endif
  NUM++;
  return NUM;
}

JUST_API bool justRingBufferCreate(
  JustRingBuffer*      RING,
  size_t               CAPACITY,
  size_t               ELEMENT_SIZE,
  bool                 ALLOW_OVERWRITE,
  JustLinearAllocator* ALLOCATOR,
  const char*          TAG)
{
  JUST_ASSERT_DEBUG_MESSAGE(RING != NULL, "[RING BUFFER] : Target pointer cannot be NULL");
  JUST_ASSERT_DEBUG_MESSAGE(CAPACITY > 0, "[RING BUFFER] : CAPACITY must be > 0");
  JUST_ASSERT_DEBUG_MESSAGE(ELEMENT_SIZE > 0, "[RING BUFFER] : Element size must be > 0");

  RING->capacity       = justRingNextPowerOfTwo(CAPACITY);
  RING->mask           = RING->capacity - 1; 
  RING->elementSize    = ELEMENT_SIZE;
  RING->head           = 0;
  RING->tail           = 0;
  RING->count          = 0;
  RING->allowOverwrite = ALLOW_OVERWRITE;
  RING->allocator      = ALLOCATOR;
  RING->tag            = TAG;

  size_t totalBytes = RING->capacity * RING->elementSize;

  if (RING->allocator)
  {
    RING->data = (uint8_t*) justLinearAllocAllocate(RING->allocator, totalBytes, 0);
  }
  else
  {
    RING->data = (uint8_t*) JUST_MALLOC_TAGGED(totalBytes, RING->tag);
  }

  if (!RING->data)
  {
    JUST_LOG_ERROR("[RING BUFFER] : Failed to allocate %zu bytes for ring buffer", totalBytes);
    RING->capacity = 0;
    RING->mask     = 0;
    return false;
  }

  return true;
}

JUST_API void justRingBufferDestroy(JustRingBuffer* RING)
{
  JUST_ASSERT_DEBUG_MESSAGE(RING != NULL, "[RING BUFFER] : Cannot destroy a NULL RING");

  if (RING->data && !RING->allocator)
  {
    JUST_FREE(RING->data);
  }

  RING->data        = NULL;
  RING->capacity    = 0;
  RING->mask        = 0;
  RING->elementSize = 0;
  RING->head        = 0;
  RING->tail        = 0;
  RING->count       = 0;
  RING->allocator   = NULL;
}
