/**
 * @file testThreadPool.c
 * @brief Unit test suite for C11 thread pool.
 */

#include <justUtils/testing/testManager.h>
#include <justUtils/testing/expect.h>
#include <justUtils/system/threadPool.h>
#include <justUtils/memory/linearAlloc.h>
#include <justUtils/memory/tracker.h>
#include <stdatomic.h>
#include <stdint.h>

#define GROUP_TP_LIFECYCLE 0
#define GROUP_TP_TASKS     1
#define GROUP_TP_STRESS    2
#define GROUP_TP_ARENA     3

// Simple task context for counting executions
typedef struct TaskData
{
  atomic_size_t* counter;
  size_t         incrementValue;
} TaskData;

static void executeTask(void* arg)
{
  TaskData* data = (TaskData*)arg;
  atomic_fetch_add(data->counter, data->incrementValue);
}

// =============================================================================
// 1. LIFECYCLE TESTS
// =============================================================================

static JustTestResult testThreadPoolCreateDestroy(void)
{
  const char* tag = "TP_LIFECYCLE";
  justMemorySetLimit(8192, tag);
  justMemorySetLimit(8192, "TP_QUEUE");

  justThreadPool pool;
  // 4 workers, 16 capacity
  bool ok = justThreadpoolCreate(&pool, 4, 16, NULL, tag);
  JUST_EXPECT_TO_BE_TRUE(ok);

  JUST_EXPECT_TO_BE((size_t)4, pool.threadCount);
  JUST_EXPECT_TO_BE((size_t)0, justThreadpoolPendingTasks(&pool));
  JUST_EXPECT_TO_BE_FALSE(pool.shutdown);

  // Waiting on empty pool should return immediately
  justThreadpoolWait(&pool);

  justThreadpoolDestroy(&pool);
  JUST_EXPECT_TO_BE_NULL(pool.threads);
  JUST_EXPECT_TO_BE((size_t)0, pool.threadCount);

  justMemorySetLimit(0, tag);
  justMemorySetLimit(0, "TP_QUEUE");
  return JUST_TEST_PASS;
}

// =============================================================================
// 2. DISPATCH, EXECUTION, AND WAIT
// =============================================================================

static JustTestResult testThreadPoolExecution(void)
{
  const char* tag = "TP_TASKS";
  justMemorySetLimit(8192, tag);
  justMemorySetLimit(8192, "TP_QUEUE");

  justThreadPool pool;
  justThreadpoolCreate(&pool, 4, 32, NULL, tag);

  atomic_size_t counter = 0;
  TaskData tasks[16];

  for (size_t i = 0; i < 16; ++i)
  {
    tasks[i].counter = &counter;
    tasks[i].incrementValue = 1;
    bool added = justThreadpoolAddTask(&pool, executeTask, &tasks[i]);
    JUST_EXPECT_TO_BE_TRUE(added);
  }

  // Block until all 16 tasks complete
  justThreadpoolWait(&pool);

  JUST_EXPECT_TO_BE((size_t)16, atomic_load(&counter));
  JUST_EXPECT_TO_BE((size_t)0, justThreadpoolPendingTasks(&pool));

  justThreadpoolDestroy(&pool);
  justMemorySetLimit(0, tag);
  justMemorySetLimit(0, "TP_QUEUE");
  return JUST_TEST_PASS;
}

// =============================================================================
// 3. CONCURRENCY STRESS TEST
// =============================================================================

static JustTestResult testThreadPoolStressBatch(void)
{
  const char* tag = "TP_STRESS";
  justMemorySetLimit(65536, tag);
  justMemorySetLimit(65536, "TP_QUEUE");

  justThreadPool pool;
  // 8 worker threads, 256 task queue slots
  justThreadpoolCreate(&pool, 8, 256, NULL, tag);

  atomic_size_t sum = 0;
  const size_t totalTasks = 200;
  TaskData tasks[200];

  for (size_t i = 0; i < totalTasks; ++i)
  {
    tasks[i].counter = &sum;
    tasks[i].incrementValue = i + 1; // Sum of 1..200 = 20100
    while (!justThreadpoolAddTask(&pool, executeTask, &tasks[i]))
    {
      // If queue is full, yield/wait momentarily
      thrd_yield();
    }
  }

  justThreadpoolWait(&pool);

  // Expected sum: N * (N + 1) / 2 = 200 * 201 / 2 = 20100
  JUST_EXPECT_TO_BE((size_t)20100, atomic_load(&sum));
  JUST_EXPECT_TO_BE((size_t)0, justThreadpoolPendingTasks(&pool));

  justThreadpoolDestroy(&pool);
  justMemorySetLimit(0, tag);
  justMemorySetLimit(0, "TP_QUEUE");
  return JUST_TEST_PASS;
}

// =============================================================================
// 4. ARENA (LINEAR ALLOCATOR) BACKING
// =============================================================================

static JustTestResult testThreadPoolArenaBacked(void)
{
  const char* tag = "TP_ARENA";
  justMemorySetLimit(32768, tag);

  justLinearAllocator arena;
  bool arenaOk = justLinearAllocCreate(&arena, 16384, NULL, tag);
  JUST_EXPECT_TO_BE_TRUE(arenaOk);

  justThreadPool pool;
  bool poolOk = justThreadpoolCreate(&pool, 2, 32, &arena, tag);
  JUST_EXPECT_TO_BE_TRUE(poolOk);

  atomic_size_t counter = 0;
  TaskData tasks[8];
  for (size_t i = 0; i < 8; ++i)
  {
    tasks[i].counter = &counter;
    tasks[i].incrementValue = 10;
    justThreadpoolAddTask(&pool, executeTask, &tasks[i]);
  }

  justThreadpoolWait(&pool);
  JUST_EXPECT_TO_BE((size_t)80, atomic_load(&counter));

  // Destroying arena-backed thread pool joins threads without freeing from system heap
  justThreadpoolDestroy(&pool);
  JUST_EXPECT_TO_BE_NULL(pool.threads);
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
  justTestRegister(testThreadPoolCreateDestroy, "ThreadPool: Init, Sizing, and Destruction", GROUP_TP_LIFECYCLE);
  justTestRegister(testThreadPoolExecution,     "ThreadPool: Task Dispatch, Exec, and Wait", GROUP_TP_TASKS);
  justTestRegister(testThreadPoolStressBatch,   "ThreadPool: High Concurrency Math Stress",  GROUP_TP_STRESS);
  justTestRegister(testThreadPoolArenaBacked,   "ThreadPool: Linear Allocator Arena Backing",GROUP_TP_ARENA);

  return (int)justTestRunAll();
}
