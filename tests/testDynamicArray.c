/**
 * @file testDynamicArray.c
 * @brief Unit test suite for justDynamicArray.
 */

#include <justUtils/testing/testManager.h>
#include <justUtils/testing/expect.h>
#include <justUtils/dataStructures/dynamicArray.h>
#include <justUtils/memory/linearAlloc.h>
#include <justUtils/memory/tracker.h>
#include <stdint.h>
#include <string.h>

#define GROUP_DARRAY_BASIC  0
#define GROUP_DARRAY_OPS    1
#define GROUP_DARRAY_ARENA  2

typedef struct Vector3D
{
  float x, y, z;
} Vector3D;

// =============================================================================
// 1. BASIC LIFECYCLE & CAPACITY TESTS
// =============================================================================

static JustTestResult testDynamicArrayCreateDestroy(void)
{
  justDynamicArray arr;
  bool ok = JUST_DARRAY_INIT(&arr, 4, int32_t);
  JUST_EXPECT_TO_BE_TRUE(ok);

  JUST_EXPECT_TO_BE((size_t)0, justDynamicArraySize(&arr));
  JUST_EXPECT_TO_BE((size_t)4, justDynamicArrayCapacity(&arr));
  JUST_EXPECT_TO_BE_TRUE(justDynamicArrayIsEmpty(&arr));
  JUST_EXPECT_TO_BE((size_t)sizeof(int32_t), arr.elementSize);

  justDynamicArrayDestroy(&arr);
  return JUST_TEST_PASS;
}

static JustTestResult testDynamicArrayReserveAndShrink(void)
{
  const char* tag = "DARRAY_RESERVE";
  justMemorySetLimit(4096, tag);

  justDynamicArray arr;
  bool ok = JUST_DARRAY_INIT_TAGGED(&arr, 2, uint32_t, tag);
  JUST_EXPECT_TO_BE_TRUE(ok);

  // Reserve up to 32 elements
  bool reserved = justDynamicArrayReserve(&arr, 32);
  JUST_EXPECT_TO_BE_TRUE(reserved);
  JUST_EXPECT_TO_BE_TRUE(justDynamicArrayCapacity(&arr) >= 32);
  JUST_EXPECT_TO_BE((size_t)0, justDynamicArraySize(&arr));

  // Push 3 values
  for (uint32_t i = 1; i <= 3; ++i)
  {
    JUST_DARRAY_PUSH_VAL(&arr, uint32_t, i * 10);
  }
  JUST_EXPECT_TO_BE((size_t)3, justDynamicArraySize(&arr));

  // Shrink capacity down to match size
  bool shrunk = justDynamicArrayShrinkToFit(&arr);
  JUST_EXPECT_TO_BE_TRUE(shrunk);
  JUST_EXPECT_TO_BE((size_t)3, justDynamicArrayCapacity(&arr));
  JUST_EXPECT_TO_BE((size_t)3, justDynamicArraySize(&arr));

  // Confirm values survived reallocation
  JUST_EXPECT_TO_BE((uint32_t)10, JUST_DARRAY_GET(&arr, uint32_t, 0));
  JUST_EXPECT_TO_BE((uint32_t)20, JUST_DARRAY_GET(&arr, uint32_t, 1));
  JUST_EXPECT_TO_BE((uint32_t)30, JUST_DARRAY_GET(&arr, uint32_t, 2));

  justDynamicArrayDestroy(&arr);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 2. DATA OPERATIONS, PUSH, POP, EMPLACE & MACROS
// =============================================================================

static JustTestResult testDynamicArrayPushAndPop(void)
{
  const char* tag = "DARRAY_OPS";
  justMemorySetLimit(4096, tag);

  justDynamicArray arr;
  JUST_DARRAY_INIT_TAGGED(&arr, 2, int64_t, tag);

  // Push 10 elements to force multiple geometric growths
  for (int64_t i = 0; i < 10; ++i)
  {
    JUST_DARRAY_PUSH_VAL(&arr, int64_t, i * 100);
  }

  JUST_EXPECT_TO_BE((size_t)10, justDynamicArraySize(&arr));
  JUST_EXPECT_TO_BE_FALSE(justDynamicArrayIsEmpty(&arr));

  // Verify elements via indexed access
  for (size_t i = 0; i < 10; ++i)
  {
    int64_t val = JUST_DARRAY_GET(&arr, int64_t, i);
    JUST_EXPECT_TO_BE((int64_t)(i * 100), val);
  }

  // Pop elements in LIFO order
  int64_t popped = 0;
  bool popOk = justDynamicArrayPop(&arr, &popped);
  JUST_EXPECT_TO_BE_TRUE(popOk);
  JUST_EXPECT_TO_BE((int64_t)900, popped);
  JUST_EXPECT_TO_BE((size_t)9, justDynamicArraySize(&arr));

  // Clear without freeing memory
  justDynamicArrayClear(&arr);
  JUST_EXPECT_TO_BE((size_t)0, justDynamicArraySize(&arr));
  JUST_EXPECT_TO_BE_TRUE(justDynamicArrayIsEmpty(&arr));
  JUST_EXPECT_TO_BE_TRUE(justDynamicArrayCapacity(&arr) >= 10);

  // Pop on empty array must safely fail
  bool emptyPop = justDynamicArrayPop(&arr, &popped);
  JUST_EXPECT_TO_BE_FALSE(emptyPop);

  justDynamicArrayDestroy(&arr);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

static JustTestResult testDynamicArrayEmplaceStruct(void)
{
  justDynamicArray arr;
  JUST_DARRAY_INIT(&arr, 2, Vector3D);

  // Emplace directly into uninitialized storage (zero-copy)
  Vector3D* v1 = JUST_DARRAY_EMPLACE(&arr, Vector3D);
  JUST_EXPECT_TO_BE_NOT_NULL(v1);
  v1->x = 1.0f; v1->y = 2.0f; v1->z = 3.0f;

  Vector3D* v2 = JUST_DARRAY_EMPLACE(&arr, Vector3D);
  JUST_EXPECT_TO_BE_NOT_NULL(v2);
  v2->x = 4.0f; v2->y = 5.0f; v2->z = 6.0f;

  JUST_EXPECT_TO_BE((size_t)2, justDynamicArraySize(&arr));

  Vector3D* fetched = (Vector3D*)justDynamicArrayAt(&arr, 1);
  JUST_EXPECT_TO_BE_NOT_NULL(fetched);
  JUST_EXPECT_FLOAT_TO_BE(4.0f, fetched->x, 0.0001f);
  JUST_EXPECT_FLOAT_TO_BE(5.0f, fetched->y, 0.0001f);
  JUST_EXPECT_FLOAT_TO_BE(6.0f, fetched->z, 0.0001f);

  justDynamicArrayDestroy(&arr);
  return JUST_TEST_PASS;
}

static JustTestResult testDynamicArrayPushRange(void)
{
  const char* tag = "DARRAY_RANGE";
  justMemorySetLimit(4096, tag);

  justDynamicArray arr;
  JUST_DARRAY_INIT_TAGGED(&arr, 2, int32_t, tag);

  int32_t sourceData[6] = { 10, 20, 30, 40, 50, 60 };
  bool ok = JUST_DARRAY_PUSH_RANGE(&arr, sourceData, 6);
  JUST_EXPECT_TO_BE_TRUE(ok);

  JUST_EXPECT_TO_BE((size_t)6, justDynamicArraySize(&arr));

  int32_t* rawData = JUST_DARRAY_DATA(&arr, int32_t);
  for (size_t i = 0; i < 6; ++i)
  {
    JUST_EXPECT_TO_BE(sourceData[i], rawData[i]);
  }

  justDynamicArrayDestroy(&arr);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 3. ARENA (LINEAR ALLOCATOR) BACKED ARRAY
// =============================================================================

static JustTestResult testDynamicArrayArenaBacked(void)
{
  const char* tag = "DARRAY_ARENA";
  justLinearAllocator arena;
  bool arenaOk = justLinearAllocCreate(&arena, 512, NULL, tag);
  JUST_EXPECT_TO_BE_TRUE(arenaOk);

  justDynamicArray arr;
  bool arrOk = justDynamicArrayCreate(&arr, 4, sizeof(uint32_t), &arena, tag);
  JUST_EXPECT_TO_BE_TRUE(arrOk);

  for (uint32_t i = 0; i < 4; ++i)
  {
    JUST_DARRAY_PUSH_VAL(&arr, uint32_t, (i + 1) * 7);
  }
  JUST_EXPECT_TO_BE((size_t)4, justDynamicArraySize(&arr));

  // Destroying arena-backed array must not call free() on arena memory
  justDynamicArrayDestroy(&arr);
  JUST_EXPECT_TO_BE_TRUE(justMemoryCheckBounds());

  justLinearAllocDestroy(&arena);
  return JUST_TEST_PASS;
}

// =============================================================================
// MAIN ENTRY POINT
// =============================================================================

int main(void)
{
  // Group 0: Lifecycle & Capacity
  justTestRegister(testDynamicArrayCreateDestroy,    "DArray: Init, Sizing, and Destroy",       GROUP_DARRAY_BASIC);
  justTestRegister(testDynamicArrayReserveAndShrink, "DArray: Reserve Growth and ShrinkToFit",  GROUP_DARRAY_BASIC);

  // Group 1: Operations, Emplace & Ranges
  justTestRegister(testDynamicArrayPushAndPop,       "DArray: Push, Geometric Growth, and Pop", GROUP_DARRAY_OPS);
  justTestRegister(testDynamicArrayEmplaceStruct,    "DArray: Zero-Copy Struct Emplacement",    GROUP_DARRAY_OPS);
  justTestRegister(testDynamicArrayPushRange,        "DArray: Batch Range Memcpy Append",       GROUP_DARRAY_OPS);

  // Group 2: Arena Integration
  justTestRegister(testDynamicArrayArenaBacked,      "DArray: Arena Backing Allocation",        GROUP_DARRAY_ARENA);

  return (int)justTestRunAll();
}
