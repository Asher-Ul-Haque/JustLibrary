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
 * @file testManager.h 
 * @brief Sequential unit test runner with process-isolation crash detection.
 */

#pragma once

#include <justUtils/defines.h>
#include <justUtils/testing/expect.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JUST_MAX_TESTS_PER_GROUP 256
#define JUST_MAX_TEST_GROUPS     32

typedef JustTestResult (*justTestFunc)(void);

/**
 * @brief : Registers a test to be executed sequentially.
 * @param TEST_FUNC : Function returning justTestResult.
 * @param DESCRIPTION : Short description of the test.
 * @param GROUP_ID : Logical cluster/category index (0 to JUST_MAX_TEST_GROUPS - 1).
 */
JUST_API void justTestRegister(
  justTestFunc TEST_FUNC, 
  const char*   DESCRIPTION, 
  uint8_t       GROUP_ID);

/**
 * @brief : Runs all registered tests across all groups and outputs a formatted summary.
 * @return : Total number of failed + crashed tests (0 indicates clean suite).
 */
JUST_API size_t justTestRunAll(void);

#ifdef __cplusplus
}
#endif
