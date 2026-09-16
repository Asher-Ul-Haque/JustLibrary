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
 * @file logger.h 
 * @brief provides logging utilities to any program, have colored logging output, severity based coloring etc
 * 
 * @warning use stdout and not thread safe, if you want logging in a file, pipe stdout to a file 
 * @see logger.c 
*/

#pragma once

#include <justUtils/defines.h>

#ifdef __cplusplus
extern "C" {
#endif


// - - - | Log macros and defs | - - - 


// - - - Enables - - -

/// @brief The DEBUG macro turns thorough debugging on by default
#ifdef DEBUG

  #ifndef LOG_WARNING_ENABLED 
    #define LOG_WARNING_ENABLED
  #endif

  #ifndef LOG_DEBUG_ENABLED
    #define LOG_DEBUG_ENABLED
  #endif

  #ifndef LOG_TRACE_ENABLED
    #define LOG_TRACE_ENABLED
  #endif

  #ifndef LOG_INFO_ENABLED
    #define LOG_INFO_ENABLED
  #endif

#endif 

#ifndef PRINT_LOG_TYPES
  #define PRINT_LOG_TYPES 1
#endif

#ifndef PRINT_LOG_COLORS
  #define PRINT_LOG_COLORS 1
#endif


// - - - Log Levels - - -

/// @brief Represents log level, internally
typedef enum LogLevel 
{
  LOG_LEVEL_FATAL   =   0,      /// @brief Just give up and die 
  LOG_LEVEL_ERROR   =   1,      /// @brief Something went wrong badly
  LOG_LEVEL_WARNING =   2,      /// @brief Are you sure about that
  LOG_LEVEL_INFO    =   3,      /// @brief Just some information
  LOG_LEVEL_DEBUG   =   4,      /// @brief Debugging information
  LOG_LEVEL_TRACE   =   5       /// @brief Trace every step, be verbose
} LogLevel;


// - - - API Controls - - -

JUST_API void justLogOutput(LogLevel LEVEL, const char* MESSAGE, ...); // - - - Multivariate, takes any number of arguments greater than 1

static inline void justLogNoOp(const char* NOTHING, ...) { (void)NOTHING; }

// - - - Fatal log
#define JUST_LOG_FATAL(...) justLogOutput(LOG_LEVEL_FATAL, __VA_ARGS__, "%s", "");

// - - - Error log
#define JUST_LOG_ERROR(...) justLogOutput(LOG_LEVEL_ERROR, __VA_ARGS__, "%s", "");

// - - - For the rest, define only when enabled, else define to nothingness
#ifdef LOG_WARNING_ENABLED
  #define JUST_LOG_WARNING(...) justLogOutput(LOG_LEVEL_WARNING, __VA_ARGS__, "%s", "");
#else
  #define JUST_LOG_WARNING(...) justLogNoOp(__VA_ARGS__)
#endif

#ifdef LOG_INFO_ENABLED
  #define JUST_LOG_INFO(...) justLogOutput(LOG_LEVEL_INFO, __VA_ARGS__, "%s", "")
#else
  #define JUST_LOG_INFO(...)  justLogNoOp(__VA_ARGS__)
#endif

#ifdef LOG_DEBUG_ENABLED
  #define JUST_LOG_DEBUG(...) justLogOutput(LOG_LEVEL_DEBUG, __VA_ARGS__, "%s", "")
#else
  #define JUST_LOG_DEBUG(...) justLogNoOp(__VA_ARGS__)
#endif

#ifdef LOG_TRACE_ENABLED
  #define JUST_LOG_TRACE(...) justLogOutput(LOG_LEVEL_TRACE, __VA_ARGS__, "%s", "")
#else
  #define JUST_LOG_TRACE(...) justLogNoOp(__VA_ARGS__)
#endif

#define JUST_LOG_CLEAR() printf("\033[H\033[J")


#ifdef __cplusplus
}
#endif
