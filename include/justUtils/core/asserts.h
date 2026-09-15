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
  * @file asserts.hpp
  * @brief Assertion and TODO helper macros used throughout the utils module.
  *
  * Provides compile-time assertions, runtime assertion checks, and TODO
  * markers that log an error before terminating the program.
*/

#pragma once

#include <justUtils/defines.h>
#include <stdnoreturn.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


// - - - Assert Methods - - -

/**
 * @brief : Internal forge library function to report an assert based on where it happened 
 * @param EXPRESSION : what was the expression being asserted 
 * @param MESSAGE : the message to be logged in case assert failed 
 * @param FILE : which file was the expression in 
 * @param FUNCTION : which function was the expression in, if any 
 * @param LINE : what line was the expression on
 */
JUST_API _Noreturn void justReportAssertionFailure(
  const char* EXPRESSION, 
  const char* MESSAGE, 
  const char* FILE, 
  const char* FUNCTION, 
  size_t      LINE);

/**
 * @brief Internal forge library function to report a TODO 
 * @param COMMENT the message to be logged in case TODO was executed
 * @param FILE which file was the TODO in
 * @param FUNCTION which function was the TODO in, if any 
 * @param LINE what line was the TODO on
 */
JUST_API _Noreturn void reportTODO(
  const char* COMMENT, 
  const char* FILE, 
  const char* FUNCTION, 
  size_t      LINE);

/**
  * @brief Compile-time assertion macro.
  *
  * Evaluates a condition during compilation and produces a compiler error
  * if the condition is false. Used to enforce invariants that must hold
  * at compile time.
  *
  * @see Usage: 
  ```C
    JUST_COMPILE_TIME_ASSERT(sizeof(myStruct) < 8);
  ```
*/
#define JUST_COMPILE_TIME_ASSERT(EXPRESSION)                   _Static_assert(EXPRESSION, "Compile-time assertion failed: " #EXPRESSION)
#define JUST_COMPILE_TIME_ASSERT_MESSAGE(EXPRESSION, MESSAGE)  _Static_assert(EXPRESSION, #MESSAGE)

/**
  * @brief Runtime assertion check.
  *
  * Evaluates the expression at runtime. If the expression evaluates to
  * `false`, a fatal log message is emitted and the program aborts.
  *
  * @param EXPRESSION Boolean expression that must evaluate to true.
  *
  * @see Usage: 
  ```C 
  JUST_ASSERT(value < 32 && myStruct.isValid == true);
  ```
*/
#define JUST_ASSERT(EXPRESSION)                                                              \
  do                                                                                          \
  {                                                                                           \
    if (EXPRESSION){}                                                                         \
    else                                                                                      \
    {                                                                                         \
      justReportAssertionFailure(#EXPRESSION, "Assert Fail\t", __FILE__, __func__, __LINE__); \
    }                                                                                         \
  }                                                                                           \
  while(0)

/**
  * @brief Runtime assertion check with custom message.
  *
  * Same as `RUNTIME_ASSERT` but allows attaching additional diagnostic
  * information to the log output.
  *
  * @param EXPRESSION Boolean expression that must evaluate to true.
  * @param MESSAGE Custom message describing the failure.
  *
  * @see Usage:
  ```C 
  JUST_ASSERT_MESSAGE(value < 32, "Value must be less than 32");
  ```
*/
#define JUST_ASSERT_MESSAGE(EXPRESSION, MESSAGE)                                 \
  do                                                                              \
  {                                                                               \
    if (EXPRESSION){}                                                             \
    else                                                                          \
    {                                                                             \
    justReportAssertionFailure(#EXPRESSION, MESSAGE, __FILE__, __func__, __LINE__);   \
    }                                                                             \
  }                                                                               \
  while (0)

#ifdef DEBUG

/**
 * @brief TODO, haven't written your code? just write TODO 
 * @warning crashes the program on execution in debug mode, 
 * @warning wont allow the program to compile in release mode, fix todos before you make a release
 *
 * @see DEBUG 
 * @see Usage: 
 ```C
 TODO;
 ```
*/
  #define TODO reportTODO(NULL,    __FILE__, __func__, __LINE__);

/**
 * @brief TODO, haven't written your code? just write TODO, but with a comment
 *
 * @param COMMENT, the comment to print if execution reaches TODO
 * @warning crashes the program on execution in debug mode, 
 * @warning wont allow the program to compile in release mode, fix todos before you make a release
 *
 * @see DEBUG 
 * @see Usage: 
 ```C
 TODO_COMMENT("Finish the forge library before going too ther projects";
 ```
*/
  #define TODO_COMMENT(COMMENT) reportTODO(COMMENT, __FILE__, __func__, __LINE__);

/**
 * @brief JUST_ASSERT but in debug mode 
 * @see JUST_ASSERT
*/
  #define JUST_ASSERT_DEBUG(EXPRESSION) JUST_ASSERT(EXPRESSION)

/**
 * @brief JUST_ASSERT_MESSAGE but in debug mode 
 * @see JUST_ASSERT_MESSAGE
*/
  #define JUST_ASSERT_DEBUG_MESSAGE(EXPRESSION, MESSAGE) JUST_ASSERT_MESSAGE(EXPRESSION, MESSAGE)

#else
  #define TODO JUST_COMPILE_TIME_ASSERT_MESSAGE(0, "Unresolved TODO detected in release build");

  #define TODO_COMMENT(COMMENT) JUST_COMPILE_TIME_ASSERT_MESSAGE(0, "Unresolved TODO : " COMMENT);

  #define JUST_ASSERT_DEBUG(EXPRESSION)  \
    do                                    \
    {                                     \
      (void) (EXPRESSION);                \
    }                                     \
    while (0)

  #define JUST_ASSERT_DEBUG_MESSAGE(EXPRESSION, MESSAGE) \
    do                                                    \
    {                                                     \
      (void) (EXPRESSION);                                \
      (void) (MESSAGE);                                   \
    }                                                     \
    while (0)

#endif 


#ifdef __cplusplus
}
#endif
