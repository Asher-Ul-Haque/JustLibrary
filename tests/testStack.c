/**
 * @file testStack.c
 * @brief Unit test suite for JustStack.
 */

#include <justUtils/testing/testManager.h>
#include <justUtils/testing/expect.h>
#include <justUtils/dataStructures/stack.h>
#include <justUtils/memory/linearAlloc.h>
#include <justUtils/memory/tracker.h>
#include <stdint.h>
#include <string.h>

#define GROUP_STACK_LIFECYCLE 0
#define GROUP_STACK_LIFO      1
#define GROUP_STACK_MACROS    2
#define GROUP_STACK_ARENA     3

typedef struct StackFrame
{
  uint32_t frameId;
  uint64_t returnAddress;
} StackFrame;

// =============================================================================
// 1. LIFECYCLE TESTS
// =============================================================================

static JustTestResult testStackCreateDestroy(void)
{
  const char* tag = "STACK_INIT";
  justMemorySetLimit(4096, tag);

  JustStack stack;
  bool ok = JUST_STACK_INIT_TAGGED(&stack, 4, int32_t, tag);
  JUST_EXPECT_TO_BE_TRUE(ok);

  JUST_EXPECT_TO_BE((size_t)0, justStackSize(&stack));
  JUST_EXPECT_TO_BE_TRUE(justStackIsEmpty(&stack));
  JUST_EXPECT_TO_BE_NULL(justStackPeek(&stack));

  justStackDestroy(&stack);
  JUST_EXPECT_TO_BE((size_t)0, justStackSize(&stack));
  JUST_EXPECT_TO_BE_TRUE(justStackIsEmpty(&stack));

  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 2. LIFO ORDERING & POP/PEEK
// =============================================================================

static JustTestResult testStackLIFOOrdering(void)
{
  const char* tag = "STACK_LIFO";
  justMemorySetLimit(4096, tag);

  JustStack stack;
  JUST_STACK_INIT_TAGGED(&stack, 4, int32_t, tag);

  // Push values: 10, 20, 30
  int32_t v1 = 10, v2 = 20, v3 = 30;
  JUST_EXPECT_TO_BE_TRUE(justStackPush(&stack, &v1));
  JUST_EXPECT_TO_BE_TRUE(justStackPush(&stack, &v2));
  JUST_EXPECT_TO_BE_TRUE(justStackPush(&stack, &v3));

  JUST_EXPECT_TO_BE((size_t)3, justStackSize(&stack));
  JUST_EXPECT_TO_BE_FALSE(justStackIsEmpty(&stack));

  // Peek top element (must be 30)
  int32_t* topVal = (int32_t*)justStackPeek(&stack);
  JUST_EXPECT_TO_BE_NOT_NULL(topVal);
  JUST_EXPECT_TO_BE((int32_t)30, *topVal);
  JUST_EXPECT_TO_BE((size_t)3, justStackSize(&stack));

  // Pop in LIFO sequence: 30, then 20, then 10
  int32_t popped = 0;
  JUST_EXPECT_TO_BE_TRUE(justStackPop(&stack, &popped));
  JUST_EXPECT_TO_BE((int32_t)30, popped);
  JUST_EXPECT_TO_BE((size_t)2, justStackSize(&stack));

  JUST_EXPECT_TO_BE_TRUE(justStackPop(&stack, &popped));
  JUST_EXPECT_TO_BE((int32_t)20, popped);

  JUST_EXPECT_TO_BE_TRUE(justStackPop(&stack, &popped));
  JUST_EXPECT_TO_BE((int32_t)10, popped);

  JUST_EXPECT_TO_BE((size_t)0, justStackSize(&stack));
  JUST_EXPECT_TO_BE_TRUE(justStackIsEmpty(&stack));
  JUST_EXPECT_TO_BE_NULL(justStackPeek(&stack));

  // Underflow safety check
  JUST_EXPECT_TO_BE_FALSE(justStackPop(&stack, &popped));

  justStackDestroy(&stack);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 3. TYPED MACROS & ZERO-COPY EMPLACE
// =============================================================================

static JustTestResult testStackMacrosAndEmplace(void)
{
  const char* tag = "STACK_MACRO";
  justMemorySetLimit(8192, tag);

  JustStack stack;
  JUST_STACK_INIT_TAGGED(&stack, 2, StackFrame, tag);

  // Zero-copy emplacement
  StackFrame* f1 = JUST_STACK_EMPLACE(&stack, StackFrame);
  JUST_EXPECT_TO_BE_NOT_NULL(f1);
  f1->frameId = 1;
  f1->returnAddress = 0x1000ULL;

  StackFrame* f2 = JUST_STACK_EMPLACE(&stack, StackFrame);
  JUST_EXPECT_TO_BE_NOT_NULL(f2);
  f2->frameId = 2;
  f2->returnAddress = 0x2000ULL;

  JUST_EXPECT_TO_BE((size_t)2, justStackSize(&stack));

  // Inspect top via JUST_STACK_TOP
  StackFrame* topFrame = JUST_STACK_TOP(&stack, StackFrame);
  JUST_EXPECT_TO_BE_NOT_NULL(topFrame);
  JUST_EXPECT_TO_BE((uint32_t)2, topFrame->frameId);
  JUST_EXPECT_TO_BE((uint64_t)0x2000ULL, topFrame->returnAddress);

  // Push by value macro (triggers dynamic array growth from cap 2 -> 4)
  StackFrame f3 = { .frameId = 3, .returnAddress = 0x3000ULL };
  JUST_STACK_PUSH_VAL(&stack, StackFrame, f3);

  JUST_EXPECT_TO_BE((size_t)3, justStackSize(&stack));
  topFrame = JUST_STACK_TOP(&stack, StackFrame);
  JUST_EXPECT_TO_BE((uint32_t)3, topFrame->frameId);
  JUST_EXPECT_TO_BE((uint64_t)0x3000ULL, topFrame->returnAddress);

  // Drain stack
  StackFrame outFrame;
  JUST_EXPECT_TO_BE_TRUE(justStackPop(&stack, &outFrame));
  JUST_EXPECT_TO_BE((uint32_t)3, outFrame.frameId);

  JUST_EXPECT_TO_BE_TRUE(justStackPop(&stack, &outFrame));
  JUST_EXPECT_TO_BE((uint32_t)2, outFrame.frameId);

  JUST_EXPECT_TO_BE_TRUE(justStackPop(&stack, &outFrame));
  JUST_EXPECT_TO_BE((uint32_t)1, outFrame.frameId);

  JUST_EXPECT_TO_BE_TRUE(justStackIsEmpty(&stack));

  justStackDestroy(&stack);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 4. ARENA (LINEAR ALLOCATOR) INTEGRATION
// =============================================================================

static JustTestResult testStackArenaBacked(void)
{
  const char* tag = "STACK_ARENA";
  justMemorySetLimit(4096, tag);

  justLinearAllocator arena;
  bool arenaOk = justLinearAllocCreate(&arena, 1024, NULL, tag);
  JUST_EXPECT_TO_BE_TRUE(arenaOk);

  JustStack stack;
  bool stackOk = justStackCreate(&stack, 8, sizeof(int32_t), &arena, tag);
  JUST_EXPECT_TO_BE_TRUE(stackOk);

  for (int32_t i = 1; i <= 8; ++i)
  {
    JUST_STACK_PUSH_VAL(&stack, int32_t, i * 5);
  }
  JUST_EXPECT_TO_BE((size_t)8, justStackSize(&stack));

  int32_t popped = 0;
  for (int32_t i = 8; i >= 1; --i)
  {
    JUST_EXPECT_TO_BE_TRUE(justStackPop(&stack, &popped));
    JUST_EXPECT_TO_BE((int32_t)(i * 5), popped);
  }
  JUST_EXPECT_TO_BE_TRUE(justStackIsEmpty(&stack));

  // Destroying arena-backed stack should clean struct without calling free()
  justStackDestroy(&stack);
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
  justTestRegister(testStackCreateDestroy,    "Stack: Init, Sizing, and Destroy",      GROUP_STACK_LIFECYCLE);

  // Group 1: LIFO Ordering
  justTestRegister(testStackLIFOOrdering,     "Stack: Push, Pop, and Peek Semantics",  GROUP_STACK_LIFO);

  // Group 2: Macros & Emplace
  justTestRegister(testStackMacrosAndEmplace, "Stack: Zero-Copy Emplace and Macros",   GROUP_STACK_MACROS);

  // Group 3: Arena Backing
  justTestRegister(testStackArenaBacked,      "Stack: Linear Allocator Backing",       GROUP_STACK_ARENA);

  return (int)justTestRunAll();
}
