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
 * @file : orderedSet.h 
 * @brief : Ordered set implementation using a self-balancing AVL tree
 */

#pragma once

#include <justUtils/defines.h>
#include <justUtils/core/asserts.h>
#include <justUtils/memory/linearAlloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief : A normal compare function, that checks which of the two elements is greater
 * @param A : One item
 * @param B : Another item
 * @param SIZE : Size of the items (for memcmp in default mostly)
 * @return : 1 if A is greater, -1 is B is greater, 0 if equal
 */
typedef int32_t (*justCompareFunc)(const void* A,      const void* B, size_t SIZE);

/**
 * @brief : A visitor function / iterator
 * @param VALUE :
 * @param USER_DATA : Optional parameter to call along the iteration, although not const, just lib does not touch it
 */
typedef void    (*justVisitorFunc)(const void* VALUE,  void* USER_DATA);


/// @brief : Structure of an AVL tree, a NODE
typedef struct justAVLNode
{
  struct justAVLNode*          left;   ///< Left child
  struct justAVLNode*          right;  ///< Right child
  int32_t                       height; ///< height of th etree
  alignas(max_align_t) uint8_t  data[]; ///< data as an array, to ensure that it is on the same memory location as the entire struct, that data* does not give me
} JustAVLNode;

/// @brief : Structure of an AVL Trree, the view of the entire tree
typedef struct justAVLTree
{
  JustAVLNode*         root;         ///< The root node
  size_t                size;         ///< How many nodes
  size_t                elementSize;  ///< How big is an element in bytes
  justCompareFunc      compare;      ///< Compare function
  const char*           tag;          ///< Why was this tree created
  JustLinearAllocator* allocator;    ///< Optional linear allocator
} JustAVLTree;

typedef JustAVLTree OrderedSet;

/**
 * @brief : Initializes an AVL Tree (Ordered Set).
 * @param TREE : Pointer to AVLTree struct.
 * @param ELEMENT_SIZE : Size of each element in bytes.
 * @param COMPARATOR : Comparison callback (or NULL for default memcmp).
 * @param ALLOCATOR : Optional linear allocator (NULL for heap).
 * @param TAG : Tracking tag for memory tracker.
 * @return : true on success, false on allocation failure.
 */
JUST_API bool justOrderedSetCreate(
  JustAVLTree*         TREE,
  size_t                ELEMENT_SIZE,
  justCompareFunc      COMPARATOR,
  JustLinearAllocator* ALLOCATOR,
  const char*           TAG);

/**
 * @brief : Destroys and cleans up a tree
 * @param TREE : Pointer to the tree
*/
JUST_API void justOrderedSetDestroy(JustAVLTree* TREE);

/**
 * @param : Inserts a value to the orderedSet
 * @param TREE : Pointer to the orderedSet
 * @param VALUE_PTR : Pointer to the value to be inserted
 * @return : true if inserted, false otherwise
*/
JUST_API bool justOrderedSetInsert(JustAVLTree* TREE, const void* VALUE_PTR);

/**
 * @param : Removes a value from the orderedSet
 * @param TREE : Pointer to the orderedSet
 * @param VALUE_PTR : Pointer to the value to be removed
 * @return : true if removed, false otherwise
*/
JUST_API bool justOrderedSetRemove(JustAVLTree* TREE, const void* VALUE_PTR);

/**
 * @param : Inserts a value to the orderedSet
 * @param TREE : Pointer to the orderedSet
 * @param VALUE_PTR : Pointer to the value to be inserted
 * @return : true if inserted, false otherwise
*/
JUST_API void* justOrderedSetFind(const JustAVLTree* TREE, const void* VALUE_PTR);

/**
 * @param : Traverses a tree in order
 * @param TREE : Pointer to the orderedSet
 * @param VISITOR : Pointer to the visitor function
 * @param USER_DATA : Optional user data to be used while visiting, although not const, the library does not touch it
*/
JUST_API void justOrderedSetTraverseInorder(const JustAVLTree* TREE, justVisitorFunc VISITOR, void* USER_DATA);

/**
 * @brief : Clear out an orderedSet
 * @param TREE : Pointer to the tree
*/
JUST_API void justOrderedSetClear(JustAVLTree* TREE);

/**
 * @brief : Tells whether the set contains an item
 * @param TREE : Pointer to the set
 * @param VALUE_PTR : Pointer to the item to search
 * return : true if the item is in the set, false otherwise
*/
JUST_API static inline bool justOrderedSetContains(const JustAVLTree* TREE, const void* VALUE_PTR)
{
  JUST_ASSERT_DEBUG_MESSAGE(TREE != NULL, "[ORDERED SET] : TREE cannot be NULL");
  JUST_ASSERT_DEBUG_MESSAGE(VALUE_PTR != NULL, "[ORDERED SET] : VALUE_PTR cannot be NULL");

  return (justOrderedSetFind(TREE, VALUE_PTR) != NULL);
}

/**
 * @brief : Tells the size of an orderedSet
 * @param TREE : Pointer to the tree whose size is to be checked
 * @return : size in terms of number of nodes
*/
JUST_API static inline size_t justOrderedSetSize(const JustAVLTree* TREE)
{
  JUST_ASSERT_DEBUG_MESSAGE(TREE != NULL, "[ORDERED SET] : TREE cannot be NULL for checking size");

  return TREE->size;
}

/**
 * @brief : Tells whether the set is empty
 * @param TREE : Pointer to the tree whose size is to be checked
 * @return : true if set is empty, false otherwise
*/
JUST_API static inline bool justOrderedSetIsEmpty(const JustAVLTree* TREE)
{
  JUST_ASSERT_DEBUG_MESSAGE(TREE != NULL, "[ORDERED SET] : TREE cannot be NULL for checking emptiness");

  return (TREE->size == 0);
}


// - - - Ergonomic Macros - - -

#define JUST_ORDERED_SET_INIT(TREE_PTR, TYPE) \
  justOrderedSetCreate((TREE_PTR), sizeof(TYPE), NULL, NULL, "ORDERED_SET")

#define JUST_ORDERED_SET_INIT_TAGGED(TREE_PTR, TYPE, TAG) \
  justOrderedSetCreate((TREE_PTR), sizeof(TYPE), NULL, NULL, (TAG))

#define JUST_ORDERED_SET_INSERT_VAL(TREE_PTR, TYPE, VAL) \
  justOrderedSetInsert((TREE_PTR), &(TYPE){(VAL)})

#define JUST_ORDERED_SET_REMOVE_VAL(TREE_PTR, TYPE, VAL) \
  justOrderedSetRemove((TREE_PTR), &(TYPE){(VAL)})

#define JUST_ORDERED_SET_CONTAINS_VAL(TREE_PTR, TYPE, VAL) \
  justOrderedSetContains((TREE_PTR), &(TYPE){(VAL)})

#define JUST_ORDERED_SET_FIND_VAL(TREE_PTR, TYPE, VAL) \
  ((TYPE*)justOrderedSetFind((TREE_PTR), &(TYPE){(VAL)}))

#define JUST_ORDERED_SET_FIND_AS(TREE_PTR, TYPE, PTR) \
  ((TYPE*)justOrderedSetFind((TREE_PTR), (PTR)))

#ifdef __cplusplus
}
#endif
