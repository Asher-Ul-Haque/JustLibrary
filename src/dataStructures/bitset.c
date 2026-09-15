#include <justUtils/dataStructures/bitset.h>
#include <justUtils/memory/tracker.h>
#include <justUtils/core/logger.h>
#include <stdalign.h>
#include <stdint.h>

JUST_API bool justBitsetCreate(
  justBitset* SET,
  size_t       CAPACITY,
  void*        USER_BUFFER,
  const char*  TAG)
{
  JUST_ASSERT_DEBUG_MESSAGE(SET != NULL, "[BITSET] : Target bitset cannot be NULL");
  JUST_ASSERT_DEBUG_MESSAGE(CAPACITY > 0, "[BITSET] : Capacity must be > 0");

  SET->capacity   = CAPACITY;
  SET->wordCount  = (CAPACITY + JUST_BITS_PER_WORD - 1) / JUST_BITS_PER_WORD;
  SET->ownsMemory = (USER_BUFFER == NULL);

  size_t totalBytes = SET->wordCount * sizeof(uint64_t);

  if (SET->ownsMemory)
  {
    const char* allocTag = (TAG && TAG[0] != '\0') ? TAG : "BITSET";
    SET->words = (uint64_t*) JUST_MALLOC_TAGGED(totalBytes, allocTag);
    if (!SET->words)
    {
      JUST_LOG_ERROR("[BITSET] : Failed to allocate buffer of %zu bytes", totalBytes);
      return false;
    }
  }
  else
  {
    JUST_ASSERT_DEBUG_MESSAGE(
      ((uintptr_t)USER_BUFFER % alignof(uint64_t)) == 0,
      "[BITSET] : Provided userBuffer is not properly word-aligned");
    SET->words = (uint64_t*) USER_BUFFER;
  }

  justBitsetClearAll(SET);
  return true;
}

JUST_API void justBitsetDestroy(justBitset* SET)
{
  JUST_ASSERT_DEBUG_MESSAGE(SET != NULL, "[BITSET] : Cannot destroy a NULL set");

  if (SET->ownsMemory && SET->words)
  {
    JUST_FREE(SET->words);
  }

  SET->words      = NULL;
  SET->capacity   = 0;
  SET->wordCount  = 0;
  SET->ownsMemory = false;
}

JUST_API bool justBitsetEquals(const justBitset* A, const justBitset* B)
{
  JUST_ASSERT_DEBUG_MESSAGE(A != NULL && B != NULL, "[BITSET] : Cannot compare NULL sets");

  if (A->capacity != B->capacity) return false;
  if (A->wordCount == 0)          return true;

  // - - - Compare full words
  for (size_t i = 0; i < A->wordCount - 1; ++i)
  {
    if (A->words[i] != B->words[i]) return false;
  }

  uint64_t  tailMask  = justBitsetGetTailMask(A->capacity);
  size_t    lastIdx   = A->wordCount - 1;
  return ((A->words[lastIdx] & tailMask) == (B->words[lastIdx] & tailMask));
}

JUST_API void justBitsetUnion(justBitset* DST, const justBitset* SRC)
{
  JUST_ASSERT_DEBUG_MESSAGE(DST != NULL && SRC != NULL, "[BITSET] : Cannot union NULL sets");

  size_t                    count = (DST->wordCount < SRC->wordCount) ? DST->wordCount : SRC->wordCount;
  uint64_t* restrict        d     = DST->words;
  const uint64_t* restrict  s     = SRC->words;

  for (size_t i = 0; i < count; ++i)
  {
    d[i] |= s[i];
  }
}

JUST_API void justBitsetIntersection(justBitset* DST, const justBitset* SRC)
{
  JUST_ASSERT_DEBUG_MESSAGE(DST != NULL && SRC != NULL, "[BITSET] : Cannot intersect NULL bitsets");

  size_t                    count = (DST->wordCount < SRC->wordCount) ? DST->wordCount : SRC->wordCount;
  uint64_t* restrict        d     = DST->words;
  const uint64_t* restrict  s     = SRC->words;

  for (size_t i = 0; i < count; ++i)
  {
    d[i] &= s[i];
  }
}

JUST_API void justBitsetDifference(justBitset* DST, const justBitset* SRC)
{
  JUST_ASSERT_DEBUG_MESSAGE(DST != NULL && SRC != NULL, "[BITSET] : Cannot difference NULL bitsets");

  size_t                    count = (DST->wordCount < SRC->wordCount) ? DST->wordCount : SRC->wordCount;
  uint64_t* restrict        d     = DST->words;
  const uint64_t* restrict  s     = SRC->words;

  for (size_t i = 0; i < count; ++i)
  {
    d[i] &= ~s[i];
  }
}
