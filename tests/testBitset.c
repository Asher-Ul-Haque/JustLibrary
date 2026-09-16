/**
 * @file testBitset.c
 * @brief Unit test suite for justBitset.
 */

#include <justUtils/testing/testManager.h>
#include <justUtils/testing/expect.h>
#include <justUtils/dataStructures/bitset.h>
#include <justUtils/memory/tracker.h>
#include <stdint.h>
#include <string.h>

#define GROUP_BITSET_BASIC 0
#define GROUP_BITSET_LOGIC 1
#define GROUP_BITSET_USER  2

// =============================================================================
// 1. LIFECYCLE, MUTATION & BOUNDARY TESTS
// =============================================================================

static JustTestResult testBitsetCreateAndDestroy(void)
{
  justBitset bs;
  bool ok = justBitsetCreate(&bs, 100, NULL, "BITSET_TEST");
  JUST_EXPECT_TO_BE_TRUE(ok);

  JUST_EXPECT_TO_BE((size_t)100, justBitsetCapacity(&bs));
  JUST_EXPECT_TO_BE((size_t)2, bs.wordCount); // ceil(100 / 64) = 2
  JUST_EXPECT_TO_BE_TRUE(bs.ownsMemory);
  JUST_EXPECT_TO_BE_NOT_NULL(bs.words);

  // By default, creation initializes bits to zero
  for (size_t i = 0; i < 100; ++i)
  {
    JUST_EXPECT_TO_BE_FALSE(justBitsetGet(&bs, i));
  }

  justBitsetDestroy(&bs);
  return JUST_TEST_PASS;
}

static JustTestResult testBitsetMutationsAndWordBoundaries(void)
{
  justBitset bs;
  justBitsetCreate(&bs, 130, NULL, "BITSET_MUTATION"); // Spans 3 64-bit words

  // Test bit index 0 (first word)
  justBitsetSet(&bs, 0);
  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&bs, 0));

  // Test word boundaries: 63 (end of word 0), 64 (start of word 1), 127 (end of word 1), 128 (start of word 2)
  justBitsetSet(&bs, 63);
  justBitsetSet(&bs, 64);
  justBitsetSet(&bs, 127);
  justBitsetSet(&bs, 128);

  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&bs, 63));
  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&bs, 64));
  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&bs, 127));
  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&bs, 128));

  // Ensure neighbouring bits remained clear
  JUST_EXPECT_TO_BE_FALSE(justBitsetGet(&bs, 1));
  JUST_EXPECT_TO_BE_FALSE(justBitsetGet(&bs, 62));
  JUST_EXPECT_TO_BE_FALSE(justBitsetGet(&bs, 65));
  JUST_EXPECT_TO_BE_FALSE(justBitsetGet(&bs, 129));

  // Test toggle
  justBitsetToggle(&bs, 64); // 1 -> 0
  JUST_EXPECT_TO_BE_FALSE(justBitsetGet(&bs, 64));
  justBitsetToggle(&bs, 64); // 0 -> 1
  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&bs, 64));

  // Test clear
  justBitsetClear(&bs, 63);
  JUST_EXPECT_TO_BE_FALSE(justBitsetGet(&bs, 63));

  justBitsetDestroy(&bs);
  return JUST_TEST_PASS;
}

static JustTestResult testBitsetBulkOperationsAndTailMask(void)
{
  justBitset bs;
  // 70 bits = Word 0 (64 bits) + Word 1 (6 bits active, 58 bits padded)
  justBitsetCreate(&bs, 70, NULL, "BITSET_BULK");

  justBitsetSetAll(&bs);

  // All valid bits should be set
  for (size_t i = 0; i < 70; ++i)
  {
    JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&bs, i));
  }

  // Bits past capacity in word 1 must be zeroed out by tail mask
  uint64_t expectedWord1 = ((uint64_t)1 << (70 % 64)) - 1; // 6 set bits: 0x3F
  JUST_EXPECT_TO_BE((uint64_t)~0ULL, bs.words[0]);
  JUST_EXPECT_TO_BE(expectedWord1, bs.words[1]);

  justBitsetClearAll(&bs);
  JUST_EXPECT_TO_BE((uint64_t)0, bs.words[0]);
  JUST_EXPECT_TO_BE((uint64_t)0, bs.words[1]);

  justBitsetDestroy(&bs);
  return JUST_TEST_PASS;
}

// =============================================================================
// 2. BITWISE LOGIC & EQUALITY TESTS
// =============================================================================

static JustTestResult testBitsetEquality(void)
{
  justBitset a, b, c;
  justBitsetCreate(&a, 80, NULL, "BS_A");
  justBitsetCreate(&b, 80, NULL, "BS_B");
  justBitsetCreate(&c, 90, NULL, "BS_C");

  // Empty sets of same capacity are equal
  JUST_EXPECT_TO_BE_TRUE(justBitsetEquals(&a, &b));

  // Different capacities are not equal
  JUST_EXPECT_TO_BE_FALSE(justBitsetEquals(&a, &c));

  justBitsetSet(&a, 10);
  justBitsetSet(&a, 75);
  JUST_EXPECT_TO_BE_FALSE(justBitsetEquals(&a, &b));

  justBitsetSet(&b, 10);
  justBitsetSet(&b, 75);
  JUST_EXPECT_TO_BE_TRUE(justBitsetEquals(&a, &b));

  justBitsetDestroy(&a);
  justBitsetDestroy(&b);
  justBitsetDestroy(&c);
  return JUST_TEST_PASS;
}

static JustTestResult testBitsetBitwiseOperations(void)
{
  justBitset a, b;
  justBitsetCreate(&a, 128, NULL, "BS_OP_A");
  justBitsetCreate(&b, 128, NULL, "BS_OP_B");

  // A = { 5, 50, 70 }
  justBitsetSet(&a, 5);
  justBitsetSet(&a, 50);
  justBitsetSet(&a, 70);

  // B = { 50, 70, 90 }
  justBitsetSet(&b, 50);
  justBitsetSet(&b, 70);
  justBitsetSet(&b, 90);

  // Union test: A = A | B => { 5, 50, 70, 90 }
  justBitset aUnion;
  justBitsetCreate(&aUnion, 128, NULL, "BS_UNION");
  justBitsetSet(&aUnion, 5);
  justBitsetSet(&aUnion, 50);
  justBitsetSet(&aUnion, 70);

  justBitsetUnion(&aUnion, &b);
  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&aUnion, 5));
  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&aUnion, 50));
  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&aUnion, 70));
  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&aUnion, 90));
  JUST_EXPECT_TO_BE_FALSE(justBitsetGet(&aUnion, 0));

  // Intersection test: A = A & B => { 50, 70 }
  justBitset aInter;
  justBitsetCreate(&aInter, 128, NULL, "BS_INTER");
  justBitsetSet(&aInter, 5);
  justBitsetSet(&aInter, 50);
  justBitsetSet(&aInter, 70);

  justBitsetIntersection(&aInter, &b);
  JUST_EXPECT_TO_BE_FALSE(justBitsetGet(&aInter, 5));
  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&aInter, 50));
  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&aInter, 70));
  JUST_EXPECT_TO_BE_FALSE(justBitsetGet(&aInter, 90));

  // Difference test: A = A & ~B => { 5 }
  justBitset aDiff;
  justBitsetCreate(&aDiff, 128, NULL, "BS_DIFF");
  justBitsetSet(&aDiff, 5);
  justBitsetSet(&aDiff, 50);
  justBitsetSet(&aDiff, 70);

  justBitsetDifference(&aDiff, &b);
  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&aDiff, 5));
  JUST_EXPECT_TO_BE_FALSE(justBitsetGet(&aDiff, 50));
  JUST_EXPECT_TO_BE_FALSE(justBitsetGet(&aDiff, 70));
  JUST_EXPECT_TO_BE_FALSE(justBitsetGet(&aDiff, 90));

  justBitsetDestroy(&a);
  justBitsetDestroy(&b);
  justBitsetDestroy(&aUnion);
  justBitsetDestroy(&aInter);
  justBitsetDestroy(&aDiff);
  return JUST_TEST_PASS;
}

// =============================================================================
// 3. USER SUPPLIED BUFFER TESTS
// =============================================================================

static JustTestResult testBitsetUserSuppliedMemory(void)
{
  uint64_t backingBuffer[2] = { 0, 0 }; // 128 bits capacity
  justBitset bs;

  bool ok = justBitsetCreate(&bs, 128, backingBuffer, "USER_MEM");
  JUST_EXPECT_TO_BE_TRUE(ok);
  JUST_EXPECT_TO_BE_FALSE(bs.ownsMemory);
  JUST_EXPECT_TO_BE((uint64_t*)backingBuffer, bs.words);

  justBitsetSet(&bs, 10);
  justBitsetSet(&bs, 70);

  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&bs, 10));
  JUST_EXPECT_TO_BE_TRUE(justBitsetGet(&bs, 70));

  // Verify bits directly modified the user buffer
  JUST_EXPECT_TO_BE(((uint64_t)1 << 10), backingBuffer[0]);
  JUST_EXPECT_TO_BE(((uint64_t)1 << (70 - 64)), backingBuffer[1]);

  // Destroying a non-owning bitset should not attempt to free the buffer
  justBitsetDestroy(&bs);
  JUST_EXPECT_TO_BE_TRUE(justMemoryCheckBounds());

  return JUST_TEST_PASS;
}

// =============================================================================
// MAIN ENTRY POINT
// =============================================================================

int main(void)
{
  // Group 0: Lifecycle & Operations
  justTestRegister(testBitsetCreateAndDestroy,               "Bitset: Create, Sizing, and Destroy",       GROUP_BITSET_BASIC);
  justTestRegister(testBitsetMutationsAndWordBoundaries,    "Bitset: Get/Set/Toggle & Word Boundaries",  GROUP_BITSET_BASIC);
  justTestRegister(testBitsetBulkOperationsAndTailMask,     "Bitset: SetAll, ClearAll, and Tail Mask",   GROUP_BITSET_BASIC);

  // Group 1: Logic & Equality
  justTestRegister(testBitsetEquality,                      "Bitset: Equality Comparison",               GROUP_BITSET_LOGIC);
  justTestRegister(testBitsetBitwiseOperations,             "Bitset: Union, Intersection, Difference",   GROUP_BITSET_LOGIC);

  // Group 2: User Memory Buffer
  justTestRegister(testBitsetUserSuppliedMemory,            "Bitset: User-Supplied Buffer Non-Owning",   GROUP_BITSET_USER);

  return (int)justTestRunAll();
}
