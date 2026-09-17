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
 * @file : dynamicArray.h 
 * @brief : Dynamic Array implementation in C
*/

#pragma once 

#include <justUtils/defines.h>
#include <justUtils/memory/linearAlloc.h>
#include <justUtils/core/asserts.h>
#include <justUtils/core/logger.h>
#include <stdint.h>
#include <memory.h>

#ifdef __cplusplus 
extern "C" {
#endif 

#define JUST_DARRAY_DEFAULT_GROWTH_CAPACITY 8

/// @brief Dynamic Array : similar to std::vector in c++
typedef struct justDynamicArray
{
  uint8_t*                data;         ///< POinter to contiguois element memory 
  size_t                  capacity;     ///< Total number of elements allocated
  size_t                  size;         ///< Current number of elements stored
  size_t                  elementSize;  ///< Size of an individual element in bytes
  const char*             tag;          ///< Why are you creating this
  JustLinearAllocator*    allocator;    ///< Optional custom linear allocator, NULL for the vector to manage its own memory
} JustDynamicArray;


// - - - C API - - - 

/**
 * @brief : Creates a dynamic array instance.
 * @param DARRAY: Pointer to dynamic array struct 
 * @param INITIAL_CAPACITY : Initial element capacity (0 defaults to 8)
 * @param ELEMENT_SIZE : Size of each element in bytes 
 * @param ALLOCATOR : Pointer to linear allocator or NULL for the dynamic array to allocate memory on its own 
 * @return true if initialized successfully, false otherwise
*/
JUST_API bool justDynamicArrayCreate(
  JustDynamicArray*    DARRAY, 
  size_t               INITIAL_CAPACITY, 
  size_t               ELEMENT_SIZE, 
  JustLinearAllocator* ALLOCATOR,
  const char*          TAG);

/**
 * @brief : Destroys the dynamic array and releases memory if owned. 
 * @param DARRAY : Pointer to the dynamic array to be destroyed
*/
JUST_API void justDynamicArrayDestroy(JustDynamicArray* DARRAY);

/**
 * @brief : Ensures capacity exists for at least MIN_CAPACITY elements.
 * @param DARRAY : The array to be reserved 
 * @param MIN_CAPACITY : How much to reserve as count of elements 
 * @return : True if successfull and false if not
*/
JUST_API bool justDynamicArrayReserve(JustDynamicArray* DARRAY, size_t MIN_CAPACITY);

/**
 * @brief : Internal slow-path growth function
 * @warning: Internal function
 * @param DARRAY : The array to be grown
 * @return : True if succesful and false if not
 */
bool __justDynamicArrayGrow(JustDynamicArray* DARRAY);

/**
 * @brief : Appends a contigous range of elements via a single block memcpy
 * @param DARRAY : The dynamic array to which the range is to be pushed.
 * @param SRC_BUFFER : Pointer to the source to be copied
 * @param COUNT : How many elements in the buffer
 * @return : True if succesful, false if not
 */
JUST_API bool justDynamicArrayPushRange(JustDynamicArray* DARRAY, const void* SRC_BUFFER, size_t COUNT);

/**
 * @brief : Reserves a slot at the end and returns a direct pointer to unitialized element memory. Enables zero-copy costruction directly into array storage, and bypasses memcpy
 * @param DARRAY : The dynamic array pointer
 * @return : Pointer to the unitialized element
 * @warning : Does not intialize the element, use the pointer to initialize
 */
JUST_API void* justDynamicArrayEmplace(JustDynamicArray* DARRAY);

/**
 * @brief : Pushes a new element value to the back of the array.
 * @param DARRAY : The dynamic array to which the value is to be pushed 
 * @param VALUE_PTR : Pointer to the value being stored
 * @warning : VALUE_PTR's value will be copied
 * @return : True if push was succesful, false if not 
*/
JUST_API static inline bool justDynamicArrayPush(JustDynamicArray* DARRAY, const void* VALUE_PTR)
{
  JUST_ASSERT_DEBUG_MESSAGE(DARRAY != NULL, "[DYNAMIC DARRAY] : Cannot push into null DARRAY");
  JUST_ASSERT_DEBUG_MESSAGE(VALUE_PTR != NULL, "[DYNAMIC DARRAY] : Cannot push a null VALUE_PTR");

  void* slot = justDynamicArrayEmplace(DARRAY);
  if (!slot) return false;

  memcpy(slot, VALUE_PTR, DARRAY->elementSize);
  return true;
}

/**
 * @brief : Pops the last element from the array 
 * @param OUT_VALUE_PTR : Optional pointer to receive the popped element bytes
 * @warning : the size of OUT_VALUE_PTR should be big enough to store the element
 * @return : whether the pop was successfull
*/
JUST_API static inline bool justDynamicArrayPop(JustDynamicArray* DARRAY, void* OUT_VALUE_PTR)
{
  JUST_ASSERT_DEBUG_MESSAGE(DARRAY != NULL, "[DYNAMIC DARRAY] : Cannot Pop from a NULL DARRAY");

  if (DARRAY->size == 0)
  {
    JUST_LOG_WARNING("[DYNAMIC DARRAY] : The size of the DARRAY is 0, cannot pop");
    return false;
  }

  DARRAY->size--;
  if (OUT_VALUE_PTR)
  {
    uint8_t* source = DARRAY->data + (DARRAY->size * DARRAY->elementSize);
    memcpy(OUT_VALUE_PTR, source, DARRAY->elementSize);
  }

  return true;
}

/**
 * @brief : Returns a pointer to the element at the given index 
 * @param DARRAY : Pointer to the array which is to be accessed
 * @param INDEX : The index to access at 
 * @warning : The index must be in bounds
 * @warning : Since this returns a void*, you can override it directly, but be careful, since you get access to the memory underneath
 * @return : A pointer to the object in the array at the given index
*/
JUST_API static inline void* justDynamicArrayAt(const JustDynamicArray* DARRAY, size_t INDEX)
{
  JUST_ASSERT_DEBUG_MESSAGE(DARRAY != NULL, "[DYNAMIC DARRAY] : Cannot access a NULL DARRAY");
  JUST_ASSERT_DEBUG_MESSAGE(INDEX < DARRAY->size, "[DYNAMIC DARRAY] : INDEX out of bounds");

  return (void*) (DARRAY->data + (INDEX * DARRAY->elementSize));
}

/**
 * @brief : Clears all elements without freeing memory.
 * @param DARRAY : A pointer to the array to be cleared
*/
JUST_API static inline void justDynamicArrayClear(JustDynamicArray* DARRAY)
{
  JUST_ASSERT_DEBUG_MESSAGE(DARRAY != NULL, "[DYNAMIC DARRAY] : Cannot clear a NULL DARRAY");
  DARRAY->size = 0;
}

/**
 * @brief : Gets the size of the array
 * @param DARRAY : Pointer to the dynamic array
 * @return : size of the array in terms of how many elements
 */
JUST_API static inline size_t justDynamicArraySize(const JustDynamicArray* DARRAY)
{
  JUST_ASSERT_DEBUG_MESSAGE(DARRAY != NULL, "[DYNAMIC DARRAY] : Cannot size a NULL DARRAY");
  return DARRAY->size;
}

/**
 * @brief : Gets the capacity of the array
 * @param DARRAY : Pointer to the dynamic array
 * @return : capacity of the array in terms of how many elements
 */
JUST_API static inline size_t justDynamicArrayCapacity(const JustDynamicArray* DARRAY)
{
  JUST_ASSERT_DEBUG_MESSAGE(DARRAY != NULL, "[DYNAMIC DARRAY] : Cannot check capacity of a NULL DARRAY");
  return DARRAY->capacity;
}

/**
 * @brief : Tells whether the array is empty
 * @param DARRAY : Pointer to the dynamic array
 * @return : true if empty, false otherwise
 */
JUST_API static inline bool justDynamicArrayIsEmpty(const JustDynamicArray* DARRAY)
{
  return (DARRAY->size == 0);
}

/**
 * @brief : Shrinks the array on demand
 * @param DARRAY : Pointer to the dynamic array to be shrunk
 * @return : True if the array shrunk, false otherwise
 */
JUST_API bool justDynamicArrayShrinkToFit(JustDynamicArray* DARRAY);


// - - - Helper Macros for Ergonomic Usage - - - 

/**
 * @brief : Helper macro to initialize array with implicit type sizing 
 * @see dynamicArrayCreate
*/
#define JUST_DARRAY_INIT_TAGGED(DARRAY_PTR, CAPACITY, TYPE, TAG) \
  justDynamicArrayCreate((DARRAY_PTR), (CAPACITY), sizeof(TYPE), NULL, (TAG))

/**
 * @brief : Helper macro to initialize array with implicit type sizing 
 * @see dynamicArrayCreate
*/
#define JUST_DARRAY_INIT(DARRAY_PTR, CAPACITY, TYPE) JUST_DARRAY_INIT_TAGGED(DARRAY_PTR, CAPACITY, TYPE, "DYNAMIC DARRAY")

/// @brief : View the dynamic array as a standard C array. Like dynamicArray to []
#define JUST_DARRAY_DATA(DARRAY_PTR, TYPE) \
  ((TYPE*) (DARRAY_PTR)->data)

/**
 * @brief : Type-safe get element macro
 * @see : dynamicArrayAt
*/
#define JUST_DARRAY_GET(DARRAY_PTR, TYPE, INDEX) \
  (JUST_DARRAY_DATA(DARRAY_PTR, TYPE)[INDEX])

/// @brief : Direct zero-copy typed emplace
#define JUST_DARRAY_EMPLACE(DARRAY_PTR, TYPE) \
  ((TYPE*) justDynamicArrayEmplace(DARRAY_PTR))

/// @brief : Fast-path push value
#define JUST_DARRAY_PUSH_VAL(DARRAY_PTR, TYPE, VALUE)     \
  do                                                      \
  {                                                       \
    TYPE* _slot = JUST_DARRAY_EMPLACE(DARRAY_PTR, TYPE);  \
    if (_slot) *_slot = (VALUE);                          \
  } while(0)

/// @brief : Batch range append
#define JUST_DARRAY_PUSH_RANGE(DARRAY_PTR, SRC_PTR, COUNT) \
  justDynamicArrayPushRange((DARRAY_PTR), (const void*)(SRC_PTR), (COUNT))


#ifdef __cplusplus
}
#endif
