/**
 * @file testRingBuffer.c
 * @brief Unit test suite for JustRingBuffer.
 */

#include <justUtils/testing/testManager.h>
#include <justUtils/testing/expect.h>
#include <justUtils/dataStructures/ringBuffer.h>
#include <justUtils/memory/linearAlloc.h>
#include <justUtils/memory/tracker.h>
#include <stdint.h>
#include <string.h>

#define GROUP_RING_LIFECYCLE 0
#define GROUP_RING_FIFO      1
#define GROUP_RING_BLOCKING  2
#define GROUP_RING_OVERWRITE 3
#define GROUP_RING_MACROS    4
#define GROUP_RING_ARENA     5

typedef struct AudioPacket
{
  uint32_t channelId;
  float    amplitude;
} AudioPacket;

// =============================================================================
// 1. LIFECYCLE TESTS
// =============================================================================

static JustTestResult testRingBufferCreateDestroy(void)
{
  const char* tag = "RING_INIT";
  justMemorySetLimit(4096, tag);

  JustRingBuffer ring;
  bool ok = JUST_RING_INIT_TAGGED(&ring, 8, int32_t, false, tag);
  JUST_EXPECT_TO_BE_TRUE(ok);

  JUST_EXPECT_TO_BE((size_t)0, justRingBufferSize(&ring));
  JUST_EXPECT_TO_BE((size_t)8, justRingBufferCapacity(&ring));
  JUST_EXPECT_TO_BE((size_t)7, ring.mask);
  JUST_EXPECT_TO_BE_TRUE(justRingBufferIsEmpty(&ring));
  JUST_EXPECT_TO_BE_FALSE(justRingBufferIsFull(&ring));
  JUST_EXPECT_TO_BE_NOT_NULL(ring.data);
  JUST_EXPECT_TO_BE_NULL(justRingBufferPeek(&ring));

  justRingBufferDestroy(&ring);
  JUST_EXPECT_TO_BE_NULL(ring.data);
  JUST_EXPECT_TO_BE((size_t)0, justRingBufferSize(&ring));
  JUST_EXPECT_TO_BE_TRUE(justRingBufferIsEmpty(&ring));

  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 2. FIFO ORDERING & PEEK
// =============================================================================

static JustTestResult testRingBufferFIFOOrdering(void)
{
  const char* tag = "RING_FIFO";
  justMemorySetLimit(4096, tag);

  JustRingBuffer ring;
  JUST_RING_INIT_TAGGED(&ring, 8, int32_t, false, tag);

  for (int32_t i = 10; i <= 40; i += 10)
  {
    JUST_RING_PUSH_VAL(&ring, int32_t, i);
  }

  JUST_EXPECT_TO_BE((size_t)4, justRingBufferSize(&ring));
  JUST_EXPECT_TO_BE_FALSE(justRingBufferIsEmpty(&ring));
  JUST_EXPECT_TO_BE_FALSE(justRingBufferIsFull(&ring));

  int32_t* peeked = JUST_RING_PEEK(&ring, int32_t);
  JUST_EXPECT_TO_BE_NOT_NULL(peeked);
  JUST_EXPECT_TO_BE((int32_t)10, *peeked);

  int32_t outVal = 0;
  JUST_EXPECT_TO_BE_TRUE(justRingBufferPop(&ring, &outVal));
  JUST_EXPECT_TO_BE((int32_t)10, outVal);

  JUST_EXPECT_TO_BE_TRUE(justRingBufferPop(&ring, &outVal));
  JUST_EXPECT_TO_BE((int32_t)20, outVal);
  JUST_EXPECT_TO_BE((size_t)2, justRingBufferSize(&ring));

  peeked = JUST_RING_PEEK(&ring, int32_t);
  JUST_EXPECT_TO_BE_NOT_NULL(peeked);
  JUST_EXPECT_TO_BE((int32_t)30, *peeked);

  JUST_EXPECT_TO_BE_TRUE(justRingBufferPop(&ring, &outVal));
  JUST_EXPECT_TO_BE((int32_t)30, outVal);
  JUST_EXPECT_TO_BE_TRUE(justRingBufferPop(&ring, &outVal));
  JUST_EXPECT_TO_BE((int32_t)40, outVal);

  JUST_EXPECT_TO_BE_TRUE(justRingBufferIsEmpty(&ring));
  JUST_EXPECT_TO_BE_NULL(justRingBufferPeek(&ring));
  JUST_EXPECT_TO_BE_FALSE(justRingBufferPop(&ring, &outVal));

  justRingBufferDestroy(&ring);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 3. NO-OVERWRITE (BLOCKING) SATURATION
// =============================================================================

static JustTestResult testRingBufferNoOverwriteSaturation(void)
{
  const char* tag = "RING_BLOCK";
  justMemorySetLimit(4096, tag);

  JustRingBuffer ring;
  // allowOverwrite = false
  JUST_RING_INIT_TAGGED(&ring, 4, uint32_t, false, tag);

  for (uint32_t i = 1; i <= 4; ++i)
  {
    bool pushed = justRingBufferPush(&ring, &i);
    JUST_EXPECT_TO_BE_TRUE(pushed);
  }

  JUST_EXPECT_TO_BE((size_t)4, justRingBufferSize(&ring));
  JUST_EXPECT_TO_BE_TRUE(justRingBufferIsFull(&ring));

  // Fifth push must fail without modifying buffer
  uint32_t overflowVal = 99;
  bool pushedOverflow = justRingBufferPush(&ring, &overflowVal);
  JUST_EXPECT_TO_BE_FALSE(pushedOverflow);
  JUST_EXPECT_TO_BE_NULL(justRingBufferEmplace(&ring));
  JUST_EXPECT_TO_BE((size_t)4, justRingBufferSize(&ring));

  // Pop one element to free up a slot
  uint32_t outVal = 0;
  JUST_EXPECT_TO_BE_TRUE(justRingBufferPop(&ring, &outVal));
  JUST_EXPECT_TO_BE((uint32_t)1, outVal);
  JUST_EXPECT_TO_BE_FALSE(justRingBufferIsFull(&ring));

  // Push should now succeed
  uint32_t validVal = 5;
  JUST_EXPECT_TO_BE_TRUE(justRingBufferPush(&ring, &validVal));
  JUST_EXPECT_TO_BE_TRUE(justRingBufferIsFull(&ring));

  // Drain remaining items: 2, 3, 4, 5
  uint32_t expectedSequence[] = { 2, 3, 4, 5 };
  for (size_t i = 0; i < 4; ++i)
  {
    JUST_EXPECT_TO_BE_TRUE(justRingBufferPop(&ring, &outVal));
    JUST_EXPECT_TO_BE(expectedSequence[i], outVal);
  }

  JUST_EXPECT_TO_BE_TRUE(justRingBufferIsEmpty(&ring));

  justRingBufferDestroy(&ring);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 4. OVERWRITE (DROP OLDEST) WRAP-AROUND
// =============================================================================

static JustTestResult testRingBufferOverwriteDropOldest(void)
{
  const char* tag = "RING_OVERWRITE";
  justMemorySetLimit(4096, tag);

  JustRingBuffer ring;
  // allowOverwrite = true
  JUST_RING_INIT_TAGGED(&ring, 4, int32_t, true, tag);

  // Fill buffer: [1, 2, 3, 4]
  for (int32_t i = 1; i <= 4; ++i)
  {
    JUST_RING_PUSH_VAL(&ring, int32_t, i);
  }

  JUST_EXPECT_TO_BE((size_t)4, justRingBufferSize(&ring));
  JUST_EXPECT_TO_BE_TRUE(justRingBufferIsFull(&ring));

  // Pushing 5 drops 1: window is now [2, 3, 4, 5]
  JUST_RING_PUSH_VAL(&ring, int32_t, 5);
  JUST_EXPECT_TO_BE((size_t)4, justRingBufferSize(&ring));
  JUST_EXPECT_TO_BE_TRUE(justRingBufferIsFull(&ring));
  JUST_EXPECT_TO_BE((int32_t)2, *JUST_RING_PEEK(&ring, int32_t));

  // Pushing 6 drops 2: window is now [3, 4, 5, 6]
  JUST_RING_PUSH_VAL(&ring, int32_t, 6);
  JUST_EXPECT_TO_BE((size_t)4, justRingBufferSize(&ring));
  JUST_EXPECT_TO_BE_TRUE(justRingBufferIsFull(&ring));
  JUST_EXPECT_TO_BE((int32_t)3, *JUST_RING_PEEK(&ring, int32_t));

  // Drain and verify oldest dropped items are gone
  int32_t expectedSequence[] = { 3, 4, 5, 6 };
  for (size_t i = 0; i < 4; ++i)
  {
    int32_t outVal = 0;
    JUST_EXPECT_TO_BE_TRUE(justRingBufferPop(&ring, &outVal));
    JUST_EXPECT_TO_BE(expectedSequence[i], outVal);
  }

  JUST_EXPECT_TO_BE_TRUE(justRingBufferIsEmpty(&ring));

  justRingBufferDestroy(&ring);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 5. EMPLACE MACROS & CLEAR
// =============================================================================

static JustTestResult testRingBufferEmplaceAndClear(void)
{
  const char* tag = "RING_EMPLACE";
  justMemorySetLimit(4096, tag);

  JustRingBuffer ring;
  JUST_RING_INIT_TAGGED(&ring, 4, AudioPacket, false, tag);

  AudioPacket* p1 = JUST_RING_EMPLACE(&ring, AudioPacket);
  JUST_EXPECT_TO_BE_NOT_NULL(p1);
  p1->channelId = 1;
  p1->amplitude = 0.75f;

  AudioPacket* p2 = JUST_RING_EMPLACE(&ring, AudioPacket);
  JUST_EXPECT_TO_BE_NOT_NULL(p2);
  p2->channelId = 2;
  p2->amplitude = 0.90f;

  JUST_EXPECT_TO_BE((size_t)2, justRingBufferSize(&ring));

  AudioPacket* peekedPacket = JUST_RING_PEEK(&ring, AudioPacket);
  JUST_EXPECT_TO_BE_NOT_NULL(peekedPacket);
  JUST_EXPECT_TO_BE((uint32_t)1, peekedPacket->channelId);
  JUST_EXPECT_FLOAT_TO_BE(0.75f, peekedPacket->amplitude, 0.0001f);

  // Clear without releasing memory
  justRingBufferClear(&ring);
  JUST_EXPECT_TO_BE((size_t)0, justRingBufferSize(&ring));
  JUST_EXPECT_TO_BE_TRUE(justRingBufferIsEmpty(&ring));
  JUST_EXPECT_TO_BE_FALSE(justRingBufferIsFull(&ring));
  JUST_EXPECT_TO_BE_NULL(justRingBufferPeek(&ring));
  JUST_EXPECT_TO_BE((size_t)4, justRingBufferCapacity(&ring));

  justRingBufferDestroy(&ring);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 6. ARENA (LINEAR ALLOCATOR) BACKING
// =============================================================================

static JustTestResult testRingBufferArenaBacked(void)
{
  const char* tag = "RING_ARENA";
  justMemorySetLimit(4096, tag);

  JustLinearAllocator arena;
  bool arenaOk = justLinearAllocCreate(&arena, 1024, NULL, tag);
  JUST_EXPECT_TO_BE_TRUE(arenaOk);

  JustRingBuffer ring;
  bool ringOk = justRingBufferCreate(&ring, 8, sizeof(uint64_t), true, &arena, tag);
  JUST_EXPECT_TO_BE_TRUE(ringOk);

  for (uint64_t i = 1; i <= 6; ++i)
  {
    JUST_RING_PUSH_VAL(&ring, uint64_t, i * 100);
  }
  JUST_EXPECT_TO_BE((size_t)6, justRingBufferSize(&ring));

  uint64_t val = 0;
  for (uint64_t i = 1; i <= 6; ++i)
  {
    JUST_EXPECT_TO_BE_TRUE(justRingBufferPop(&ring, &val));
    JUST_EXPECT_TO_BE((uint64_t)(i * 100), val);
  }

  // Destroying arena-backed ring buffer cleans state without calling free()
  justRingBufferDestroy(&ring);
  JUST_EXPECT_TO_BE_NULL(ring.data);
  JUST_EXPECT_TO_BE_TRUE(justMemoryCheckBounds());

  justLinearAllocDestroy(&arena);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// MAIN ENTRY POINT
// =============================================================================

int main(void)
{
  // Group 0: Lifecycle
  justTestRegister(testRingBufferCreateDestroy,          "RingBuffer: Init, Sizing, and Destroy",      GROUP_RING_LIFECYCLE);

  // Group 1: FIFO Ordering
  justTestRegister(testRingBufferFIFOOrdering,           "RingBuffer: FIFO Push, Pop, and Peek",       GROUP_RING_FIFO);

  // Group 2: Blocking Saturation
  justTestRegister(testRingBufferNoOverwriteSaturation,  "RingBuffer: No-Overwrite Full Rejection",    GROUP_RING_BLOCKING);

  // Group 3: Overwrite Drop-Oldest
  justTestRegister(testRingBufferOverwriteDropOldest,    "RingBuffer: Overwrite Drop-Oldest Sliding",  GROUP_RING_OVERWRITE);

  // Group 4: Emplace & Clear
  justTestRegister(testRingBufferEmplaceAndClear,        "RingBuffer: Zero-Copy Emplace and Clear",    GROUP_RING_MACROS);

  // Group 5: Arena Backing
  justTestRegister(testRingBufferArenaBacked,            "RingBuffer: Linear Allocator Arena Backing", GROUP_RING_ARENA);

  return (int)justTestRunAll();
}
