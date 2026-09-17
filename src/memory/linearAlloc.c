#include <justUtils/memory/linearAlloc.h>
#include <justUtils/memory/tracker.h>
#include <justUtils/core/asserts.h>
#include <justUtils/core/logger.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>

static inline bool justlinearAllocIsPowerOfTwo(size_t X)
{
  return (X != 0) && ((X & (X - 1)) == 0);
}

bool justLinearAllocCreate(
  JustLinearAllocator*  ALLOCATOR,
  size_t                TOTAL_SIZE,
  void*                 USER_BUFFER,
  const char*           TAG)
{
  JUST_ASSERT_DEBUG_MESSAGE(ALLOCATOR != NULL, "[LINEAR ALLOC] : Target ALLOCATOR pointer cannot be NULL");
  JUST_ASSERT_DEBUG_MESSAGE(TOTAL_SIZE > 0,    "[LINEAR ALLOC] : TOTAL_SIZE must be greater than 0");

  ALLOCATOR->totalSize  = TOTAL_SIZE;
  ALLOCATOR->allocated  = 0;
  ALLOCATOR->ownsMemory = (USER_BUFFER == NULL);

  if (ALLOCATOR->ownsMemory)
  {
    const char* allocTag = TAG;
    ALLOCATOR->memory = JUST_MALLOC_TAGGED(TOTAL_SIZE, allocTag);

    if (!ALLOCATOR->memory)
    {
      JUST_LOG_FATAL("[LINEAR ALLOC] : Failed to allocate %zu bytes for backing buffer", TOTAL_SIZE);
      return false;
    }
  }
  else
  {
    // - - - Verify user buffer satisfies max_align_t alignment
    JUST_ASSERT_DEBUG_MESSAGE(
      ((uintptr_t)USER_BUFFER % alignof(max_align_t)) == 0,
      "[LINEAR ALLOC] : Supplied USER_BUFFER must be aligned to max_align_t");

    ALLOCATOR->memory = USER_BUFFER;
  }

  return true;
}

void justLinearAllocDestroy(JustLinearAllocator* ALLOCATOR)
{
  if (!ALLOCATOR) return;

  if (ALLOCATOR->ownsMemory && ALLOCATOR->memory)
  {
    JUST_FREE(ALLOCATOR->memory);
  }

  ALLOCATOR->memory     = NULL;
  ALLOCATOR->totalSize  = 0;
  ALLOCATOR->allocated  = 0;
  ALLOCATOR->ownsMemory = false;
}

void* justLinearAllocAllocate(
  JustLinearAllocator*  ALLOCATOR, 
  size_t                SIZE, 
  size_t                ALIGNMENT)
{
  JUST_ASSERT_DEBUG(ALLOCATOR != NULL);
  if (SIZE == 0) return NULL;

  if (ALIGNMENT == 0)
  {
    ALIGNMENT = DEFAULT_ALIGNMENT_BYTES;
  }

  JUST_ASSERT_DEBUG_MESSAGE(justlinearAllocIsPowerOfTwo(ALIGNMENT), "[LINEAR ALLOC] : ALIGNMENT must be a power of two");

  // - - - Absolute pointer alignment
  uintptr_t currentPtr  = (uintptr_t)ALLOCATOR->memory + ALLOCATOR->allocated;
  uintptr_t alignedPtr  = (currentPtr + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
  size_t    padding     = alignedPtr - currentPtr;

  if (ALLOCATOR->allocated + padding + SIZE > ALLOCATOR->totalSize)
  {
    JUST_LOG_ERROR("[LINEAR ALLOC] : Out of memory! Requested %zu bytes (pad %zu), but only %zu bytes remaining",
                    SIZE, padding, ALLOCATOR->totalSize - ALLOCATOR->allocated);
    return NULL;
  }

  ALLOCATOR->allocated += padding + SIZE;
  return (void*)alignedPtr;
}

void justLinearAllocDebugPrint(JustLinearAllocator* ALLOCATOR)
{
  #ifdef DEBUG 
    #include <memory.h>
    JUST_ASSERT_DEBUG_MESSAGE(ALLOCATOR != NULL, "[LINEAR ALLOCATOR] : Cannot debug print a NULL ALLOCATOR");

    size_t BAR_WIDTH = 50;

    size_t used = ALLOCATOR->allocated;
    if (used > ALLOCATOR->totalSize) used = ALLOCATOR->totalSize;

    size_t freeBytes = ALLOCATOR->totalSize - used;

    double usedPercent = 0.0;
    double freePercent = 0.0;

    if (ALLOCATOR->totalSize > 0)
    {
      usedPercent = 100.0 * (double)used / (double)ALLOCATOR->totalSize;
      freePercent = 100.0 - usedPercent;
    }

    size_t filled = (ALLOCATOR->totalSize == 0)
      ? 0
      : (used * BAR_WIDTH) / ALLOCATOR->totalSize;

    char bar[BAR_WIDTH + 1];

    memset(bar, '_', BAR_WIDTH);
    memset(bar, '*', filled);

    if (filled < BAR_WIDTH) bar[filled] = '@';

    bar[(int)BAR_WIDTH] = '\0';

    JUST_LOG_INFO(
      "Pool [%s]\n"
      "Capacity : %zu bytes\n"
      "Used     : %zu (%.1f%%)\n"
      "Free     : %zu (%.1f%%)",
      bar,
      ALLOCATOR->totalSize,
      used,
      usedPercent,
      freeBytes,
      freePercent);
  #else 
    (void)ALLOCATOR;
  #endif
}
