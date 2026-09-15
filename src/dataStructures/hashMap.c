#include <justUtils/dataStructures/hashMap.h>
#include <justUtils/memory/tracker.h>
#include <justUtils/core/logger.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Slot header: hash + state
typedef struct justSlotHeader
{
  alignas(max_align_t) uint64_t   hash;
  justHashMapEntryState           state;
} justSlotHeader;

// - - - Fast 64-bit SplitMix-style hash for <=8 byte keys, fallback to block-hash
static uint64_t defaultFastHash(const void* KEY, size_t KEY_SIZE)
{
  const uint8_t*  bytes = (const uint8_t*)KEY;
  uint64_t        hash  = 14695981039346656037ULL;
  for (size_t i = 0; i < KEY_SIZE; ++i)
  {
    hash ^= bytes[i];
    hash *= 1099511628211ULL;
  }
  return hash;
}

static int32_t defaultComparator(const void* A, const void* B, size_t SIZE)
{
  if (SIZE == sizeof(uint64_t))
  {
    return *(const uint64_t*)A == *(const uint64_t*)B ? 0 : 1;
  }
  return memcmp(A, B, SIZE);
}

static inline size_t justMapNextPowerOfTwo(size_t N)
{
  if (N < JUST_MAP_DEFAULT_CAPACITY) return JUST_MAP_DEFAULT_CAPACITY;
  N--;
  N |= N >> 1; N |= N >> 2; N |= N >> 4; N |= N >> 8; N |= N >> 16;
#if UINTPTR_MAX > 0xFFFFFFFF
  N |= N >> 32;
#endif
  N++;
  return N;
}

static inline justSlotHeader* getHeader(const JustHashMap* MAP, size_t INDEX)
{
  return (justSlotHeader*)(MAP->slots + (INDEX * MAP->slotStride));
}

static inline void* getKeyPtr(const JustHashMap* MAP, size_t INDEX)
{
  return (void*)(MAP->slots + (INDEX * MAP->slotStride) + MAP->keyOffset);
}

static inline void* getValPtr(const JustHashMap* MAP, size_t INDEX)
{
  return (void*)(MAP->slots + (INDEX * MAP->slotStride) + MAP->valueOffset);
}

static bool allocateSlots(JustHashMap* MAP, size_t CAPACITY)
{
  size_t    totalBytes = CAPACITY * MAP->slotStride;
  uint8_t*  buf        = MAP->allocator
                 ? (uint8_t*) justLinearAllocAllocate(MAP->allocator, totalBytes, 16)
                 : (uint8_t*) JUST_MALLOC_TAGGED(totalBytes, MAP->tag);

  if (!buf) return false;

  memset(buf, 0, totalBytes);
  MAP->slots = buf;
  return true;
}

JUST_API bool justHashmapCreate(
  JustHashMap*           MAP,
  size_t                 KEY_SIZE,
  size_t                 VALUE_SIZE,
  size_t                 INITIAL_CAPACITY,
  justHashFunction       HASHER,
  justKeyCompareFunction COMPARATOR,
  justLinearAllocator*   ALLOCATOR,
  const char*            TAG)
{
  JUST_ASSERT_DEBUG_MESSAGE(MAP != NULL, "[HASH MAP] : Cannot create a NULL Hashmap");
  JUST_ASSERT_DEBUG_MESSAGE(KEY_SIZE > 0, "[HASH MAP] : Cannot create a Hash map with KEY_SIZE under 1");
  JUST_ASSERT_DEBUG_MESSAGE(VALUE_SIZE > 0, "[HASH MAP] : Cannot create a Hash map with VALUE_SIZE under 1");

  MAP->keySize          = KEY_SIZE;
  MAP->valueSize        = VALUE_SIZE;
  MAP->capacity         = justMapNextPowerOfTwo(INITIAL_CAPACITY);
  MAP->mask             = MAP->capacity ? (MAP->capacity - 1) : 0;
  MAP->count            = 0;
  MAP->tombstoneCount   = 0;
  MAP->hashFunction     = HASHER ? HASHER : defaultFastHash;
  MAP->compareFunction  = COMPARATOR ? COMPARATOR : defaultComparator;
  MAP->allocator        = ALLOCATOR;
  MAP->tag              = TAG;

  // - - - Interleaved slot layout: [Header] [Key] [Pad] [Value] [Pad]
  MAP->keyOffset    = sizeof(justSlotHeader);
  size_t keyAligned = (KEY_SIZE + 7) & ~7;
  MAP->valueOffset  = MAP->keyOffset + keyAligned;
  size_t valAligned = (VALUE_SIZE + 7) & ~7;
  MAP->slotStride   = MAP->valueOffset + valAligned;

  size_t maxAlign = alignof(max_align_t);
  size_t alignMask = maxAlign - 1;

  MAP->slotStride = (MAP->valueOffset + valAligned + alignMask) & ~alignMask;

  if (MAP->capacity == 0) return true;

  if (!allocateSlots(MAP, MAP->capacity))
  {
    JUST_LOG_ERROR("[HASHMAP] : Failed to allocate slot memory");
    MAP->capacity = 0;
    MAP->mask     = 0;
    return false;
  }

  return true;
}

JUST_API void justHashmapDestroy(JustHashMap* MAP)
{
  JUST_ASSERT_DEBUG(MAP != NULL);

  if (MAP->slots && !MAP->allocator)
  {
    JUST_FREE(MAP->slots);
  }

  MAP->slots          = NULL;
  MAP->capacity       = 0;
  MAP->mask           = 0;
  MAP->count          = 0;
  MAP->tombstoneCount = 0;
  MAP->allocator      = NULL;
}

JUST_API static bool hashmapResize(JustHashMap* MAP, size_t NEW_CAPACITY)
{
  uint8_t* oldSlots     = MAP->slots;
  size_t   oldCap       = MAP->capacity;
  size_t   oldStride    = MAP->slotStride;
  size_t   oldKeyOffset = MAP->keyOffset;
  size_t   oldValOffset = MAP->valueOffset;

  MAP->capacity       = NEW_CAPACITY;
  MAP->mask           = NEW_CAPACITY - 1;
  MAP->count          = 0;
  MAP->tombstoneCount = 0;

  if (!allocateSlots(MAP, NEW_CAPACITY)) return false;

  // - - - Re-insert using cached hash without calling hasher or comparator
  if (oldSlots && oldCap > 0)
  {
    for (size_t i = 0; i < oldCap; ++i)
    {
      justSlotHeader* oldHeader = (justSlotHeader*)(oldSlots + (i * oldStride));
      if (oldHeader->state == JUST_MAP_OCCUPIED)
      {
        const void* key   = (const void*)(oldSlots + (i * oldStride) + oldKeyOffset);
        const void* value = (const void*)(oldSlots + (i * oldStride) + oldValOffset);

        size_t index = oldHeader->hash & MAP->mask;
        while (getHeader(MAP, index)->state == JUST_MAP_OCCUPIED)
        {
          index = (index + 1) & MAP->mask;
        }

        justSlotHeader* newHdr = getHeader(MAP, index);
        newHdr->hash            = oldHeader->hash;
        newHdr->state           = JUST_MAP_OCCUPIED;

        memcpy(getKeyPtr(MAP, index), key, MAP->keySize);
        memcpy(getValPtr(MAP, index), value, MAP->valueSize);
        MAP->count++;
      }
    }
    if (!MAP->allocator) JUST_FREE(oldSlots);
  }

  return true;
}

JUST_API bool justHashmapSet(JustHashMap* MAP, const void* KEY_PTR, const void* VALUE_PTR)
{
  JUST_ASSERT_DEBUG_MESSAGE(MAP != NULL, "[HASH MAP] : Cannot set in a NULL MAP");
  JUST_ASSERT_DEBUG_MESSAGE(KEY_PTR != NULL, "[HASH MAP] : Cannot set with a KEY_PTR");
  JUST_ASSERT_DEBUG_MESSAGE(VALUE_PTR != NULL, "[HASH MAP] : Cannot set with a VALUE_PTR");

  // - - - Lazy alloc or 75% load factor expansion
  if (MAP->capacity == 0)
  {
    if (!hashmapResize(MAP, JUST_MAP_DEFAULT_CAPACITY)) return false;
  }

  // - - - Purge tombstones in-place if count alone is below 50% load, otherwise double
  else if ((MAP->count + MAP->tombstoneCount + 1) * 4 >= MAP->capacity * 3)
  {
    size_t targetCap = (MAP->count * 2 > MAP->capacity) ? (MAP->capacity * 2) : MAP->capacity;
    if (!hashmapResize(MAP, targetCap)) return false;
  }

  uint64_t hash      = MAP->hashFunction(KEY_PTR, MAP->keySize);
  size_t   index     = hash & MAP->mask;
  int64_t  firstTomb = -1;

  for (size_t i = 0; i < MAP->capacity; ++i)
  {
    size_t            probeIdx  = (index + i) & MAP->mask;
    justSlotHeader*  header    = getHeader(MAP, probeIdx);

    if (header->state == JUST_MAP_EMPTY)
    {
      size_t            targetIdx = (firstTomb != -1) ? (size_t)firstTomb : probeIdx;
      justSlotHeader*  targetHdr = getHeader(MAP, targetIdx);

      if (targetHdr->state == JUST_MAP_TOMBSTONE)
      {
        MAP->tombstoneCount--;
      }

      targetHdr->hash  = hash;
      targetHdr->state = JUST_MAP_OCCUPIED;

      memcpy(getKeyPtr(MAP, targetIdx), KEY_PTR, MAP->keySize);
      memcpy(getValPtr(MAP, targetIdx), VALUE_PTR, MAP->valueSize);
      MAP->count++;
      return true;
    }

    if (header->state == JUST_MAP_TOMBSTONE)
    {
      if (firstTomb == -1) firstTomb = (int64_t)probeIdx;
    }
    else if (header->hash == hash)
    {
      if (MAP->compareFunction(getKeyPtr(MAP, probeIdx), KEY_PTR, MAP->keySize) == 0)
      {
        memcpy(getValPtr(MAP, probeIdx), VALUE_PTR, MAP->valueSize);
        return true;
      }
    }
  }

  // - - - If table had tombstones but no EMPTY slots in probe path
  if (firstTomb != -1)
  {
    size_t targetIndex            = (size_t) firstTomb;
    justSlotHeader* targetHeader = getHeader(MAP, targetIndex);

    MAP->tombstoneCount--;
    targetHeader->hash  = hash;
    targetHeader->state = JUST_MAP_OCCUPIED;

    memcpy(getKeyPtr(MAP, targetIndex), KEY_PTR, MAP->keySize);
    memcpy(getKeyPtr(MAP, targetIndex), VALUE_PTR, MAP->valueSize);
    MAP->count++;
    return true;
  }

  return false;
}

JUST_API void* justHashmapGet(const JustHashMap* MAP, const void* KEY_PTR)
{
  JUST_ASSERT_DEBUG_MESSAGE(MAP != NULL, "[HASH MAP] : Cannot get in a NULL MAP");
  JUST_ASSERT_DEBUG_MESSAGE(KEY_PTR != NULL, "[HASH MAP] : Cannot get with a KEY_PTR");

  if (MAP->count == 0 || MAP->capacity == 0) return NULL;

  uint64_t hash = MAP->hashFunction(KEY_PTR, MAP->keySize);
  size_t   idx  = hash & MAP->mask;

  for (size_t i = 0; i < MAP->capacity; ++i)
  {
    size_t            probeIdx  = (idx + i) & MAP->mask;
    justSlotHeader*  header    = getHeader(MAP, probeIdx);

    if (header->state == JUST_MAP_EMPTY) return NULL;

    if (header->state == JUST_MAP_OCCUPIED && header->hash == hash)
    {
      if (MAP->compareFunction(getKeyPtr(MAP, probeIdx), KEY_PTR, MAP->keySize) == 0)
      {
        return getValPtr(MAP, probeIdx);
      }
    }
  }

  return NULL;
}

JUST_API bool justHashmapRemove(JustHashMap* MAP, const void* KEY_PTR)
{
  JUST_ASSERT_DEBUG_MESSAGE(MAP != NULL, "[HASH MAP] : Cannot remove from a NULL MAP");
  JUST_ASSERT_DEBUG_MESSAGE(KEY_PTR != NULL, "[HASH MAP] : Cannot remove with a NULL KEY_PTR");

  if (MAP->count == 0 || MAP->capacity == 0) return false;

  uint64_t hash   = MAP->hashFunction(KEY_PTR, MAP->keySize);
  size_t   index  = hash & MAP->mask;

  for (size_t i = 0; i < MAP->capacity; ++i)
  {
    size_t            probeIdx  = (index + i) & MAP->mask;
    justSlotHeader*  header    = getHeader(MAP, probeIdx);

    if (header->state == JUST_MAP_EMPTY) return false;

    if (header->state == JUST_MAP_OCCUPIED && header->hash == hash)
    {
      if (MAP->compareFunction(getKeyPtr(MAP, probeIdx), KEY_PTR, MAP->keySize) == 0)
      {
        header->state = JUST_MAP_TOMBSTONE;
        MAP->count--;
        MAP->tombstoneCount++;
        return true;
      }
    }
  }

  return false;
}

JUST_API void justHashmapClear(JustHashMap* MAP)
{
  JUST_ASSERT_DEBUG_MESSAGE(MAP != NULL, "[HASH MAP] : Cannot clear a NULL MAP");
  if (!MAP->slots) return;

  memset(MAP->slots, 0, MAP->capacity * MAP->slotStride);
  MAP->count          = 0;
  MAP->tombstoneCount = 0;
}
