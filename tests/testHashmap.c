/**
 * @file testHashMap.c
 * @brief Unit test suite for JustHashMap.
 */

#include <justUtils/testing/testManager.h>
#include <justUtils/testing/expect.h>
#include <justUtils/dataStructures/hashMap.h>
#include <justUtils/memory/linearAlloc.h>
#include <justUtils/memory/tracker.h>
#include <stdint.h>
#include <string.h>

#define GROUP_MAP_LIFECYCLE 0
#define GROUP_MAP_OPS       1
#define GROUP_MAP_COLLISION 2
#define GROUP_MAP_RESIZE    3
#define GROUP_MAP_ARENA     4

// =============================================================================
// 1. BASIC LIFECYCLE & CAPACITY
// =============================================================================

static JustTestResult testHashMapCreateDestroy(void)
{
  const char* tag = "MAP_INIT";
  justMemorySetLimit(4096, tag);

  JustHashMap map;
  bool ok = JUST_HASHMAP_INIT_TAGGED(&map, uint32_t, uint64_t, 16, tag);
  JUST_EXPECT_TO_BE_TRUE(ok);

  JUST_EXPECT_TO_BE((size_t)0, justHashmapSize(&map));
  JUST_EXPECT_TO_BE((size_t)16, map.capacity);
  JUST_EXPECT_TO_BE((size_t)15, map.mask);
  JUST_EXPECT_TO_BE((size_t)0, map.tombstoneCount);
  JUST_EXPECT_TO_BE_TRUE(justHashmapIsEmpty(&map));
  JUST_EXPECT_TO_BE_NOT_NULL(map.slots);

  justHashmapDestroy(&map);
  JUST_EXPECT_TO_BE_NULL(map.slots);
  JUST_EXPECT_TO_BE((size_t)0, justHashmapSize(&map));
  JUST_EXPECT_TO_BE_TRUE(justHashmapIsEmpty(&map));

  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 2. INSERTION, RETRIEVAL, AND OVERWRITE
// =============================================================================

static JustTestResult testHashMapSetAndGet(void)
{
  const char* tag = "MAP_OPS";
  justMemorySetLimit(4096, tag);

  JustHashMap map;
  JUST_HASHMAP_INIT_TAGGED(&map, int32_t, int64_t, 16, tag);

  int32_t key1 = 42;
  int64_t val1 = 1000;
  int32_t key2 = 99;
  int64_t val2 = 2000;

  // Insert key1
  bool ok1 = justHashmapSet(&map, &key1, &val1);
  JUST_EXPECT_TO_BE_TRUE(ok1);
  JUST_EXPECT_TO_BE((size_t)1, justHashmapSize(&map));
  JUST_EXPECT_TO_BE_FALSE(justHashmapIsEmpty(&map));
  JUST_EXPECT_TO_BE_TRUE(justHashmapContains(&map, &key1));

  // Insert key2
  bool ok2 = justHashmapSet(&map, &key2, &val2);
  JUST_EXPECT_TO_BE_TRUE(ok2);
  JUST_EXPECT_TO_BE((size_t)2, justHashmapSize(&map));

  // Retrieve values via macro
  int64_t* retrieved1 = JUST_HASHMAP_GET(&map, int64_t, &key1);
  int64_t* retrieved2 = JUST_HASHMAP_GET(&map, int64_t, &key2);
  JUST_EXPECT_TO_BE_NOT_NULL(retrieved1);
  JUST_EXPECT_TO_BE_NOT_NULL(retrieved2);
  JUST_EXPECT_TO_BE((int64_t)1000, *retrieved1);
  JUST_EXPECT_TO_BE((int64_t)2000, *retrieved2);

  // Missing key lookup
  int32_t missingKey = 999;
  JUST_EXPECT_TO_BE_NULL(justHashmapGet(&map, &missingKey));
  JUST_EXPECT_TO_BE_FALSE(justHashmapContains(&map, &missingKey));

  // Overwrite key1 with new value
  int64_t val1Updated = 5555;
  bool okUpdate = justHashmapSet(&map, &key1, &val1Updated);
  JUST_EXPECT_TO_BE_TRUE(okUpdate);
  JUST_EXPECT_TO_BE((size_t)2, justHashmapSize(&map)); // Size must stay identical

  int64_t* retrievedUpdated = JUST_HASHMAP_GET(&map, int64_t, &key1);
  JUST_EXPECT_TO_BE_NOT_NULL(retrievedUpdated);
  JUST_EXPECT_TO_BE((int64_t)5555, *retrievedUpdated);

  justHashmapDestroy(&map);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 3. REMOVAL, TOMBSTONES, AND CLEAR
// =============================================================================

static JustTestResult testHashMapRemoveAndRecycle(void)
{
  const char* tag = "MAP_REMOVE";
  justMemorySetLimit(4096, tag);

  JustHashMap map;
  JUST_HASHMAP_INIT_TAGGED(&map, int32_t, int32_t, 16, tag);

  int32_t k1 = 1, v1 = 10;
  int32_t k2 = 2, v2 = 20;
  int32_t k3 = 3, v3 = 30;

  justHashmapSet(&map, &k1, &v1);
  justHashmapSet(&map, &k2, &v2);
  justHashmapSet(&map, &k3, &v3);
  JUST_EXPECT_TO_BE((size_t)3, justHashmapSize(&map));

  // Remove middle element
  bool removed = justHashmapRemove(&map, &k2);
  JUST_EXPECT_TO_BE_TRUE(removed);
  JUST_EXPECT_TO_BE((size_t)2, justHashmapSize(&map));
  JUST_EXPECT_TO_BE((size_t)1, map.tombstoneCount);
  JUST_EXPECT_TO_BE_FALSE(justHashmapContains(&map, &k2));
  JUST_EXPECT_TO_BE_NULL(justHashmapGet(&map, &k2));

  // Remaining keys must still be reachable
  JUST_EXPECT_TO_BE_TRUE(justHashmapContains(&map, &k1));
  JUST_EXPECT_TO_BE_TRUE(justHashmapContains(&map, &k3));

  // Removing already removed key must fail safely
  bool removeAgain = justHashmapRemove(&map, &k2);
  JUST_EXPECT_TO_BE_FALSE(removeAgain);

  // Re-inserting into map can reuse tombstone slot
  int32_t v2New = 200;
  bool reinsert = justHashmapSet(&map, &k2, &v2New);
  JUST_EXPECT_TO_BE_TRUE(reinsert);
  JUST_EXPECT_TO_BE((size_t)3, justHashmapSize(&map));
  JUST_EXPECT_TO_BE((int32_t)200, *JUST_HASHMAP_GET(&map, int32_t, &k2));

  // Clear map
  justHashmapClear(&map);
  JUST_EXPECT_TO_BE((size_t)0, justHashmapSize(&map));
  JUST_EXPECT_TO_BE((size_t)0, map.tombstoneCount);
  JUST_EXPECT_TO_BE_TRUE(justHashmapIsEmpty(&map));
  JUST_EXPECT_TO_BE_FALSE(justHashmapContains(&map, &k1));

  justHashmapDestroy(&map);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 4. FORCED COLLISIONS & REHASH EXPANSION
// =============================================================================

// Bad hasher that forces every key to hash to the exact same bucket (slot 0)
static uint64_t degenerateHash(const void* KEY, size_t KEY_SIZE)
{
  (void)KEY;
  (void)KEY_SIZE;
  return 0;
}

static JustTestResult testHashMapCollisions(void)
{
  const char* tag = "MAP_COLLIDE";
  justMemorySetLimit(8192, tag);

  JustHashMap map;
  bool ok = justHashmapCreate(&map, sizeof(int32_t), sizeof(int32_t), 16, degenerateHash, NULL, NULL, tag);
  JUST_EXPECT_TO_BE_TRUE(ok);

  // Insert 6 elements that all hash to slot 0 to exercise linear probing
  for (int32_t i = 0; i < 6; ++i)
  {
    int32_t val = i * 11;
    bool inserted = justHashmapSet(&map, &i, &val);
    JUST_EXPECT_TO_BE_TRUE(inserted);
  }

  JUST_EXPECT_TO_BE((size_t)6, justHashmapSize(&map));

  // Verify all elements can be looked up through the linear probe chain
  for (int32_t i = 0; i < 6; ++i)
  {
    int32_t* val = JUST_HASHMAP_GET(&map, int32_t, &i);
    JUST_EXPECT_TO_BE_NOT_NULL(val);
    JUST_EXPECT_TO_BE((int32_t)(i * 11), *val);
  }

  // Remove key 2 from middle of collision probe chain
  int32_t removeKey = 2;
  JUST_EXPECT_TO_BE_TRUE(justHashmapRemove(&map, &removeKey));
  JUST_EXPECT_TO_BE_FALSE(justHashmapContains(&map, &removeKey));

  // Keys probed after the tombstone must still be found
  int32_t laterKey = 4;
  int32_t* laterVal = JUST_HASHMAP_GET(&map, int32_t, &laterKey);
  JUST_EXPECT_TO_BE_NOT_NULL(laterVal);
  JUST_EXPECT_TO_BE((int32_t)44, *laterVal);

  justHashmapDestroy(&map);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

static JustTestResult testHashMapRehashResize(void)
{
  const char* tag = "MAP_RESIZE";
  justMemorySetLimit(16384, tag);

  JustHashMap map;
  JUST_HASHMAP_INIT_TAGGED(&map, uint32_t, uint32_t, 16, tag);

  // Insert 40 items to trigger multiple capacity doublings (16 -> 32 -> 64)
  for (uint32_t i = 0; i < 40; ++i)
  {
    uint32_t val = i * 100;
    bool ok = justHashmapSet(&map, &i, &val);
    JUST_EXPECT_TO_BE_TRUE(ok);
  }

  JUST_EXPECT_TO_BE((size_t)40, justHashmapSize(&map));
  JUST_EXPECT_TO_BE_TRUE(map.capacity >= 64);

  // Ensure all 40 items survived across multiple rehashes
  for (uint32_t i = 0; i < 40; ++i)
  {
    uint32_t* val = JUST_HASHMAP_GET(&map, uint32_t, &i);
    JUST_EXPECT_TO_BE_NOT_NULL(val);
    JUST_EXPECT_TO_BE((uint32_t)(i * 100), *val);
  }

  justHashmapDestroy(&map);
  justMemorySetLimit(0, tag);
  return JUST_TEST_PASS;
}

// =============================================================================
// 5. LINEAR ALLOCATOR (ARENA) BACKING
// =============================================================================

static JustTestResult testHashMapArenaBacked(void)
{
  const char* tag = "MAP_ARENA";
  justLinearAllocator arena;
  bool arenaOk = justLinearAllocCreate(&arena, 1024, NULL, tag);
  JUST_EXPECT_TO_BE_TRUE(arenaOk);

  JustHashMap map;
  bool mapOk = justHashmapCreate(&map, sizeof(int32_t), sizeof(int32_t), 16, NULL, NULL, &arena, tag);
  JUST_EXPECT_TO_BE_TRUE(mapOk);
  JUST_EXPECT_TO_BE_NOT_NULL(map.slots);

  for (int32_t i = 0; i < 5; ++i)
  {
    int32_t val = i * 2;
    justHashmapSet(&map, &i, &val);
  }

  JUST_EXPECT_TO_BE((size_t)5, justHashmapSize(&map));

  // Destroying arena-backed map must reset state without calling free()
  justHashmapDestroy(&map);
  JUST_EXPECT_TO_BE_NULL(map.slots);
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
  justTestRegister(testHashMapCreateDestroy,   "HashMap: Init, Masking, and Destroy",      GROUP_MAP_LIFECYCLE);

  // Group 1: Standard Operations
  justTestRegister(testHashMapSetAndGet,         "HashMap: Set, Get, and In-Place Overwrite", GROUP_MAP_OPS);
  justTestRegister(testHashMapRemoveAndRecycle, "HashMap: Remove, Tombstones, and Clear",   GROUP_MAP_OPS);

  // Group 2: Collisions
  justTestRegister(testHashMapCollisions,        "HashMap: Linear Probing & Collision Chain", GROUP_MAP_COLLISION);

  // Group 3: Geometric Resizing
  justTestRegister(testHashMapRehashResize,      "HashMap: Geometric Growth & Rehash Survival", GROUP_MAP_RESIZE);

  // Group 4: Arena Backing
  justTestRegister(testHashMapArenaBacked,       "HashMap: Linear Allocator Backing",         GROUP_MAP_ARENA);

  return (int)justTestRunAll();
}
