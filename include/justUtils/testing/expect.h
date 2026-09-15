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
 * @file expect.h 
 * @brief Test assertions and return status codes for justLibrary.
 */

#pragma once

#include <justUtils/core/logger.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief : Result of executing a test
typedef enum justTestResult
{
  JUST_TEST_FAIL = 0, ///< Test failed
  JUST_TEST_PASS = 1, ///< Test passed
  JUST_TEST_SKIP = 2  ///< Test skipped
} JustTestResult;


// - - - Assertion Macros - - -

#define JUST_EXPECT_TO_BE(EXPECTED, ACTUAL)                                                           \
  do                                                                                                  \
  {                                                                                                   \
    if ((EXPECTED) != (ACTUAL))                                                                       \
    {                                                                                                 \
      JUST_LOG_ERROR("  [EXPECT FAILED] %s != %s at %s:%d", #EXPECTED, #ACTUAL, __FILE__, __LINE__);  \
      return JUST_TEST_FAIL;                                                                          \
    }                                                                                                 \
  } while(0)

#define JUST_EXPECT_NOT_TO_BE(NOT_EXPECTED, ACTUAL)                                                           \
  do                                                                                                          \
  {                                                                                                           \
    if ((NOT_EXPECTED) == (ACTUAL))                                                                           \
    {                                                                                                         \
      JUST_LOG_ERROR("  [NOT_EXPECT FAILED] %s == %s at %s:%d", #NOT_EXPECTED, #ACTUAL, __FILE__, __LINE__);  \
      return JUST_TEST_FAIL;                                                                                  \
    }                                                                                                         \
  } while(0)

#define JUST_EXPECT_C_STRING_TO_BE(EXPECTED, ACTUAL)                                              \
  do                                                                                              \
  {                                                                                               \
    const char* _exp = (EXPECTED);                                                                \
    const char* _act = (ACTUAL);                                                                  \
    if (_exp == NULL || _act == NULL || strcmp(_exp, _act) != 0)                                  \
    {                                                                                             \
      JUST_LOG_ERROR("  [EXPECT FAILED] String mismatch: Expected \"%s\", got \"%s\" at %s:%d",   \
                      _exp ? _exp : "NULL", _act ? _act : "NULL", __FILE__, __LINE__);            \
      return JUST_TEST_FAIL;                                                                      \
    }                                                                                             \
  } while(0)

#define JUST_EXPECT_FLOAT_TO_BE(EXPECTED, ACTUAL, EPS)                                      \
  do                                                                                        \
  {                                                                                         \
    double _diff = (double)(EXPECTED) - (double)(ACTUAL);                                   \
    if (_diff < 0.0) _diff = -_diff;                                                        \
    if (_diff > (double)(EPS))                                                              \
    {                                                                                       \
      JUST_LOG_ERROR("  [EXPECT FAILED] Float mismatch: %s vs %s (Diff: %f > %f) at %s:%d", \
                      #EXPECTED, #ACTUAL, _diff, (double)(EPS), __FILE__, __LINE__);        \
      return JUST_TEST_FAIL;                                                                \
    }                                                                                       \
  } while(0)

#define JUST_EXPECT_TO_BE_NULL(PTR)                                                                 \
  do                                                                                                \
  {                                                                                                 \
    if ((PTR) != NULL)                                                                              \
    {                                                                                               \
      JUST_LOG_ERROR("  [EXPECT FAILED] Expected NULL for %s at %s:%d", #PTR, __FILE__, __LINE__);  \
      return JUST_TEST_FAIL;                                                                        \
    }                                                                                               \
  } while(0)

#define JUST_EXPECT_TO_BE_NOT_NULL(PTR)                                                                 \
  do                                                                                                    \
  {                                                                                                     \
    if ((PTR) == NULL)                                                                                  \
    {                                                                                                   \
      JUST_LOG_ERROR("  [EXPECT FAILED] Expected non-NULL for %s at %s:%d", #PTR, __FILE__, __LINE__);  \
      return JUST_TEST_FAIL;                                                                            \
    }                                                                                                   \
  } while(0)

#define JUST_EXPECT_TO_BE_TRUE(EXPR)                                                              \
  do                                                                                              \
  {                                                                                               \
    if (!(EXPR))                                                                                  \
    {                                                                                             \
      JUST_LOG_ERROR("  [EXPECT FAILED] Expected TRUE: %s at %s:%d", #EXPR, __FILE__, __LINE__);  \
      return JUST_TEST_FAIL;                                                                      \
    }                                                                                             \
  } while(0)

#define JUST_EXPECT_TO_BE_FALSE(EXPR)                                                             \
  do                                                                                              \
  {                                                                                               \
    if ((EXPR))                                                                                   \
    {                                                                                             \
      JUST_LOG_ERROR("  [EXPECT FAILED] Expected FALSE: %s at %s:%d", #EXPR, __FILE__, __LINE__); \
      return JUST_TEST_FAIL;                                                                      \
    }                                                                                             \
  } while(0)

#ifdef __cplusplus
}
#endif
