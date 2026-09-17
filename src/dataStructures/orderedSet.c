#include <justUtils/dataStructures/orderedSet.h>
#include <justUtils/memory/linearAlloc.h>
#include <justUtils/memory/tracker.h>
#include <justUtils/core/asserts.h>
#include <justUtils/core/logger.h>
#include <stdlib.h>
#include <string.h>

static int32_t defaultMemcmp(const void* A, const void* B, size_t SIZE)
{
  return memcmp(A, B, SIZE);
}

static inline int32_t nodeHeight(JustAVLNode* NODE)
{
  return NODE ? NODE->height : 0;
}

static inline int32_t maxInt(int32_t A, int32_t B)
{
  return (A > B) ? A : B;
}

static inline int32_t getBalance(JustAVLNode* NODE)
{
  return NODE ? (nodeHeight(NODE->left) - nodeHeight(NODE->right)) : 0;
}

static inline JustAVLNode* rotateRight(JustAVLNode* Y)
{
  JustAVLNode* x  = Y->left;
  JustAVLNode* t2 = x->right;

  x->right = Y;
  Y->left  = t2;

  Y->height = maxInt(nodeHeight(Y->left), nodeHeight(Y->right)) + 1;
  x->height = maxInt(nodeHeight(x->left), nodeHeight(x->right)) + 1;

  return x;
}

static inline JustAVLNode* rotateLeft(JustAVLNode* X)
{
  JustAVLNode* y  = X->right;
  JustAVLNode* t2 = y->left;

  y->left  = X;
  X->right = t2;

  X->height = maxInt(nodeHeight(X->left), nodeHeight(X->right)) + 1;
  y->height = maxInt(nodeHeight(y->left), nodeHeight(y->right)) + 1;

  return y;
}

static JustAVLNode* createNode(JustAVLTree* TREE, const void* VALUE_PTR) 
{
  size_t totalBytes = sizeof(JustAVLNode) + TREE->elementSize;

  JustAVLNode* node = TREE->allocator
                      ? (JustAVLNode*) justLinearAllocAllocate(TREE->allocator, totalBytes, alignof(JustAVLNode))
                      : (JustAVLNode*) JUST_MALLOC_TAGGED(totalBytes, TREE->tag);

  if (!node) 
  {
    JUST_LOG_ERROR("[ORDERED SET] : Failed to allocate node of %zu bytes", totalBytes);
    return NULL;
  }

  node->height = 1;
  node->left   = NULL;
  node->right  = NULL;
  memcpy(node->data, VALUE_PTR, TREE->elementSize);

  return node;
}

static inline void freeNode(JustAVLTree* TREE, JustAVLNode* NODE) 
{
  if (!NODE) return;
  if (!TREE->allocator) JUST_FREE(NODE);
}

static void destroySubtree(JustAVLTree* TREE, JustAVLNode* NODE) 
{
  if (!NODE) return;
  destroySubtree(TREE, NODE->left);
  destroySubtree(TREE, NODE->right);
  freeNode(TREE, NODE);
}

static JustAVLNode* insertRecursive(
  JustAVLTree* TREE,
  JustAVLNode* NODE,
  const void*  VALUE_PTR,
  bool*        OUT_INSERTED)
{
  if (!NODE)
  {
    *OUT_INSERTED = true;
    return createNode(TREE, VALUE_PTR);
  }

  int32_t cmp = TREE->compare(VALUE_PTR, NODE->data, TREE->elementSize);

  if (cmp < 0) 
  {
    NODE->left = insertRecursive(TREE, NODE->left, VALUE_PTR, OUT_INSERTED);
  } 
  else if (cmp > 0) 
  {
    NODE->right = insertRecursive(TREE, NODE->right, VALUE_PTR, OUT_INSERTED);
  } 
  else 
  {
    *OUT_INSERTED = false;
    return NODE;
  }

  NODE->height    = 1 + maxInt(nodeHeight(NODE->left), nodeHeight(NODE->right));
  int32_t balance = getBalance(NODE);

  // - - - Left Left Case
  if (balance > 1 && TREE->compare(VALUE_PTR, NODE->left->data, TREE->elementSize) < 0)
  {
    return rotateRight(NODE);
  }

  // - - - Right Right Case
  if (balance < -1 && TREE->compare(VALUE_PTR, NODE->right->data, TREE->elementSize) > 0)
  {
    return rotateLeft(NODE);
  }

  // - - - Left Right Case
  if (balance > 1 && TREE->compare(VALUE_PTR, NODE->left->data, TREE->elementSize) > 0)
  {
    NODE->left = rotateLeft(NODE->left);
    return rotateRight(NODE);
  }

  // - - - Right Left Case
  if (balance < -1 && TREE->compare(VALUE_PTR, NODE->right->data, TREE->elementSize) < 0)
  {
    NODE->right = rotateRight(NODE->right);
    return rotateLeft(NODE);
  }

  return NODE;
}

static JustAVLNode* minValueNode(JustAVLNode* NODE)
{
  JustAVLNode* curr = NODE;
  while (curr && curr->left) curr = curr->left;
  return curr;
}

static JustAVLNode* removeRecursive(
  JustAVLTree* TREE,
  JustAVLNode* ROOT,
  const void*  VALUE_PTR,
  bool*        OUT_REMOVED)
{
  if (!ROOT) 
  {
    *OUT_REMOVED = false;
    return NULL;
  }

  int32_t cmp = TREE->compare(VALUE_PTR, ROOT->data, TREE->elementSize);

  if (cmp < 0) 
  {
    ROOT->left = removeRecursive(TREE, ROOT->left, VALUE_PTR, OUT_REMOVED);
  } 
  else if (cmp > 0) 
  {
    ROOT->right = removeRecursive(TREE, ROOT->right, VALUE_PTR, OUT_REMOVED);
  } 
  else 
  {
    *OUT_REMOVED = true;

    if (!ROOT->left || !ROOT->right) 
    {
      JustAVLNode* temp = ROOT->left ? ROOT->left : ROOT->right;

      if (!temp) 
      {
        freeNode(TREE, ROOT);
        return NULL;
      } 
      else 
      {
        memcpy(ROOT->data, temp->data, TREE->elementSize);
        ROOT->left   = temp->left;
        ROOT->right  = temp->right;
        ROOT->height = temp->height;
        freeNode(TREE, temp);
      }
    } 
    else 
    {
      JustAVLNode* temp = minValueNode(ROOT->right);
      memcpy(ROOT->data, temp->data, TREE->elementSize);
      ROOT->right = removeRecursive(TREE, ROOT->right, temp->data, OUT_REMOVED);
    }
  }

  ROOT->height    = 1 + maxInt(nodeHeight(ROOT->left), nodeHeight(ROOT->right));
  int32_t balance = getBalance(ROOT);

  if (balance > 1 && getBalance(ROOT->left) >= 0)
  {
    return rotateRight(ROOT);
  }

  if (balance > 1 && getBalance(ROOT->left) < 0)
  {
    ROOT->left = rotateLeft(ROOT->left);
    return rotateRight(ROOT);
  }

  if (balance < -1 && getBalance(ROOT->right) <= 0)
  {
    return rotateLeft(ROOT);
  }

  if (balance < -1 && getBalance(ROOT->right) > 0)
  {
    ROOT->right = rotateRight(ROOT->right);
    return rotateLeft(ROOT);
  }

  return ROOT;
}

JUST_API bool justOrderedSetCreate(
  JustAVLTree*         TREE,
  size_t               ELEMENT_SIZE,
  justCompareFunc      COMPARATOR,
  JustLinearAllocator* ALLOCATOR,
  const char*          TAG)
{
  JUST_ASSERT_DEBUG_MESSAGE(TREE != NULL, "[ORDERED SET] : Target pointer cannot be NULL");
  JUST_ASSERT_DEBUG_MESSAGE(ELEMENT_SIZE > 0, "[ORDERED SET] : ELEMENT_SIZE must be > 0");

  TREE->root        = NULL;
  TREE->size        = 0;
  TREE->elementSize = ELEMENT_SIZE;
  TREE->compare     = COMPARATOR ? COMPARATOR : defaultMemcmp;
  TREE->allocator   = ALLOCATOR;
  TREE->tag         = TAG;

  return true;
}

JUST_API void justOrderedSetDestroy(JustAVLTree* TREE) 
{
  JUST_ASSERT_DEBUG_MESSAGE(TREE != NULL, "[ORDERED SET] : Cannot destroy a NULL TREE");
  destroySubtree(TREE, TREE->root);
  TREE->root = NULL;
  TREE->size = 0;
}

JUST_API bool justOrderedSetInsert(JustAVLTree* TREE, const void* VALUE_PTR)
{
  JUST_ASSERT_DEBUG(TREE != NULL && VALUE_PTR != NULL);

  bool inserted = false;
  TREE->root    = insertRecursive(TREE, TREE->root, VALUE_PTR, &inserted);
  if (inserted) TREE->size++;

  return inserted;
}

JUST_API bool justOrderedSetRemove(JustAVLTree* TREE, const void* VALUE_PTR)
{
  JUST_ASSERT_DEBUG(TREE != NULL && VALUE_PTR != NULL);
  if (!TREE->root) return false;

  bool removed  = false;
  TREE->root    = removeRecursive(TREE, TREE->root, VALUE_PTR, &removed);
  if (removed) TREE->size--;

  return removed;
}

JUST_API void* justOrderedSetFind(const JustAVLTree* TREE, const void* VALUE_PTR) 
{
  JUST_ASSERT_DEBUG(TREE != NULL && VALUE_PTR != NULL);

  JustAVLNode* curr = TREE->root;
  while (curr) 
  {
    int32_t cmp = TREE->compare(VALUE_PTR, curr->data, TREE->elementSize);

    if (cmp == 0)      return (void*)curr->data;
    else if (cmp < 0) curr = curr->left;
    else              curr = curr->right;
  }

  return NULL;
}

static void inorderRecursive(JustAVLNode* NODE, justVisitorFunc VISITOR, void* USER_DATA)
{
  if (!NODE) return;
  inorderRecursive(NODE->left, VISITOR, USER_DATA);
  VISITOR((const void*)NODE->data, USER_DATA);
  inorderRecursive(NODE->right, VISITOR, USER_DATA);
}

JUST_API void justOrderedSetTraverseInorder(
  const JustAVLTree* TREE,
  justVisitorFunc    VISITOR,
  void*              USER_DATA)
{
  JUST_ASSERT_DEBUG(TREE != NULL && VISITOR != NULL);
  inorderRecursive(TREE->root, VISITOR, USER_DATA);
}

JUST_API void justOrderedSetClear(JustAVLTree* TREE)
{
  JUST_ASSERT_DEBUG(TREE != NULL);
  destroySubtree(TREE, TREE->root);
  TREE->root = NULL;
  TREE->size = 0;
}
