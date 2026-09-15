#include <justUtils/core/asserts.h>
#include <justUtils/core/logger.h>
#include <justUtils/memory/objectPool.h>
#include <justUtils/memory/tracker.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>

static inline bool objectPoolIsPowerOfTwo(size_t x)
{
  return (x != 0) && ((x & (x - 1)) == 0);
}

bool justObjectPoolCreate(
  justObjectPool*  POOL,
  size_t            CAPACITY,
  size_t            OBJECT_SIZE,
  size_t            ALIGNMENT,
  void*             USER_MEMORY,
  const char*       TAG)
{
  JUST_ASSERT_DEBUG_MESSAGE(POOL != NULL,        "[OBJECT POOL] : Cannot initialize a NULL ObjectPool pointer");
  JUST_ASSERT_DEBUG_MESSAGE(CAPACITY > 0,        "[OBJECT POOL] : CAPACITY must be at least 1");
  JUST_ASSERT_DEBUG_MESSAGE(OBJECT_SIZE > 0,     "[OBJECT POOL] : OBJECT_SIZE must be greater than 0");

  POOL->capacity    = CAPACITY;
  POOL->objectSize  = OBJECT_SIZE;
  POOL->ownsMemory  = (USER_MEMORY == NULL);

  if (ALIGNMENT == 0) ALIGNMENT = alignof(max_align_t); 
  JUST_ASSERT_DEBUG_MESSAGE(objectPoolIsPowerOfTwo(ALIGNMENT), "[OBJECT POOL] : Alignment must be a power of 2");

  POOL->capacity   = CAPACITY;
  POOL->objectSize = OBJECT_SIZE;
  POOL->ownsMemory = (USER_MEMORY == NULL);

  // - - - Stride must fit at least a free-list size_t offset and be aligned to requested alignment
  size_t minSlotSize  = (OBJECT_SIZE < sizeof(size_t)) ? sizeof(size_t) : OBJECT_SIZE;
  POOL->stride        = (minSlotSize + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

  size_t      totalBytes  = POOL->capacity * POOL->stride;
  const char* allocTag    = (TAG && TAG[0] != '\0') ? TAG : "OBJECT_POOL";

  if (POOL->ownsMemory)
  {
    POOL->memory = JUST_MALLOC_TAGGED(totalBytes, allocTag);
    if (!POOL->memory)
    {
      JUST_LOG_FATAL("[OBJECT POOL] : Failed to allocate %zu bytes for POOL backing buffer", totalBytes);
      return false;
    }
  }
  else
  {
    JUST_ASSERT_DEBUG_MESSAGE(((uintptr_t)USER_MEMORY & (ALIGNMENT - 1)) == 0,
                               "[OBJECT POOL] : Provided userMemory is not aligned to requested boundary");
    POOL->memory = USER_MEMORY;
  }

  #ifdef DEBUG
    if (!justBitsetCreate(&POOL->allocatedBits, CAPACITY, NULL, "OBJECT_POOL"))
    {
      if (POOL->ownsMemory) JUST_FREE(POOL->memory);
      return false;
    }
  #endif

  // - - - Wire singly linked free-list
  POOL->freeListOffset = 0;
  POOL->freeCount      = CAPACITY;

  uint8_t* bytePtr = (uint8_t*)POOL->memory;
  for (size_t i = 0; i < CAPACITY - 1; ++i)
  {
    size_t* nextSlot  = (size_t*)(bytePtr + (i * POOL->stride));
    *nextSlot         = (i + 1) * POOL->stride;
  }

  size_t* lastSlot  = (size_t*)(bytePtr + ((CAPACITY - 1) * POOL->stride));
  *lastSlot         = JUST_POOL_END_OF_LIST;

  return true;
}

void justObjectPoolDestroy(justObjectPool* POOL) 
{
  JUST_ASSERT_DEBUG_MESSAGE(POOL != NULL, "[OBJECT POOL] : Cannot destroy a NULL ObjectPool pointer");

  #ifdef DEBUG
    justBitsetDestroy(&POOL->allocatedBits);
  #endif

  if (POOL->ownsMemory && POOL->memory) 
  {
    JUST_FREE(POOL->memory);
  }

  POOL->memory          = NULL;
  POOL->capacity        = 0;
  POOL->objectSize      = 0;
  POOL->stride          = 0;
  POOL->freeListOffset  = JUST_POOL_END_OF_LIST;
  POOL->freeCount       = 0;
  POOL->ownsMemory      = false;
}

void* justObjectPoolTakeObject(justObjectPool* POOL) 
{
  JUST_ASSERT_DEBUG_MESSAGE(POOL != NULL, "[OBJECT POOL] : Cannot take from NULL pool");
  JUST_ASSERT_DEBUG_MESSAGE(POOL->memory != NULL, "[OBJECT POOL] : Pool memory is NULL, make sure pool is initialized");

  if (POOL->freeListOffset == JUST_POOL_END_OF_LIST) 
  {
    JUST_LOG_ERROR("[OBJECT POOL] : Out of objects!");
    return NULL;
  }

  // - - - Pop element from free list using byte offset
  uintptr_t objAddr   = (uintptr_t)POOL->memory + POOL->freeListOffset;
  size_t    slotIndex = POOL->freeListOffset / POOL->stride;

  // - - - Pop head of free list
  POOL->freeListOffset = *(size_t*)objAddr;
  POOL->freeCount--;

  #ifdef DEBUG
    JUST_ASSERT_DEBUG_MESSAGE(!justBitsetGet(&POOL->allocatedBits, slotIndex),
                             "[OBJECT POOL] : Internal invariant failure: taking already allocated slot");
    justBitsetSet(&POOL->allocatedBits, slotIndex);
  #endif

  return (void*)objAddr;
}

void justObjectPoolReturnObject(justObjectPool* POOL, void* OBJECT) 
{
  JUST_ASSERT_DEBUG_MESSAGE(POOL != NULL, "[OBJECT POOL] Cannot return object to NULL pool");
  JUST_ASSERT_DEBUG_MESSAGE(OBJECT != NULL, "[OBJECT POOL] Cannot return NULL object");

  uintptr_t objAddr     = (uintptr_t)OBJECT;
  uintptr_t baseAddr    = (uintptr_t)POOL->memory;
  size_t    byteOffset  = objAddr - baseAddr;

  // - - - Verify bounds and alignment
  JUST_ASSERT_DEBUG_MESSAGE(objAddr >= baseAddr && byteOffset < (POOL->capacity * POOL->stride),
                             "[OBJECT POOL] : Returned pointer is outside pool boundary");
  JUST_ASSERT_DEBUG_MESSAGE((byteOffset % POOL->stride) == 0,
                             "[OBJECT POOL] : Returned pointer is misaligned with pool stride");

  size_t slotIndex = byteOffset / POOL->stride;

#ifdef DEBUG
  JUST_ASSERT_DEBUG_MESSAGE(justBitsetGet(&POOL->allocatedBits, slotIndex),
                             "[OBJECT POOL] : Double-free detected! Slot was not active or already returned");
  justBitsetClear(&POOL->allocatedBits, slotIndex);
#endif

  // - - - Push back onto head of free list
  *(size_t*)OBJECT      = POOL->freeListOffset;
  POOL->freeListOffset  = byteOffset;
  POOL->freeCount++;
}

void justObjectPoolDebugPrint(const justObjectPool* POOL)
{
  #ifdef DEBUG
    JUST_ASSERT_DEBUG_MESSAGE(POOL != NULL, "[OBJECT POOL] Cannot visualize a NULL pool");

    if (!POOL || POOL->capacity == 0) return;

    bool* freeSlots = (bool*)JUST_MALLOC(POOL->capacity * sizeof(bool));
    if (!freeSlots) return;
    memset(freeSlots, 0, POOL->capacity * sizeof(bool));

    char* bar = (char*)JUST_MALLOC(POOL->capacity + 1);
    if (!bar)
    {
      JUST_FREE(freeSlots);
      return;
    }

    // - - - Walk free list using byte offsets
    size_t curr_offset   = POOL->freeListOffset;
    size_t visited_count = 0;

    while (curr_offset != JUST_POOL_END_OF_LIST)
    {
      size_t slot_index = curr_offset / POOL->stride;

      if (slot_index >= POOL->capacity || (curr_offset % POOL->stride != 0))
      {
        JUST_LOG_ERROR("[OBJECT POOL] : Pool is corrupted (invalid offset %zu)", curr_offset);
        break;
      }

      if (visited_count >= POOL->capacity || freeSlots[slot_index])
      {
        JUST_LOG_ERROR("[OBJECT POOL] : Cycle detected in free list!");
        break;
      }

      freeSlots[slot_index] = true;
      visited_count++;

      uintptr_t slot_addr = (uintptr_t)POOL->memory + curr_offset;
      curr_offset = *(size_t*)slot_addr;
    }

    // - - - - Build bar representation
    size_t head_index = (POOL->freeListOffset != JUST_POOL_END_OF_LIST) ? (POOL->freeListOffset / POOL->stride) : JUST_POOL_END_OF_LIST;
    for (size_t i = 0; i < POOL->capacity; ++i)
    {
      if (i == head_index && freeSlots[i]) {
        bar[i] = '@'; // Free list head
      } else {
        bar[i] = freeSlots[i] ? '_' : '*'; // _ = Free, * = Occupied
      }
    }
    bar[POOL->capacity] = '\0';

    const size_t used = POOL->capacity - POOL->freeCount;

    JUST_LOG_INFO(
      "Pool [%s]\n"
      "Capacity : %zu\n"
      "Used     : %zu (%.1f%%)\n"
      "Free     : %zu (%.1f%%)\n",
      bar,
      POOL->capacity,
      used,
      POOL->capacity ? (100.0 * (double)used) / (double)POOL->capacity : 0.0,
      POOL->freeCount,
      POOL->capacity ? (100.0 * (double)POOL->freeCount) / (double)POOL->capacity : 0.0
    );

    JUST_FREE(bar);
    JUST_FREE(freeSlots);
  #else 
    (void)POOL;
  #endif
}
