/**
 * @file testOrderedSet.c
 * @brief Unit test suite for JustAVLTree (OrderedSet).
 */

#include <justUtils/testing/testManager.h>
#include <justUtils/testing/expect.h>
#include <justUtils/dataStructures/orderedSet.h>
#include <justUtils/memory/linearAlloc.h>
#include <justUtils/memory/tracker.h>
#include <stdint.h>
#include <stdlib.h>

#define GROUP_SET_LIFECYCLE 0
#define GROUP_SET_OPS       1
#define GROUP_SET_TRAVERSE  2
#define GROUP_SET_AVL       3
#define GROUP_SET_REMOVE    4
#define GROUP_SET_ARENA     5

// Standard integer comparator matching justCompareFunc signature
static int32_t compareInt32(const void* a, const void* b, size_t size)
{
  (void)size;
  int32_t valA = *(const int32_t*)a;
  int32_t valB = *(const int32_t*)b;

  if (valA > valB) return 1;
  if (valA < valB) return -1;
  return 0;
}

// Recursive helper to verify AVL property on every node in the tree
static bool verifyAVLInvariants(const JustAVLNode* node, int32_t* outHeight)
{
  if (!node)
  {
    if (outHeight) *outHeight = 0;
    return true;
  }

  int32_t leftHeight = 0;
  int32_t rightHeight = 0;

  if (!verifyAVLInvariants(node->left, &leftHeight)) return false;
  if (!verifyAVLInvariants(node->right, &rightHeight)) return false;

  int32_t balanceFactor = leftHeight - rightHeight;
  if (balanceFactor < -1 || balanceFactor > 1) return false;

  int32_t computedHeight = 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);
  if (outHeight) *outHeight = computedHeight;

  return true;
}

// =============================================================================
// 1. LIFECYCLE TESTS
// =============================================================================

static JustTestResult testOrderedSetCreateDestroy(void)
{
  const char* tag = "SET_INIT";
  justMemorySetLimit(4096, tag);

  OrderedSet set;
  bool ok = justOrderedSetCreate(&set, sizeof(int32_t), compareInt32, NULL, tag);
  JUST_EXPECT_TO_BE_TRUE(ok);

  JUST_EXPECT_TO_BE((size_t)0, justOrderedSetSize(&set));
  JUST_EXPECT_TO_BE_TRUE(justOrderedSetIsEmpty(&set));
  JUST_EXPECT_TO_BE_NULL(set.root);

  justOrderedSetDestroy(&set);
  JUST_EXPECT_TO_BE_NULL(set.root);
  JUST_EXPECT_TO_BE((size_t)0, justOrderedSetSize(&set));
  JUST_EXPECT_TO_BE_TRUE(justOrderedSetIsEmpty(&set));

  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 2. INSERT, FIND, MACROS, AND DEDUPLICATION
// =============================================================================

static JustTestResult testOrderedSetInsertAndFind(void)
{
  const char* tag = "SET_OPS";
  justMemorySetLimit(4096, tag);

  OrderedSet set;
  JUST_ORDERED_SET_INIT_TAGGED(&set, int32_t, tag);
  set.compare = compareInt32;

  // Insert elements using typed macro
  JUST_EXPECT_TO_BE_TRUE(JUST_ORDERED_SET_INSERT_VAL(&set, int32_t, 50));
  JUST_EXPECT_TO_BE_TRUE(JUST_ORDERED_SET_INSERT_VAL(&set, int32_t, 25));
  JUST_EXPECT_TO_BE_TRUE(JUST_ORDERED_SET_INSERT_VAL(&set, int32_t, 75));

  JUST_EXPECT_TO_BE((size_t)3, justOrderedSetSize(&set));
  JUST_EXPECT_TO_BE_FALSE(justOrderedSetIsEmpty(&set));

  // Find verification
  JUST_EXPECT_TO_BE_TRUE(JUST_ORDERED_SET_CONTAINS_VAL(&set, int32_t, 50));
  JUST_EXPECT_TO_BE_TRUE(JUST_ORDERED_SET_CONTAINS_VAL(&set, int32_t, 25));
  JUST_EXPECT_TO_BE_TRUE(JUST_ORDERED_SET_CONTAINS_VAL(&set, int32_t, 75));
  JUST_EXPECT_TO_BE_FALSE(JUST_ORDERED_SET_CONTAINS_VAL(&set, int32_t, 100));

  int32_t* foundPtr = JUST_ORDERED_SET_FIND_VAL(&set, int32_t, 25);
  JUST_EXPECT_TO_BE_NOT_NULL(foundPtr);
  JUST_EXPECT_TO_BE((int32_t)25, *foundPtr);

  // Set property: duplicate insertion should be rejected or no-op
  size_t countBefore = justOrderedSetSize(&set);
  bool dupResult = JUST_ORDERED_SET_INSERT_VAL(&set, int32_t, 50);
  JUST_EXPECT_TO_BE_FALSE(dupResult);
  JUST_EXPECT_TO_BE(countBefore, justOrderedSetSize(&set));

  justOrderedSetDestroy(&set);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 3. IN-ORDER TRAVERSAL
// =============================================================================

typedef struct TraversalContext
{
  int32_t buffer[16];
  size_t  count;
} TraversalContext;

static void recordTraversal(const void* value, void* userData)
{
  TraversalContext* ctx = (TraversalContext*)userData;
  if (ctx->count < 16)
  {
    ctx->buffer[ctx->count++] = *(const int32_t*)value;
  }
}

static JustTestResult testOrderedSetInorderTraversal(void)
{
  const char* tag = "SET_TRAV";
  justMemorySetLimit(4096, tag);

  OrderedSet set;
  justOrderedSetCreate(&set, sizeof(int32_t), compareInt32, NULL, tag);

  // Insert in arbitrary order
  int32_t values[] = { 40, 20, 60, 10, 30, 50, 70 };
  for (size_t i = 0; i < 7; ++i)
  {
    justOrderedSetInsert(&set, &values[i]);
  }
  JUST_EXPECT_TO_BE((size_t)7, justOrderedSetSize(&set));

  TraversalContext ctx = { .count = 0 };
  justOrderedSetTraverseInorder(&set, recordTraversal, &ctx);

  JUST_EXPECT_TO_BE((size_t)7, ctx.count);

  // In-order traversal must output strictly ascending sequence
  int32_t expected[] = { 10, 20, 30, 40, 50, 60, 70 };
  for (size_t i = 0; i < 7; ++i)
  {
    JUST_EXPECT_TO_BE(expected[i], ctx.buffer[i]);
  }

  justOrderedSetDestroy(&set);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 4. AVL TREE ROTATION & BALANCE INVARIANTS
// =============================================================================

static JustTestResult testOrderedSetAVLBalancing(void)
{
  const char* tag = "SET_BAL";
  justMemorySetLimit(16384, tag);

  OrderedSet set;
  justOrderedSetCreate(&set, sizeof(int32_t), compareInt32, NULL, tag);

  // Sequential ascending inserts: would degenerate an un-balanced BST into an O(N) linked list.
  // In an AVL tree, this triggers cascading left rotations.
  for (int32_t i = 1; i <= 31; ++i)
  {
    justOrderedSetInsert(&set, &i);
    int32_t h = 0;
    JUST_EXPECT_TO_BE_TRUE(verifyAVLInvariants(set.root, &h));
  }

  JUST_EXPECT_TO_BE((size_t)31, justOrderedSetSize(&set));

  // Max height of a 31-node AVL tree must be <= 6 (1.44 * log2(32))
  int32_t totalHeight = 0;
  bool balanced = verifyAVLInvariants(set.root, &totalHeight);
  JUST_EXPECT_TO_BE_TRUE(balanced);
  JUST_EXPECT_TO_BE_TRUE(totalHeight <= 6);

  justOrderedSetDestroy(&set);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 5. REMOVAL, BALANCE RETENTION, AND CLEAR
// =============================================================================

static JustTestResult testOrderedSetRemoveAndClear(void)
{
  const char* tag = "SET_REM";
  justMemorySetLimit(8192, tag);

  OrderedSet set;
  justOrderedSetCreate(&set, sizeof(int32_t), compareInt32, NULL, tag);

  int32_t items[] = { 15, 10, 20, 5, 12, 18, 25 };
  for (size_t i = 0; i < 7; ++i)
  {
    justOrderedSetInsert(&set, &items[i]);
  }
  JUST_EXPECT_TO_BE((size_t)7, justOrderedSetSize(&set));

  // 1. Remove leaf node (5)
  JUST_EXPECT_TO_BE_TRUE(JUST_ORDERED_SET_REMOVE_VAL(&set, int32_t, 5));
  JUST_EXPECT_TO_BE((size_t)6, justOrderedSetSize(&set));
  JUST_EXPECT_TO_BE_FALSE(JUST_ORDERED_SET_CONTAINS_VAL(&set, int32_t, 5));
  JUST_EXPECT_TO_BE_TRUE(verifyAVLInvariants(set.root, NULL));

  // 2. Remove node with one child (10 now has only 12)
  JUST_EXPECT_TO_BE_TRUE(JUST_ORDERED_SET_REMOVE_VAL(&set, int32_t, 10));
  JUST_EXPECT_TO_BE((size_t)5, justOrderedSetSize(&set));
  JUST_EXPECT_TO_BE_FALSE(JUST_ORDERED_SET_CONTAINS_VAL(&set, int32_t, 10));
  JUST_EXPECT_TO_BE_TRUE(JUST_ORDERED_SET_CONTAINS_VAL(&set, int32_t, 12));
  JUST_EXPECT_TO_BE_TRUE(verifyAVLInvariants(set.root, NULL));

  // 3. Remove root node with two children (15)
  JUST_EXPECT_TO_BE_TRUE(JUST_ORDERED_SET_REMOVE_VAL(&set, int32_t, 15));
  JUST_EXPECT_TO_BE((size_t)4, justOrderedSetSize(&set));
  JUST_EXPECT_TO_BE_FALSE(JUST_ORDERED_SET_CONTAINS_VAL(&set, int32_t, 15));
  JUST_EXPECT_TO_BE_TRUE(verifyAVLInvariants(set.root, NULL));

  // 4. Remove nonexistent node
  JUST_EXPECT_TO_BE_FALSE(JUST_ORDERED_SET_REMOVE_VAL(&set, int32_t, 999));
  JUST_EXPECT_TO_BE((size_t)4, justOrderedSetSize(&set));

  // 5. Clear all entries
  justOrderedSetClear(&set);
  JUST_EXPECT_TO_BE((size_t)0, justOrderedSetSize(&set));
  JUST_EXPECT_TO_BE_TRUE(justOrderedSetIsEmpty(&set));
  JUST_EXPECT_TO_BE_NULL(set.root);

  justOrderedSetDestroy(&set);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 6. ARENA (LINEAR ALLOCATOR) BACKING
// =============================================================================

static JustTestResult testOrderedSetArenaBacked(void)
{
  const char* tag = "SET_ARENA";
  justMemorySetLimit(4096, "SET_ARENA");
  JustLinearAllocator arena;
  bool arenaOk = justLinearAllocCreate(&arena, 2048, NULL, tag);
  JUST_EXPECT_TO_BE_TRUE(arenaOk);

  OrderedSet set;
  bool setOk = justOrderedSetCreate(&set, sizeof(int32_t), compareInt32, &arena, tag);
  JUST_EXPECT_TO_BE_TRUE(setOk);

  for (int32_t i = 1; i <= 8; ++i)
  {
    justOrderedSetInsert(&set, &i);
  }
  JUST_EXPECT_TO_BE((size_t)8, justOrderedSetSize(&set));
  JUST_EXPECT_TO_BE_TRUE(verifyAVLInvariants(set.root, NULL));

  // Destroying arena-backed set must reset state without calling free()
  justOrderedSetDestroy(&set);
  JUST_EXPECT_TO_BE_NULL(set.root);
  JUST_EXPECT_TO_BE_TRUE(justMemoryCheckBounds());

  justLinearAllocDestroy(&arena);
  return JUST_TEST_PASS;
}

// =============================================================================
// MAIN ENTRY POINT
// =============================================================================

int main(void)
{
  // Group 0: Lifecycle
  justTestRegister(testOrderedSetCreateDestroy,   "OrderedSet: Init, Sizing, and Destroy",      GROUP_SET_LIFECYCLE);

  // Group 1: Insert & Find
  justTestRegister(testOrderedSetInsertAndFind,   "OrderedSet: Insert, Find, and Deduplication", GROUP_SET_OPS);

  // Group 2: Traversal
  justTestRegister(testOrderedSetInorderTraversal,"OrderedSet: In-Order Ascending Traversal",   GROUP_SET_TRAVERSE);

  // Group 3: AVL Balance Invariants
  justTestRegister(testOrderedSetAVLBalancing,    "OrderedSet: Sequential Insert AVL Balancing",GROUP_SET_AVL);

  // Group 4: Removal & Clear
  justTestRegister(testOrderedSetRemoveAndClear,  "OrderedSet: Leaf, Single, Two-Child Delete", GROUP_SET_REMOVE);

  // Group 5: Arena Backing
  justTestRegister(testOrderedSetArenaBacked,     "OrderedSet: Linear Allocator Arena Backing", GROUP_SET_ARENA);

  return (int)justTestRunAll();
}
