/*
 * Copyright (C) [2026] [Asher-Ul-Haque aka Just Somebody]
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/**
 * @file : threadPool.h 
 * @brief : Pure worker thread pool using <threads.h>
 */

#pragma once 

#include <justUtils/defines.h>
#include <justUtils/dataStructures/queue.h>
#include <justUtils/memory/linearAlloc.h>
#include <threads.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __STDC_NO_THREADS__
  #error "Target C compiler/platform does not support C11 <threads.h>"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Task callback function signature
typedef void (*justTaskFunc)(void* ARG);

/// @brief Task representation
typedef struct justTask 
{
  justTaskFunc func; ///< Task function to execute
  void*         arg;  ///< Argument passed to func
} justTask;

/// @brief : Thread pool representation
typedef struct justThreadPool 
{
  thrd_t*               threads;        ///< Standard C11 worker thread handles
  size_t                threadCount;    ///< Number of spawned worker threads
  
  JustQueue            taskQueue;      ///< Task queue
  
  mtx_t                 lock;           ///< C11 mutex protecting queue state
  cnd_t                 hasWork;        ///< Signaled when tasks are pushed
  cnd_t                 workingDone;    ///< Signaled when all tasks complete
  
  size_t                activeWorkers;  ///< Workers currently executing a task
  bool                  shutdown;       ///< Shutdown flag
  const char*           tag;            ///< Memory tracking tag
  justLinearAllocator* allocator;      ///< Optional linear allocator
} justThreadPool;

/**
 * @brief Creates a worker thread pool using C11 threads.
 * @param POOL Pointer to ThreadPool struct.
 * @param THREAD_COUNT Number of worker threads.
 * @param QUEUE_CAPACITY Max pending tasks (rounded to power-of-two).
 * @param ALLOCATOR Optional linear allocator (NULL for heap).
 * @param TAG Tracking tag for memory tracker.
 * @return true on success, false on failure.
 */
JUST_API bool justThreadpoolCreate(
  justThreadPool*      POOL,
  size_t               THREAD_COUNT,
  size_t               QUEUE_CAPACITY,
  justLinearAllocator* ALLOCATOR,
  const char*          TAG);

/**
 * @brief Submits a task to the queue.
 * @return true if enqueued, false if queue is full or pool is shutting down.
 */
JUST_API bool justThreadpoolAddTask(
  justThreadPool* POOL, 
  justTaskFunc    FUNC, 
  void*           ARG);

/**
 * @brief : Blocks calling thread until all currently queued and active tasks complete.
 * @param POOL : The pointer to the POOL
 */
JUST_API void justThreadpoolWait(justThreadPool* POOL);

/**
 * @brief : Gracefully terminates the thread pool and joins all worker threads.
 * @param POOL : Pointer of the pool
 */
JUST_API void justThreadpoolDestroy(justThreadPool* POOL);

/**
 * @brief : Returns total tasks currently pending or executing.
 * @param POOL : Pointer to the pool
 * @return : how many tasks remaining
 */
JUST_API size_t justThreadpoolPendingTasks(justThreadPool* POOL);


// - - - Ergonomic Macros - - -

#define JUST_THREAD_POOL_INIT_TAGGED(POOL, THREAD_COUNT, TAG) \
  justThreadpoolCreate((POOL_PTR), (THREAD_COUNT), (0), NULL, (TAG))

#define JUST_THREAD_POOL_INIT(POOL, THREAD_COUNT) JUST_THREAD_POOL_INIT_TAGGED(POOL, THREAD_COUNT, "THREAD POOL")

#ifdef __cplusplus
}
#endif
