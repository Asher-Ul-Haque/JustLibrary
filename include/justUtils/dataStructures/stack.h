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
 * @file : stack.h 
 * @brief : Stack implementation using dynamicArray 
 * @see : dynamicArray.h
 */

#pragma once 

#include <justUtils/defines.h>
#include <justUtils/dataStructures/dynamicArray.h>
#include <justUtils/core/asserts.h>
#include <justUtils/core/logger.h>

#ifdef __cplusplus
  extern "C" {
#endif

/// @brief : Stack just has an underlying dynamic Array 
typedef struct justStack 
{
  JustDynamicArray array; ///< The underlying dynamic Array
} JustStack;

/**
 * @brief : Initializes a stack instance.
 * @param STACK : The stack to create 
 * @param INITIAL_CAPACITY : The initial capacity of the stack 
 * @param ELEMENT_SIZE : The size of one element in the stack 
 * @param ALLOCATOR : Optional linear allocator
 * @return : True if successful, false if not
*/
JUST_API static inline bool justStackCreate(
  JustStack*           STACK,
  size_t                INITIAL_CAPACITY,
  size_t                ELEMENT_SIZE,
  JustLinearAllocator* ALLOCATOR,
  const char*           TAG)
{
  JUST_ASSERT_DEBUG_MESSAGE(STACK != NULL, "[STACK] : Cannot create a NULL STACK");
  JUST_ASSERT_DEBUG_MESSAGE(ELEMENT_SIZE > 0, "[STACK] : Element size must be greater than 0");

  return justDynamicArrayCreate(&STACK->array, INITIAL_CAPACITY, ELEMENT_SIZE, ALLOCATOR, TAG);
}

/**
 * @brief : Destroys the stack and frees internal resources.
 * @param STACK : The stack to destroy 
 */
JUST_API static inline void justStackDestroy(JustStack* STACK) 
{
  JUST_ASSERT_DEBUG_MESSAGE(STACK != NULL, "[STACK] : Cannot destroy a NULL STACK");
  justDynamicArrayDestroy(&STACK->array); 
}

/**
 * @brief : Pushes an element onto the top of the stack (O(1)).
 * @param STACK : The stack to push into 
 * @param VALUE_PTR : The value to push
 * @return : True if successful, false if not
*/
JUST_API static inline bool justStackPush(JustStack* STACK, const void* VALUE_PTR)
{
  JUST_ASSERT_DEBUG_MESSAGE(STACK != NULL, "[STACK] : Cannot push to a NULL stack");
  JUST_ASSERT_DEBUG_MESSAGE(VALUE_PTR != NULL, "[STACK] : Cannot push a NULL VALUE_PTR to a stack");

  return justDynamicArrayPush(&STACK->array, VALUE_PTR);
}

/**
 * @brief : Reserves a slot at the top and returns a direct pointer to uninitialized element memory.
 * @param STACK : The stack to be emplaced
 * @return : Pointer to uninitialized data
 * @warning: The element is uninitialized, use the pointer to Initialize
 */
JUST_API static inline void* justStackEmplace(JustStack* STACK)
{
  JUST_ASSERT_DEBUG_MESSAGE(STACK != NULL, "[STACK] : Cannot emplace in a NULL STACK");

  return justDynamicArrayEmplace(&STACK->array);
}

/**
 * @brief : Pops the top element from the stack (O(1)).
 * @param STACK : The stack to pop from 
 * @param OUT_VALUE_PTR : Optional pointer to store the value in
 * @return : True if successful, False if not
 */
JUST_API static inline bool justStackPop(JustStack* STACK, void* OUT_VALUE_PTR) 
{
  JUST_ASSERT_DEBUG_MESSAGE(STACK != NULL, "[STACK] : Cannot pop from a NULL ARRAY");

  if (STACK->array.size == 0)
  {
    JUST_LOG_WARNING("[STACK] : Cannot pop, stack is empty");
    return false;
  }

  return justDynamicArrayPop(&STACK->array, OUT_VALUE_PTR);
}

/**
 * @brief : Views the top element without removing it.
 * @param STACK : The stack to peek from 
 * @return : Pointer to the top of the stack
 */
JUST_API static inline void* justStackPeek(const JustStack* STACK) 
{
  JUST_ASSERT_DEBUG_MESSAGE(STACK != NULL, "[STACK] : Cannot peek into a NULL STACK");

  if (STACK->array.size == 0) return NULL;
  return justDynamicArrayAt(&STACK->array, STACK->array.size - 1);
}

/**
 * @brief : Returns current element count.
 * @param STACK : The stack whose size is to be known
 * @return : how many elements in the stack
 */
JUST_API static inline size_t justStackSize(const JustStack* STACK) 
{
  JUST_ASSERT_DEBUG_MESSAGE(STACK != NULL, "[STACK] : Cannot check size of a NULL STACK");
  return STACK->array.size; 
}

/**
 * @brief : Checks if stack is empty.
 * @param STACK : The stack to check 
 * @return : True if stack empty, False if not
 */
JUST_API static inline bool justStackIsEmpty(const JustStack* STACK) 
{
  JUST_ASSERT_DEBUG_MESSAGE(STACK != NULL, "[STACK] : Cannot check if a NULL STACK is empty");
  return STACK->array.size == 0; 
}


// - - - Ergonomic & Type-Safe Macros - - -

/// @brief Direct creation of stack from type
#define JUST_STACK_INIT_TAGGED(STACK_PTR, CAPACITY, TYPE, TAG) \
  justStackCreate((STACK_PTR), (CAPACITY), sizeof(TYPE), (NULL), (TAG))

/// @brief Stack creation but without a tag
#define JUST_STACK_INIT(STACK_PTR, CAPACITY, TYPE) JUST_STACK_INIT_TAGGED(STACK_PTR, CAPACITY, TYPE, "STACK")

/// @brief Direct typed top element inspection: *JUST_STACK_TOP(s, MyType)
#define JUST_STACK_TOP(STACK_PTR, TYPE) \
  (&((TYPE*)(STACK_PTR)->array.data)[(STACK_PTR)->array.size - 1])

/// @brief Zero-copy emplace onto stack
#define JUST_STACK_EMPLACE(STACK_PTR, TYPE) \
  ((TYPE*) justStackEmplace(STACK_PTR))

/// @brief Type-safe push by value
#define JUST_STACK_PUSH_VAL(STACK_PTR, TYPE, VALUE) \
  JUST_DARRAY_PUSH_VAL(&(STACK_PTR)->array, TYPE, (VALUE))

#ifdef __cplusplus
}
#endif
