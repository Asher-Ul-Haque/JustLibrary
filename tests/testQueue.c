/**
 * @file testQueue.c
 * @brief Unit test suite for JustQueue.
 */

#include <justUtils/testing/testManager.h>
#include <justUtils/testing/expect.h>
#include <justUtils/dataStructures/queue.h>
#include <justUtils/memory/linearAlloc.h>
#include <justUtils/memory/tracker.h>
#include <stdint.h>
#include <string.h>

#define GROUP_QUEUE_LIFECYCLE 0
#define GROUP_QUEUE_FIFO      1
#define GROUP_QUEUE_WRAP      2
#define GROUP_QUEUE_GROWTH    3
#define GROUP_QUEUE_EMPLACE   4
#define GROUP_QUEUE_ARENA     5

typedef struct QueueJob
{
  uint32_t id;
  uint64_t payload;
} QueueJob;

// =============================================================================
// 1. LIFECYCLE TESTS
// =============================================================================

static JustTestResult testQueueCreateDestroy(void)
{
  const char* tag = "Q_INIT";
  justMemorySetLimit(4096, tag);

  JustQueue q;
  bool ok = JUST_QUEUE_INIT_TAGGED(&q, 8, int32_t, tag);
  JUST_EXPECT_TO_BE_TRUE(ok);

  JUST_EXPECT_TO_BE((size_t)0, justQueueSize(&q));
  JUST_EXPECT_TO_BE((size_t)8, justQueueCapacity(&q));
  JUST_EXPECT_TO_BE((size_t)7, q.mask);
  JUST_EXPECT_TO_BE_TRUE(justQueueIsEmpty(&q));
  JUST_EXPECT_TO_BE_NOT_NULL(q.data);
  JUST_EXPECT_TO_BE_NULL(justQueuePeek(&q));

  justQueueDestroy(&q);
  JUST_EXPECT_TO_BE_NULL(q.data);
  JUST_EXPECT_TO_BE((size_t)0, justQueueSize(&q));

  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 2. FIFO ORDERING & PEEK
// =============================================================================

static JustTestResult testQueueFIFOOrdering(void)
{
  const char* tag = "Q_FIFO";
  justMemorySetLimit(4096, tag);

  JustQueue q;
  JUST_QUEUE_INIT_TAGGED(&q, 8, int32_t, tag);

  // Enqueue 4 items
  for (int32_t i = 10; i <= 40; i += 10)
  {
    JUST_QUEUE_ENQUEUE_VAL(&q, int32_t, i);
  }

  JUST_EXPECT_TO_BE((size_t)4, justQueueSize(&q));
  JUST_EXPECT_TO_BE_FALSE(justQueueIsEmpty(&q));

  // Peek must return oldest element (10) without dequeuing
  int32_t* peeked = JUST_QUEUE_PEEK(&q, int32_t);
  JUST_EXPECT_TO_BE_NOT_NULL(peeked);
  JUST_EXPECT_TO_BE((int32_t)10, *peeked);
  JUST_EXPECT_TO_BE((size_t)4, justQueueSize(&q));

  // Dequeue first two
  int32_t val = 0;
  JUST_EXPECT_TO_BE_TRUE(justQueueDequeue(&q, &val));
  JUST_EXPECT_TO_BE((int32_t)10, val);
  JUST_EXPECT_TO_BE_TRUE(justQueueDequeue(&q, &val));
  JUST_EXPECT_TO_BE((int32_t)20, val);
  JUST_EXPECT_TO_BE((size_t)2, justQueueSize(&q));

  // Peek updated head
  peeked = JUST_QUEUE_PEEK(&q, int32_t);
  JUST_EXPECT_TO_BE_NOT_NULL(peeked);
  JUST_EXPECT_TO_BE((int32_t)30, *peeked);

  // Dequeue remaining
  JUST_EXPECT_TO_BE_TRUE(justQueueDequeue(&q, &val));
  JUST_EXPECT_TO_BE((int32_t)30, val);
  JUST_EXPECT_TO_BE_TRUE(justQueueDequeue(&q, &val));
  JUST_EXPECT_TO_BE((int32_t)40, val);
  JUST_EXPECT_TO_BE_TRUE(justQueueIsEmpty(&q));

  // Underflow check: dequeue from empty queue
  JUST_EXPECT_TO_BE_FALSE(justQueueDequeue(&q, &val));

  justQueueDestroy(&q);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 3. RING BUFFER WRAP-AROUND
// =============================================================================

static JustTestResult testQueueWrapAround(void)
{
  const char* tag = "Q_WRAP";
  justMemorySetLimit(4096, tag);

  JustQueue q;
  // Minimum capacity enforced by justQueueRoundToPowerOfTwo is 8
  JUST_QUEUE_INIT_TAGGED(&q, 8, uint32_t, tag);
  JUST_EXPECT_TO_BE((size_t)8, justQueueCapacity(&q));

  // Fill capacity: slots 0 through 7
  for (uint32_t i = 1; i <= 8; ++i)
  {
    JUST_QUEUE_ENQUEUE_VAL(&q, uint32_t, i);
  }
  JUST_EXPECT_TO_BE((size_t)8, justQueueSize(&q));

  // Pop 4 items (head advances to index 4)
  uint32_t out = 0;
  for (uint32_t i = 1; i <= 4; ++i)
  {
    JUST_EXPECT_TO_BE_TRUE(justQueueDequeue(&q, &out));
    JUST_EXPECT_TO_BE(i, out);
  }
  JUST_EXPECT_TO_BE((size_t)4, justQueueSize(&q));

  // Push 4 items: these wrap around to slots 0, 1, 2, 3
  for (uint32_t i = 9; i <= 12; ++i)
  {
    JUST_QUEUE_ENQUEUE_VAL(&q, uint32_t, i);
  }
  JUST_EXPECT_TO_BE((size_t)8, justQueueSize(&q));
  JUST_EXPECT_TO_BE((size_t)8, justQueueCapacity(&q)); // Must not grow

  // Drain and verify order preserved: 5, 6, 7, 8, 9, 10, 11, 12
  for (uint32_t i = 5; i <= 12; ++i)
  {
    JUST_EXPECT_TO_BE_TRUE(justQueueDequeue(&q, &out));
    JUST_EXPECT_TO_BE(i, out);
  }

  JUST_EXPECT_TO_BE_TRUE(justQueueIsEmpty(&q));

  justQueueDestroy(&q);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 4. DYNAMIC GROWTH & BUFFER UNWRAPPING
// =============================================================================

static JustTestResult testQueueGrowthUnwrapping(void)
{
  const char* tag = "Q_GROWTH";
  justMemorySetLimit(8192, tag);

  JustQueue q;
  // Initial capacity = 4
  JUST_QUEUE_INIT_TAGGED(&q, 4, int32_t, tag);

  // Step 1: Push 3 items
  JUST_QUEUE_ENQUEUE_VAL(&q, int32_t, 10);
  JUST_QUEUE_ENQUEUE_VAL(&q, int32_t, 20);
  JUST_QUEUE_ENQUEUE_VAL(&q, int32_t, 30);

  // Step 2: Pop 2 items (head is now at index 2)
  int32_t val = 0;
  justQueueDequeue(&q, &val); // 10
  justQueueDequeue(&q, &val); // 20

  // Step 3: Push 3 items to wrap around and fill buffer
  JUST_QUEUE_ENQUEUE_VAL(&q, int32_t, 40); // index 3
  JUST_QUEUE_ENQUEUE_VAL(&q, int32_t, 50); // index 0 (wrap)
  JUST_QUEUE_ENQUEUE_VAL(&q, int32_t, 60); // index 1 (wrap)
  JUST_EXPECT_TO_BE((size_t)4, justQueueSize(&q));

  // Step 4: Trigger growth on wrapped buffer (head = 2, tail = 2)
  // Queue must unwrap correctly: [30, 40, 50, 60, 70]
  JUST_QUEUE_ENQUEUE_VAL(&q, int32_t, 70);
  JUST_EXPECT_TO_BE((size_t)5, justQueueSize(&q));
  JUST_EXPECT_TO_BE_TRUE(justQueueCapacity(&q) >= 8);

  // Step 5: Drain queue and verify sequence was not corrupted by realloc
  int32_t expectedSequence[] = { 30, 40, 50, 60, 70 };
  for (size_t i = 0; i < 5; ++i)
  {
    JUST_EXPECT_TO_BE_TRUE(justQueueDequeue(&q, &val));
    JUST_EXPECT_TO_BE(expectedSequence[i], val);
  }

  JUST_EXPECT_TO_BE_TRUE(justQueueIsEmpty(&q));

  justQueueDestroy(&q);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 5. ZERO-COPY EMPLACEMENT & CLEAR
// =============================================================================

static JustTestResult testQueueEmplaceAndClear(void)
{
  const char* tag = "Q_EMPLACE";
  justMemorySetLimit(4096, tag);

  JustQueue q;
  JUST_QUEUE_INIT_TAGGED(&q, 8, QueueJob, tag);
  JUST_EXPECT_TO_BE((size_t)8, justQueueCapacity(&q));

  // Emplace 2 elements directly into internal slots
  QueueJob* job1 = JUST_QUEUE_EMPLACE(&q, QueueJob);
  JUST_EXPECT_TO_BE_NOT_NULL(job1);
  job1->id = 101;
  job1->payload = 0xAAAAAAAAAAAAAAAAULL;

  QueueJob* job2 = JUST_QUEUE_EMPLACE(&q, QueueJob);
  JUST_EXPECT_TO_BE_NOT_NULL(job2);
  job2->id = 102;
  job2->payload = 0xBBBBBBBBBBBBBBBBULL;

  JUST_EXPECT_TO_BE((size_t)2, justQueueSize(&q));

  // Peek first job
  QueueJob* peekJob = JUST_QUEUE_PEEK(&q, QueueJob);
  JUST_EXPECT_TO_BE_NOT_NULL(peekJob);
  JUST_EXPECT_TO_BE((uint32_t)101, peekJob->id);
  JUST_EXPECT_TO_BE((uint64_t)0xAAAAAAAAAAAAAAAAULL, peekJob->payload);

  // Clear resets counters without freeing backing buffer
  justQueueClear(&q);
  JUST_EXPECT_TO_BE((size_t)0, justQueueSize(&q));
  JUST_EXPECT_TO_BE_TRUE(justQueueIsEmpty(&q));
  JUST_EXPECT_TO_BE((size_t)8, justQueueCapacity(&q));
  JUST_EXPECT_TO_BE_NULL(justQueuePeek(&q));

  justQueueDestroy(&q);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 6. ARENA (LINEAR ALLOCATOR) BACKING
// =============================================================================

static JustTestResult testQueueArenaBacked(void)
{
  const char* tag = "Q_ARENA";
  justMemorySetLimit(4096, tag);

  JustLinearAllocator arena;
  bool arenaOk = justLinearAllocCreate(&arena, 1024, NULL, tag);
  JUST_EXPECT_TO_BE_TRUE(arenaOk);

  JustQueue q;
  bool qOk = justQueueCreate(&q, 8, sizeof(uint64_t), &arena, tag);
  JUST_EXPECT_TO_BE_TRUE(qOk);

  for (uint64_t i = 1; i <= 6; ++i)
  {
    JUST_QUEUE_ENQUEUE_VAL(&q, uint64_t, i * 111);
  }
  JUST_EXPECT_TO_BE((size_t)6, justQueueSize(&q));

  uint64_t val = 0;
  for (uint64_t i = 1; i <= 6; ++i)
  {
    JUST_EXPECT_TO_BE_TRUE(justQueueDequeue(&q, &val));
    JUST_EXPECT_TO_BE((uint64_t)(i * 111), val);
  }

  // Destroying arena-backed queue must not call system free()
  justQueueDestroy(&q);
  JUST_EXPECT_TO_BE_NULL(q.data);
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
  justTestRegister(testQueueCreateDestroy,     "Queue: Init, Sizing, and Destroy",         GROUP_QUEUE_LIFECYCLE);

  // Group 1: FIFO Ordering & Peek
  justTestRegister(testQueueFIFOOrdering,      "Queue: FIFO Enqueue, Dequeue, and Peek",   GROUP_QUEUE_FIFO);

  // Group 2: Wrap-Around
  justTestRegister(testQueueWrapAround,        "Queue: Circular Ring Buffer Wrap-Around",  GROUP_QUEUE_WRAP);

  // Group 3: Dynamic Growth
  justTestRegister(testQueueGrowthUnwrapping,  "Queue: Resizing and Unwrapping on Growth", GROUP_QUEUE_GROWTH);

  // Group 4: Emplace & Clear
  justTestRegister(testQueueEmplaceAndClear,   "Queue: Zero-Copy Struct Emplace and Clear",GROUP_QUEUE_EMPLACE);

  // Group 5: Arena Backing
  justTestRegister(testQueueArenaBacked,       "Queue: Linear Allocator Arena Backing",    GROUP_QUEUE_ARENA);

  return (int)justTestRunAll();
}
