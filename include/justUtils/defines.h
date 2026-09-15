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
 * @file defines.h 
 * @brief Core platform detection, Clang visibility attributes, and single-header guards.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


// - - - Platform Detection - - -

#define JUST_PLATFORM_UNKNOWN 0
#define JUST_PLATFORM_WINDOWS 1
#define JUST_PLATFORM_LINUX   2
#define JUST_PLATFORM_ANDROID 3
#define JUST_PLATFORM_APPLE   4

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
  #define JUST_PLATFORM JUST_PLATFORM_WINDOWS
#elif defined(__linux__) || defined(__gnu_linux__)
  #define JUST_PLATFORM JUST_PLATFORM_LINUX
#elif defined(__ANDROID__)
  #define JUST_PLATFORM JUST_PLATFORM_ANDROID
#elif defined(__APPLE__)
  #define JUST_PLATFORM JUST_PLATFORM_APPLE
#else
  #define JUST_PLATFORM JUST_PLATFORM_UNKNOWN
#endif


// - - - Symbol Visibility & Export Control - - -

#if JUST_PLATFORM == JUST_PLATFORM_WINDOWS
  #if defined(JUST_EXPORT)
    #define JUST_API __declspec(dllexport)
  #else
    #define JUST_API __declspec(dllimport)
  #endif
  #define JUST_LOCAL
#else
  #define JUST_API   __attribute__((visibility("default")))
  #define JUST_LOCAL __attribute__((visibility("hidden")))
#endif

#ifdef __cplusplus
}
#endif
