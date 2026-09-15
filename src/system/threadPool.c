#include <justUtils/system/threadPool.h>
#include <justUtils/core/asserts.h>
#include <justUtils/core/logger.h>
#include <justUtils/memory/tracker.h>

#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>

#define JUST_TP_DEFAULT_CAPACITY 1024

static int32_t workerThreadLoop(void* ARGUMENTS) 
{
  justThreadPool* pool = (justThreadPool*)ARGUMENTS;

  while (1) 
  {
    mtx_lock(&pool->lock);

    while (justQueueIsEmpty(&pool->taskQueue) && !pool->shutdown) 
    {
      cnd_wait(&pool->hasWork, &pool->lock);
    }

    if (pool->shutdown && justQueueIsEmpty(&pool->taskQueue)) 
    {
      mtx_unlock(&pool->lock);
      return 0; 
    }

    justTask task;
    justQueueDequeue(&pool->taskQueue, &task);
    pool->activeWorkers++;

    mtx_unlock(&pool->lock);

    if (task.func) 
    {
      task.func(task.arg);
    }

    mtx_lock(&pool->lock);
    pool->activeWorkers--;

    if (justQueueIsEmpty(&pool->taskQueue) && pool->activeWorkers == 0) 
    {
      cnd_broadcast(&pool->workingDone);
    }

    mtx_unlock(&pool->lock);
  }

  return 0;
}

JUST_API bool justThreadpoolCreate(
  justThreadPool*      POOL,
  size_t               THREAD_COUNT,
  size_t               QUEUE_CAPACITY,
  justLinearAllocator* ALLOCATOR,
  const char*          TAG)
{
  JUST_ASSERT_DEBUG_MESSAGE(POOL != NULL, "[THREAD POOL] : Target pool cannot be NULL");

  if (THREAD_COUNT == 0)
  {
    JUST_LOG_ERROR("[THREAD POOL] : threadCount cannot be 0! Explicit worker count is required.");
    return false;
  }

  if (QUEUE_CAPACITY == 0)
  {
    QUEUE_CAPACITY = JUST_TP_DEFAULT_CAPACITY;
  }

  POOL->threadCount   = 0;
  POOL->activeWorkers = 0;
  POOL->shutdown      = false;
  POOL->allocator     = ALLOCATOR;
  POOL->tag           = TAG;

  // - - - Initialize queue via justQueue
  if (!JUST_QUEUE_INIT_TAGGED(&POOL->taskQueue, QUEUE_CAPACITY, justTask, POOL->tag))
  {
    JUST_LOG_FATAL("[THREAD POOL] : Failed to initialize internal justQueue!");
    return false;
  }

  // - - - Allocate thread handles array
  size_t threadBytes = THREAD_COUNT * sizeof(thrd_t);
  if (POOL->allocator) 
  {
    POOL->threads = (thrd_t*)justLinearAllocAllocate(POOL->allocator, threadBytes, alignof(max_align_t));
  } 
  else 
  {
    POOL->threads = (thrd_t*)JUST_MALLOC_TAGGED(threadBytes, POOL->tag);
  }

  if (!POOL->threads) 
  {
    JUST_LOG_FATAL("[THREAD POOL] : Failed to allocate worker thread handles!");
    justQueueDestroy(&POOL->taskQueue);
    return false;
  }

  // - - - Initialize synchronization primitives
  if (mtx_init(&POOL->lock, mtx_plain) != thrd_success ||
      cnd_init(&POOL->hasWork) != thrd_success ||
      cnd_init(&POOL->workingDone) != thrd_success) 
  {
    JUST_LOG_FATAL("[THREAD POOL] : Failed to initialize C11 synchronization primitives!");
    if (!POOL->allocator) JUST_FREE(POOL->threads);
    justQueueDestroy(&POOL->taskQueue);
    return false;
  }

  // - - - Spawn worker threads 
  for (size_t i = 0; i < THREAD_COUNT; ++i) 
  {
    if (thrd_create(&POOL->threads[i], workerThreadLoop, POOL) != thrd_success) 
    {
      JUST_LOG_ERROR("[THREAD POOL] : Failed to spawn worker thread #%zu", i);
      justThreadpoolDestroy(POOL);
      return false;
    }
    POOL->threadCount++;
  }

  return true;
}

JUST_API bool justThreadpoolAddTask(
  justThreadPool*  POOL, 
  justTaskFunc     FUNC, 
  void*            ARG) 
{
  JUST_ASSERT_DEBUG_MESSAGE(POOL != NULL, "[THREAD POOL] : pool cannot be NULL");
  JUST_ASSERT_DEBUG_MESSAGE(FUNC != NULL, "[THREAD POOL] : task func cannot be NULL");

  mtx_lock(&POOL->lock);

  if (POOL->shutdown) 
  {
    mtx_unlock(&POOL->lock);
    return false;
  }

  justTask* slot = JUST_QUEUE_EMPLACE(&POOL->taskQueue, justTask);
  if (!slot) 
  {
    mtx_unlock(&POOL->lock);
    return false;
  }

  slot->func = FUNC;
  slot->arg  = ARG;

  cnd_signal(&POOL->hasWork);
  mtx_unlock(&POOL->lock);

  return true;
}

JUST_API void justThreadpoolWait(justThreadPool* POOL) 
{
  JUST_ASSERT_DEBUG_MESSAGE(POOL != NULL, "[THREAD POOL] : pool cannot be NULL");

  mtx_lock(&POOL->lock);
  while (!justQueueIsEmpty(&POOL->taskQueue) || POOL->activeWorkers > 0) 
  {
    cnd_wait(&POOL->workingDone, &POOL->lock);
  }
  mtx_unlock(&POOL->lock);
}

JUST_API void justThreadpoolDestroy(justThreadPool* POOL) 
{
  JUST_ASSERT_DEBUG_MESSAGE(POOL != NULL, "[THREAD POOL] : cannot destroy NULL POOL");

  mtx_lock(&POOL->lock);
  POOL->shutdown = true;
  cnd_broadcast(&POOL->hasWork);
  mtx_unlock(&POOL->lock);

  for (size_t i = 0; i < POOL->threadCount; ++i) 
  {
    thrd_join(POOL->threads[i], NULL);
  }

  mtx_destroy(&POOL->lock);
  cnd_destroy(&POOL->hasWork);
  cnd_destroy(&POOL->workingDone);

  justQueueDestroy(&POOL->taskQueue);

  if (!POOL->allocator && POOL->threads) 
  {
    JUST_FREE(POOL->threads);
  }

  POOL->threads     = NULL;
  POOL->threadCount = 0;
}

JUST_API size_t justThreadpoolPendingTasks(justThreadPool* POOL)
{
  JUST_ASSERT_DEBUG_MESSAGE(POOL != NULL, "[THREAD POOL] : cannot check pending path NULL POOL");

  mtx_lock(&POOL->lock);
  size_t count = justQueueSize(&POOL->taskQueue) + POOL->activeWorkers;
  mtx_unlock(&POOL->lock);

  return count;
}
