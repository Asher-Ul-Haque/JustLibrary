#include <justUtils/core/logger.h>
#include <justUtils/memory/tracker.h>
#include <justUtils/dataStructures/dynamicArray.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

bool justDynamicArrayCreate(
  justDynamicArray*    DARRAY,
  size_t                INITIAL_CAPACITY,
  size_t                ELEMENT_SIZE,
  justLinearAllocator* ALLOCATOR,
  const char*           TAG)
{
  JUST_ASSERT_DEBUG_MESSAGE(DARRAY != NULL, "[DYNAMIC DARRAY] : Cannot create a NULL DARRAY");
  JUST_ASSERT_DEBUG_MESSAGE(ELEMENT_SIZE > 0, "[DYNAMIC DARRAY] : Element size must be greater than 0");

  DARRAY->elementSize   = ELEMENT_SIZE;
  DARRAY->size          = 0;
  DARRAY->capacity      = INITIAL_CAPACITY;
  DARRAY->allocator     = ALLOCATOR;
  DARRAY->tag           = TAG;

  size_t totalBytes = DARRAY->capacity * DARRAY->elementSize;

  if (DARRAY->capacity == 0) return true;

  if (DARRAY->allocator) 
  {
    DARRAY->data = (uint8_t*) justLinearAllocAllocate(DARRAY->allocator, totalBytes, 0);
  }
  else 
  {
    DARRAY->data = (uint8_t*) JUST_MALLOC_TAGGED(totalBytes, TAG);
  }

  if (!DARRAY->data)
  {
    JUST_LOG_ERROR("[DYNAMIC DARRAY] : Failed to allocate memory for array!");
    DARRAY->capacity = 0;
    return false;
  }

  return true;
}

void justDynamicArrayDestroy(justDynamicArray* DARRAY)
{
  JUST_ASSERT_DEBUG_MESSAGE(DARRAY != NULL, "[DYNAMIC DARRAY] : Cannot destroy a NULL array.");

  if (DARRAY->data)
  {
    if (!DARRAY->allocator) JUST_FREE(DARRAY->data);
  }

  DARRAY->data         = NULL;
  DARRAY->capacity     = 0;
  DARRAY->size         = 0;
  DARRAY->elementSize  = 0;
  DARRAY->allocator    = NULL;
}

bool justDynamicArrayReserve(justDynamicArray* DARRAY, size_t MIN_CAPACITY)
{
  JUST_ASSERT_DEBUG_MESSAGE(DARRAY != NULL, "[DYNAMIC DARRAY] : Cannot reserve capcity in a NULL DARRAY");

  if (MIN_CAPACITY <= DARRAY->capacity) return true;

  size_t newCapacity = DARRAY->capacity * 2;
  if (newCapacity < MIN_CAPACITY) newCapacity = MIN_CAPACITY;

  size_t    newBytes  = newCapacity * DARRAY->elementSize;
  uint8_t*  newData   = NULL;

  // - - - allocate new chunk from Linear Allocator and copy existing data
  if (DARRAY->allocator)
  {
    newData = (uint8_t*) justLinearAllocAllocate(DARRAY->allocator, newBytes, 0);
    if (!newData)
    {
      JUST_LOG_ERROR("[DYNAMIC DARRAY] : Failed to allocate new chunks via Linear Allocator");
      return false;
    }
    memcpy(newData, DARRAY->data, DARRAY->size * DARRAY->elementSize);
  }
  else
  {
    if (DARRAY->data == NULL)  newData = JUST_MALLOC_TAGGED(newBytes, DARRAY->tag);
    else                      newData = JUST_REALLOC(DARRAY->data, newBytes);
  }

  if (!newData)
  {
    JUST_LOG_ERROR("[DYNAMIC DARRAY] : Failed to expenad array capacity!");
    return false;
  }

  DARRAY->data     = newData;
  DARRAY->capacity = newCapacity;
  return true;
}

bool __justDynamicArrayGrow(justDynamicArray* DARRAY)
{
  JUST_ASSERT_DEBUG(DARRAY != NULL);

  size_t targetCap = DARRAY->capacity ? (DARRAY->capacity * 2) : JUST_DARRAY_DEFAULT_GROWTH_CAPACITY;
  return justDynamicArrayReserve(DARRAY, targetCap);
}

bool justDynamicArrayPushRange(justDynamicArray* DARRAY, const void* SRC_BUFFER, size_t COUNT)
{
  JUST_ASSERT_DEBUG(DARRAY != NULL);
  if (!SRC_BUFFER || COUNT == 0) return true;

  size_t requiredCapacity = DARRAY->size + COUNT;
  if (requiredCapacity > DARRAY->capacity)
  {
    if (!justDynamicArrayReserve(DARRAY, requiredCapacity)) 
    {
      return false;
    }
  }

  uint8_t* dest = DARRAY->data + (DARRAY->size * DARRAY->elementSize);
  memcpy(dest, SRC_BUFFER, COUNT * DARRAY->elementSize);
  DARRAY->size += COUNT;

  return true;
}

bool justDynamicArrayShrinkToFit(justDynamicArray* DARRAY)
{
  JUST_ASSERT_DEBUG_MESSAGE(DARRAY != NULL, "[DYNAMIC DARRAY] : Cannot shrink a NULL DARRAY");

  // - - - Linear allocators cannot free or shrink intermediate allocations
  if (DARRAY->allocator) 
  {
    JUST_LOG_ERROR("[DYNAMIC DARRAY] : Cannot shrink an DARRAY that uses a linear allocator");
    return false;
  }

  if (DARRAY->size == DARRAY->capacity) return true;

  if (DARRAY->size == 0)
  {
    if (DARRAY->data)
    {
      JUST_FREE(DARRAY->data);
      DARRAY->data = NULL;
    }
    DARRAY->capacity = 0;
    return true;
  }

  size_t  newBytes  = DARRAY->size * DARRAY->elementSize;
  void*   newData   = NULL;
  if (DARRAY->data)  newData = JUST_REALLOC(DARRAY->data, newBytes);
  else              newData = JUST_MALLOC_TAGGED(newBytes, DARRAY->tag);

  if (!newData)
  {
    JUST_LOG_ERROR("[DYNAMIC DARRAY] : Failed to shrink array buffer!");
    return false;
  }

  DARRAY->data     = newData;
  DARRAY->capacity = DARRAY->size;
  return true;
}

JUST_API void* justDynamicArrayEmplace(justDynamicArray* DARRAY)
{
  JUST_ASSERT_DEBUG_MESSAGE(DARRAY != NULL, "[DYNAMIC DARRAY] : Cannot emplace in a NULL array");

  if (DARRAY->size >= DARRAY->capacity)
  {
    if (!__justDynamicArrayGrow(DARRAY)) return NULL;
  }

  void* slot = DARRAY->data + (DARRAY->size * DARRAY->elementSize);
  DARRAY->size++;
  return slot;
}
