#include <justUtils/defines.h>
#include <justUtils/memory/tracker.h>
#include <justUtils/core/asserts.h>
#include <justUtils/core/logger.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdalign.h>


// - - -  Magic numbers for canary guards
#define JUST_HEADER_MAGIC 0xCAFEBABEU
#define JUST_FOOTER_MAGIC 0xDEADBEEFU
#define JUST_FREED_MAGIC  0xAA00BB11U

/// @brief : Tag Entry, how much per tag
typedef struct justTagEntry
{
  const char* name;
  size_t      allocatedBytes;
  size_t      allocationLimit;
  size_t      activeCount;
} justTagEntry;

/// @brief : Memory tracker header
typedef struct justMemoryHeader
{
  alignas(max_align_t) size_t requestedSize;
  size_t                      alignedPayloadSize;
  const char*                 file;
  const char*                 func;
  const char*                 tag;
  struct justMemoryHeader*   next;
  struct justMemoryHeader*   prev;
  uint32_t                    magic;
  int32_t                     line;
} justMemHeader;

// - - - Doubly linked list tracking all active allocations
static justMemHeader*  activeAllocations                   = NULL;
static size_t          registeredTagCount                  = 1;
static justTagEntry    memoryTags[JUST_MEMORY_TAG_LIMIT]  =
  {
    [0] =
      {
        .name            = "UNKNOWN",
        .allocatedBytes  = 0,
        .allocationLimit = JUST_MEMORY_ALLOC_DEFAULT_LIMIT,
        .activeCount     = 0
      }
  };


// - - - Internal Helpers - - -

static justTagEntry* getOrCreateTag(const char* TAG)
{
  // - - -If NULL, empty, or literally "UNKNOWN", route directly to slot 0
  if (!TAG || TAG[0] == '\0' || TAG == memoryTags[0].name || strcmp(TAG, "UNKNOWN") == 0)
  {
    return &memoryTags[0];
  }

  for (size_t i = 0; i < registeredTagCount; ++i)
  {
    if (memoryTags[i].name == TAG || strcmp(memoryTags[i].name, TAG) == 0)
    { return &memoryTags[i]; }
  }

  if (registeredTagCount < JUST_MEMORY_TAG_LIMIT)
  {
    justTagEntry* entry    = &memoryTags[registeredTagCount++];
    entry->name             = TAG;
    entry->allocatedBytes   = 0;
    entry->allocationLimit  = JUST_MEMORY_ALLOC_DEFAULT_LIMIT;
    entry->activeCount      = 0;

    return entry;
  }

  JUST_LOG_WARNING("[MEMORY TRACKER] : Max tag limit (%zu) reached! Assigning tag '%s' to UNKNOWN",
                    (size_t)JUST_MEMORY_TAG_LIMIT, TAG);
  JUST_LOG_WARNING("[MEMORY TRACKER] : The tag limit is configurable at compile time, check docs to see how");
  return &memoryTags[0];
}

static inline uint8_t* getPayloadPtr(const justMemHeader* HEADER)
{
  return (uint8_t*)HEADER + sizeof(justMemHeader);
}

static inline justMemHeader* getHeaderPtr(const void* PAYLOAD)
{
  return (justMemHeader*)((uint8_t*)PAYLOAD - sizeof(justMemHeader));
}

static inline uint32_t* getFooterPtr(const justMemHeader* header)
{
  return (uint32_t*)(getPayloadPtr(header) + header->alignedPayloadSize);
}

static void linkAllocation(justMemHeader* HEADER)
{
  HEADER->next = activeAllocations;
  HEADER->prev = NULL;
  if (activeAllocations)
  {
    activeAllocations->prev = HEADER;
  }
  activeAllocations = HEADER;

  justTagEntry* tagEntry   = getOrCreateTag(HEADER->tag);
  tagEntry->allocatedBytes += HEADER->requestedSize;
  tagEntry->activeCount++;
}

static void unlinkAllocation(justMemHeader* header)
{
  if (header->prev) header->prev->next = header->next;
  else              activeAllocations = header->next;

  if (header->next) header->next->prev = header->prev;

  justTagEntry* tagEntry   = getOrCreateTag(header->tag);
  tagEntry->allocatedBytes -= header->requestedSize;
  tagEntry->activeCount--;
}

static bool verifyIntegrity(justMemHeader* HEADER, const char* FILE, const char* FUNC, int32_t LINE)
{
  if (!HEADER) return false;

  if (HEADER->magic == JUST_FREED_MAGIC)
  {
    JUST_LOG_FATAL("[MEMORY TRACKER] : Use-after-free or double-free at %s:%d in %s()! Buffer was already freed.",
                    FILE, LINE, FUNC);
    abort();
  }

  if (HEADER->magic != JUST_HEADER_MAGIC)
  {
    JUST_LOG_FATAL("[MEMORY TRACKER] : Header corruption at %s:%d in %s()! Magic mismatch.",
                    FILE, LINE, FUNC);
    abort();
  }

  uint32_t* footer = getFooterPtr(HEADER);
  uint32_t  footerVal;
  memcpy(&footerVal, footer, sizeof(uint32_t));

  if (footerVal != JUST_FOOTER_MAGIC)
  {
    JUST_LOG_FATAL("[MEMORY TRACKER] : Buffer overflow detected! Tail canary overwritten for %zu-byte allocation (origin: %s:%d in %s). Triggered at %s:%d in %s.",
                    HEADER->requestedSize, HEADER->file, HEADER->line, HEADER->func, FILE, LINE, FUNC);
    abort();
  }

  return true;
}


// - - - Public API Implementation - - -

JUST_API void* justTrackedMalloc(
  size_t      SIZE,
  const char* FILE,
  const char* FUNC,
  int32_t     LINE,
  const char* TAG)
{
  JUST_ASSERT_DEBUG_MESSAGE(SIZE != 0, "[TRACKER] : Cannot allocate 0 bytes");

  justTagEntry* tagEntry = getOrCreateTag(TAG);

  if (tagEntry->allocationLimit > 0 && (tagEntry->allocatedBytes + SIZE > tagEntry->allocationLimit))
  {
    JUST_LOG_ERROR("[MEMORY TRACKER] : Limit of %zu bytes exceeded by allocating %zu bytes for tag '%s' at %s:%d",
                    tagEntry->allocationLimit, SIZE, tagEntry->name, FILE, LINE);
    return NULL;
  }

  size_t alignedPayload = (SIZE + 7) & ~7;
  size_t totalBytes     = sizeof(justMemHeader) + alignedPayload + sizeof(uint32_t);

  justMemHeader* header = (justMemHeader*) malloc(totalBytes);
  if (!header)
  {
    JUST_LOG_FATAL("[MEMORY TRACKER] : OS malloc failed for %zu bytes", totalBytes);
    return NULL;
  }

  header->magic              = JUST_HEADER_MAGIC;
  header->requestedSize      = SIZE;
  header->alignedPayloadSize = alignedPayload;
  header->file               = FILE;
  header->func               = FUNC;
  header->line               = LINE;
  header->tag                = tagEntry->name;

  uint32_t footerMagic = JUST_FOOTER_MAGIC;
  memcpy(getFooterPtr(header), &footerMagic, sizeof(uint32_t));

  linkAllocation(header);
  return getPayloadPtr(header);
}

JUST_API void* justTrackedCalloc(
  size_t      COUNT,
  size_t      SIZE,
  const char* FILE,
  const char* FUNC,
  int32_t     LINE,
  const char* TAG)
{
  JUST_ASSERT_DEBUG_MESSAGE(COUNT != 0 && SIZE != 0, "[MEMORY TRACKER] : Calloc arguments cannot be 0");

  size_t  total = COUNT * SIZE;
  void*   ptr   = justTrackedMalloc(total, FILE, FUNC, LINE, TAG);

  if (ptr) memset(ptr, 0, total);
  return ptr;
}

JUST_API void* justTrackedRealloc(
  void*       PTR,
  size_t      NEW_SIZE,
  const char* FILE,
  const char* FUNC,
  int32_t     LINE)
{
  JUST_ASSERT_DEBUG_MESSAGE(PTR != NULL, "[MEMORY TRACKER] : Unlike realloc, this library wont call malloc for you if you pass a NULL PTR");
  JUST_ASSERT_DEBUG_MESSAGE(NEW_SIZE != 0, "[MEMORY TRACKER] : Realloc size cannot be 0");

  justMemHeader* oldHeader = getHeaderPtr(PTR);
  if (!verifyIntegrity(oldHeader, FILE, FUNC, LINE)) return NULL;

  justTagEntry* tagEntry = getOrCreateTag(oldHeader->tag);

  if (tagEntry->allocationLimit > 0)
  {
    size_t projected = (tagEntry->allocatedBytes - oldHeader->requestedSize) + NEW_SIZE;
    if (projected > tagEntry->allocationLimit)
    {
      JUST_LOG_ERROR("[MEMORY TRACKER] : Limit exceeded during realloc of %zu bytes for tag '%s'", NEW_SIZE, tagEntry->name);
      return NULL;
    }
  }

  unlinkAllocation(oldHeader);

  size_t alignedPayload = (NEW_SIZE + 7) & ~7;
  size_t totalBytes     = sizeof(justMemHeader) + alignedPayload + sizeof(uint32_t);

  justMemHeader* newHeader = (justMemHeader*) realloc(oldHeader, totalBytes);
  if (!newHeader)
  {
    linkAllocation(oldHeader);
    JUST_LOG_FATAL("[MEMORY TRACKER] : OS realloc failed for %zu bytes", totalBytes);
    return NULL;
  }

  newHeader->requestedSize      = NEW_SIZE;
  newHeader->alignedPayloadSize = alignedPayload;
  newHeader->file               = FILE;
  newHeader->func               = FUNC;
  newHeader->line               = LINE;

  uint32_t footerMagic = JUST_FOOTER_MAGIC;
  memcpy(getFooterPtr(newHeader), &footerMagic, sizeof(uint32_t));

  linkAllocation(newHeader);
  return getPayloadPtr(newHeader);
}

JUST_API void justTrackedFree(void* PTR, const char* FILE, const char* FUNC, int32_t LINE)
{
  if (!PTR) return;

  justMemHeader* header = getHeaderPtr(PTR);
  if (!verifyIntegrity(header, FILE, FUNC, LINE)) return;

  unlinkAllocation(header);

  header->magic         = JUST_FREED_MAGIC;
  uint32_t freedFooter  = JUST_FREED_MAGIC;
  memcpy(getFooterPtr(header), &freedFooter, sizeof(uint32_t));

  free(header);
}

JUST_API void justMemoryReportLeaks(void)
{
  if (!activeAllocations)
  {
    JUST_LOG_INFO("[MEMORY TRACKER] : No memory leaks detected. Clean exit.");
    return;
  }

  size_t totalLeaked = 0;
  size_t count       = 0;

  JUST_LOG_ERROR("[MEMORY TRACKER] : Memory Leaks Detected (Trying to report, but reporting itself may cause segfault):");
  justMemHeader* curr = activeAllocations;
  while (curr)
  {
    count++;
    JUST_LOG_ERROR("  Leak #%zu: %zu bytes | Tag: '%s' | Origin: %s:%d (%s)",
                    count, curr->requestedSize, curr->tag, curr->file, curr->line, curr->func);
    totalLeaked  += curr->requestedSize;
    curr          = curr->next;
  }

  JUST_LOG_ERROR("[MEMORY TRACKER] : Total Leaked: %zu bytes across %zu allocations.", totalLeaked, count);
}

JUST_API bool justMemoryCheckBounds(void)
{
  bool            clean = true;
  justMemHeader*  curr  = activeAllocations;
  while (curr)
  {
    if (!verifyIntegrity(curr, __FILE__, __func__, __LINE__)) clean = false;
    curr = curr->next;
  }
  return clean;
}

JUST_API void justMemorySetLimit(size_t LIMIT_BYTES, const char* TAG)
{
  justTagEntry* entry     = getOrCreateTag(TAG);
  entry->allocationLimit  = LIMIT_BYTES == 0 ? SIZE_MAX : LIMIT_BYTES;
}

JUST_API size_t justMemoryGetLimit(const char* TAG)
{
  justTagEntry* entry = getOrCreateTag(TAG);
  return entry->allocationLimit;
}

JUST_API size_t justMemoryGetActiveBytes(const char* TAG)
{
  if (!TAG)
  {
    size_t total = 0;
    for (size_t i = 0; i < registeredTagCount; ++i) total += memoryTags[i].allocatedBytes;
    return total;
  }
  justTagEntry* entry = getOrCreateTag(TAG);
  return entry->allocatedBytes;
}

JUST_API void justMemoryLogUsage(bool VERBOSE)
{
  JUST_LOG_INFO("[MEMORY TRACKER] : Registered Memory Subsystems (%zu tags):", registeredTagCount);
  for (size_t i = 0; i < registeredTagCount; ++i)
  {
    justTagEntry* e = &memoryTags[i];
    if (e->allocationLimit > 0)
    {
      double pct = (double)e->allocatedBytes / (double)e->allocationLimit * 100.0;
      JUST_LOG_INFO("  ├── Tag: %-16s | %zu / %zu bytes (%.1f%%) [%zu active]",
                     e->name, e->allocatedBytes, e->allocationLimit, pct, e->activeCount);
      #ifndef DEBUG
        (void) pct;
      #endif
    }
    else
    {
      JUST_LOG_INFO("  ├── Tag: %-16s | %zu bytes (unlimited) [%zu active]",
                     e->name, e->allocatedBytes, e->activeCount);
    }

    if (VERBOSE && e->activeCount > 0)
    {
      justMemHeader* curr = activeAllocations;
      while (curr)
      {
        if (curr->tag == e->name || strcmp(curr->tag, e->name) == 0)
        {
          JUST_LOG_DEBUG("  │   └── %zu bytes at %s:%d (%s)",
                          curr->requestedSize, curr->file, curr->line, curr->func);
        }
        curr = curr->next;
      }
    }
  }
}
