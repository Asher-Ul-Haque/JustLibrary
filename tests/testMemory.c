/**
 * @file testMemory.c
 * @brief Test suite for JustLibrary memory subsystems:
 *        tracker, linearAlloc, and objectPool.
 */

#include <justUtils/testing/testManager.h>
#include <justUtils/testing/expect.h>
#include <justUtils/memory/tracker.h>
#include <justUtils/memory/linearAlloc.h>
#include <justUtils/memory/objectPool.h>
#include <stdalign.h>
#include <stdint.h>
#include <string.h>

#define GROUP_TRACKER      0
#define GROUP_LINEAR_ALLOC 1
#define GROUP_OBJECT_POOL  2

// =============================================================================
// 1. MEMORY TRACKER TESTS
// =============================================================================

static JustTestResult testTrackerBasicAllocFree(void)
{
  const char* tag = "TEST_TAG";
  size_t initialBytes = justMemoryGetActiveBytes(tag);

  void* ptr = JUST_MALLOC_TAGGED(256, tag);
  JUST_EXPECT_TO_BE_NOT_NULL(ptr);

  JUST_EXPECT_TO_BE((size_t)(initialBytes + 256), justMemoryGetActiveBytes(tag));
  JUST_EXPECT_TO_BE_TRUE(justMemoryCheckBounds());

  JUST_FREE(ptr);

  JUST_EXPECT_TO_BE(initialBytes, justMemoryGetActiveBytes(tag));
  JUST_EXPECT_TO_BE_TRUE(justMemoryCheckBounds());

  return JUST_TEST_PASS;
}

static JustTestResult testTrackerMultipleAllocations(void)
{
  const char* tag = "MULTI_TAG";
  size_t initialBytes = justMemoryGetActiveBytes(tag);
  void* ptrs[8];
  size_t allocatedBytes = 0;

  for (size_t i = 0; i < 8; ++i)
  {
    size_t sz = (i + 1) * 32;
    ptrs[i] = JUST_MALLOC_TAGGED(sz, tag);
    JUST_EXPECT_TO_BE_NOT_NULL(ptrs[i]);
    allocatedBytes += sz;
  }

  JUST_EXPECT_TO_BE((size_t)(initialBytes + allocatedBytes), justMemoryGetActiveBytes(tag));
  JUST_EXPECT_TO_BE_TRUE(justMemoryCheckBounds());

  // Free in reverse order
  for (size_t i = 8; i > 0; --i)
  {
    JUST_FREE(ptrs[i - 1]);
  }

  JUST_EXPECT_TO_BE(initialBytes, justMemoryGetActiveBytes(tag));
  return JUST_TEST_PASS;
}

static JustTestResult testTrackerRealloc(void)
{
  void* ptr = JUST_MALLOC_TAGGED(64, "REALLOC_TAG");
  JUST_EXPECT_TO_BE_NOT_NULL(ptr);
  memset(ptr, 0xAA, 64);

  void* newPtr = JUST_REALLOC(ptr, 256);
  JUST_EXPECT_TO_BE_NOT_NULL(newPtr);

  // Validate data survived across realloc
  uint8_t* byteView = (uint8_t*)newPtr;
  for (size_t i = 0; i < 64; ++i)
  {
    JUST_EXPECT_TO_BE((uint8_t)0xAA, byteView[i]);
  }

  JUST_EXPECT_TO_BE_TRUE(justMemoryCheckBounds());
  JUST_FREE(newPtr);

  return JUST_TEST_PASS;
}

static JustTestResult testTrackerAllocationLimits(void)
{
  const char* tag = "LIMIT_TAG";
  justMemorySetLimit(128, tag);
  JUST_EXPECT_TO_BE((size_t)128, justMemoryGetLimit(tag));

  void* p1 = JUST_MALLOC_TAGGED(64, tag);
  JUST_EXPECT_TO_BE_NOT_NULL(p1);

  // Exceeds remaining 64 bytes of quota
  void* p2 = JUST_MALLOC_TAGGED(100, tag);
  JUST_EXPECT_TO_BE_NULL(p2);

  JUST_FREE(p1);

  // Reset limit
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 2. LINEAR ALLOCATOR TESTS
// =============================================================================

static JustTestResult testLinearAllocCreationAndReset(void)
{
  JustLinearAllocator alloc;
  bool created = justLinearAllocCreate(&alloc, 1024, NULL, "ARENA_TEST");
  JUST_EXPECT_TO_BE_TRUE(created);

  JUST_EXPECT_TO_BE((size_t)1024, alloc.totalSize);
  JUST_EXPECT_TO_BE((size_t)0, justLinearAllocGetUsed(&alloc));
  JUST_EXPECT_TO_BE((size_t)1024, justLinearAllocGetRemaining(&alloc));

  void* block = justLinearAllocAllocate(&alloc, 256, 0);
  JUST_EXPECT_TO_BE_NOT_NULL(block);
  JUST_EXPECT_TO_BE((size_t)256, justLinearAllocGetUsed(&alloc));
  JUST_EXPECT_TO_BE((size_t)(1024 - 256), justLinearAllocGetRemaining(&alloc));

  justLinearAllocReset(&alloc);
  JUST_EXPECT_TO_BE((size_t)0, justLinearAllocGetUsed(&alloc));
  JUST_EXPECT_TO_BE((size_t)1024, justLinearAllocGetRemaining(&alloc));

  justLinearAllocDestroy(&alloc);
  return JUST_TEST_PASS;
}

static JustTestResult testLinearAllocStrictAlignment(void)
{
  JustLinearAllocator alloc;
  justLinearAllocCreate(&alloc, 4096, NULL, "ALIGN_TEST");

  // Force unaligned offset: 1-byte allocation
  void* lead1 = justLinearAllocAllocate(&alloc, 1, 1);
  JUST_EXPECT_TO_BE_NOT_NULL(lead1);

  // Request 16-byte alignment
  void* aligned16 = justLinearAllocAllocate(&alloc, 64, 16);
  JUST_EXPECT_TO_BE_NOT_NULL(aligned16);
  JUST_EXPECT_TO_BE((uintptr_t)0, (uintptr_t)aligned16 & (16 - 1));

  // Force another unaligned offset
  void* lead2 = justLinearAllocAllocate(&alloc, 3, 1);
  JUST_EXPECT_TO_BE_NOT_NULL(lead2);

  // Request 64-byte alignment
  void* aligned64 = justLinearAllocAllocate(&alloc, 128, 64);
  JUST_EXPECT_TO_BE_NOT_NULL(aligned64);
  JUST_EXPECT_TO_BE((uintptr_t)0, (uintptr_t)aligned64 & (64 - 1));

  justLinearAllocDestroy(&alloc);
  return JUST_TEST_PASS;
}

static JustTestResult testLinearAllocOverflow(void)
{
  JustLinearAllocator alloc;
  justLinearAllocCreate(&alloc, 128, NULL, "OVERFLOW_TEST");

  void* p1 = justLinearAllocAllocate(&alloc, 100, 0);
  JUST_EXPECT_TO_BE_NOT_NULL(p1);

  // Request exceeds remaining space
  void* p2 = justLinearAllocAllocate(&alloc, 50, 0);
  JUST_EXPECT_TO_BE_NULL(p2);

  justLinearAllocDestroy(&alloc);
  return JUST_TEST_PASS;
}

static JustTestResult testLinearAllocMarkerRewind(void)
{
  JustLinearAllocator alloc;
  justLinearAllocCreate(&alloc, 1024, NULL, "REWIND_TEST");

  void* p1 = justLinearAllocAllocate(&alloc, 128, 0);
  JUST_EXPECT_TO_BE_NOT_NULL(p1);

  justLinearMarker mark = justLinearAllocGetMarker(&alloc);

  void* p2 = justLinearAllocAllocate(&alloc, 256, 0);
  void* p3 = justLinearAllocAllocate(&alloc, 128, 0);
  JUST_EXPECT_TO_BE_NOT_NULL(p2);
  JUST_EXPECT_TO_BE_NOT_NULL(p3);

  // Rollback to marker
  justLinearAllocRewind(&alloc, mark);
  JUST_EXPECT_TO_BE(mark, justLinearAllocGetUsed(&alloc));

  // Next allocation must reuse previous memory address range
  void* reassigned = justLinearAllocAllocate(&alloc, 64, 0);
  JUST_EXPECT_TO_BE_NOT_NULL(reassigned);
  JUST_EXPECT_TO_BE(p2, reassigned);

  justLinearAllocDestroy(&alloc);
  return JUST_TEST_PASS;
}

// =============================================================================
// 3. OBJECT POOL TESTS
// =============================================================================

typedef struct Particle
{
  float    x, y, z;
  float    vx, vy, vz;
  uint32_t id;
  uint8_t  active;
} Particle;

static JustTestResult testObjectPoolCreationAndSizing(void)
{
  JustObjectPool pool;
  bool ok = justObjectPoolCreate(&pool, 32, sizeof(Particle), alignof(Particle), NULL, "POOL_TEST");
  JUST_EXPECT_TO_BE_TRUE(ok);

  JUST_EXPECT_TO_BE((size_t)32, pool.capacity);
  JUST_EXPECT_TO_BE((size_t)32, pool.freeCount);
  JUST_EXPECT_TO_BE_TRUE(justObjectPoolIsEmpty(&pool));
  JUST_EXPECT_TO_BE_FALSE(justObjectPoolIsFull(&pool));

  // Stride must accommodate object size and adhere to alignment boundary
  JUST_EXPECT_TO_BE_TRUE(pool.stride >= sizeof(Particle));
  JUST_EXPECT_TO_BE((size_t)0, pool.stride % alignof(Particle));

  justObjectPoolDestroy(&pool);
  return JUST_TEST_PASS;
}

static JustTestResult testObjectPoolTakeAndReturn(void)
{
  JustObjectPool pool;
  justObjectPoolCreate(&pool, 4, sizeof(uint64_t), alignof(uint64_t), NULL, "POOL_TAKE_TEST");

  uint64_t* a = (uint64_t*)justObjectPoolTakeObject(&pool);
  uint64_t* b = (uint64_t*)justObjectPoolTakeObject(&pool);
  JUST_EXPECT_TO_BE_NOT_NULL(a);
  JUST_EXPECT_TO_BE_NOT_NULL(b);
  JUST_EXPECT_TO_BE((size_t)2, pool.freeCount);

  *a = 0xDEADBEEFCAFEBABEULL;
  *b = 0x1234567890ABCDEFULL;
  JUST_EXPECT_TO_BE(0xDEADBEEFCAFEBABEULL, *a);
  JUST_EXPECT_TO_BE(0x1234567890ABCDEFULL, *b);

  // Return slot 'a'
  justObjectPoolReturnObject(&pool, a);
  JUST_EXPECT_TO_BE((size_t)3, pool.freeCount);

  // LIFO recycling: next take should return the same address as 'a'
  uint64_t* c = (uint64_t*)justObjectPoolTakeObject(&pool);
  JUST_EXPECT_TO_BE(a, c);

  justObjectPoolReturnObject(&pool, b);
  justObjectPoolReturnObject(&pool, c);
  JUST_EXPECT_TO_BE_TRUE(justObjectPoolIsEmpty(&pool));

  justObjectPoolDestroy(&pool);
  return JUST_TEST_PASS;
}

static JustTestResult testObjectPoolSaturation(void)
{
  JustObjectPool pool;
  justObjectPoolCreate(&pool, 3, sizeof(int32_t), alignof(int32_t), NULL, "POOL_SAT_TEST");

  void* p1 = justObjectPoolTakeObject(&pool);
  void* p2 = justObjectPoolTakeObject(&pool);
  void* p3 = justObjectPoolTakeObject(&pool);

  JUST_EXPECT_TO_BE_NOT_NULL(p1);
  JUST_EXPECT_TO_BE_NOT_NULL(p2);
  JUST_EXPECT_TO_BE_NOT_NULL(p3);
  JUST_EXPECT_TO_BE_TRUE(justObjectPoolIsFull(&pool));

  // Pool is saturated: further takes must fail
  void* p4 = justObjectPoolTakeObject(&pool);
  JUST_EXPECT_TO_BE_NULL(p4);

  // Recycle one and reacquire
  justObjectPoolReturnObject(&pool, p2);
  JUST_EXPECT_TO_BE_FALSE(justObjectPoolIsFull(&pool));

  void* p5 = justObjectPoolTakeObject(&pool);
  JUST_EXPECT_TO_BE(p2, p5);

  justObjectPoolReturnObject(&pool, p1);
  justObjectPoolReturnObject(&pool, p3);
  justObjectPoolReturnObject(&pool, p5);

  justObjectPoolDestroy(&pool);
  return JUST_TEST_PASS;
}

static JustTestResult testObjectPoolDoubleFreeTrap(void)
{
#ifdef DEBUG
  // In debug mode, bitset verification will trigger an assertion (SIGABRT).
  // The fork-isolated test runner safely intercepts this crash.
  JustObjectPool pool;
  justObjectPoolCreate(&pool, 2, sizeof(int32_t), alignof(int32_t), NULL, "TRAP_TEST");

  void* obj = justObjectPoolTakeObject(&pool);
  justObjectPoolReturnObject(&pool, obj);

  // Double return: should assert and crash child process cleanly
  justObjectPoolReturnObject(&pool, obj);

  justObjectPoolDestroy(&pool);
  return JUST_TEST_FAIL;
#else
  return JUST_TEST_SKIP;
#endif
}

// =============================================================================
// MAIN ENTRY POINT
// =============================================================================

int main(void)
{
  // 1. Tracker Tests
  justTestRegister(testTrackerBasicAllocFree,     "Tracker: Basic Alloc and Free",          GROUP_TRACKER);
  justTestRegister(testTrackerMultipleAllocations, "Tracker: Multiple Allocations Tracking", GROUP_TRACKER);
  justTestRegister(testTrackerRealloc,             "Tracker: Realloc Data Preservation",     GROUP_TRACKER);
  justTestRegister(testTrackerAllocationLimits,    "Tracker: Memory Limit Enforcement",      GROUP_TRACKER);

  // 2. Linear Allocator Tests
  justTestRegister(testLinearAllocCreationAndReset, "LinearAlloc: Create and Reset",         GROUP_LINEAR_ALLOC);
  justTestRegister(testLinearAllocStrictAlignment,  "LinearAlloc: Alignment Boundaries",     GROUP_LINEAR_ALLOC);
  justTestRegister(testLinearAllocOverflow,         "LinearAlloc: Overflow Rejection",       GROUP_LINEAR_ALLOC);
  justTestRegister(testLinearAllocMarkerRewind,     "LinearAlloc: Marker Rewind Semantics",  GROUP_LINEAR_ALLOC);

  // 3. Object Pool Tests
  justTestRegister(testObjectPoolCreationAndSizing, "ObjectPool: Creation and Stride Sizing", GROUP_OBJECT_POOL);
  justTestRegister(testObjectPoolTakeAndReturn,     "ObjectPool: Take and LIFO Recycle",     GROUP_OBJECT_POOL);
  justTestRegister(testObjectPoolSaturation,        "ObjectPool: Saturation Boundary",       GROUP_OBJECT_POOL);
  justTestRegister(testObjectPoolDoubleFreeTrap,    "ObjectPool: Debug Double-Free Trap",     GROUP_OBJECT_POOL);

  return (int)justTestRunAll();
}
