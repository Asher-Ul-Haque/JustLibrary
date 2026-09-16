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
 * @file logger.c 
 * Implementation of logging utilities for forgelib 
 *
 * @see logger.h for usage 
 * @warning This file is internal implementation
 */

#include <justUtils/core/logger.h>
#include <justUtils/core/asserts.h>
#include <stdarg.h>
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>


// - - - Writing Controls - - - 


/**
 * @brief Writes a log to the console 
 * @param MESSAGE what to log 
 * @param COLOR the severity represented by a color 
 */
static void writeConsole(const char* MESSAGE, LogLevel COLOR)
{
  FILE*       stream    = NULL;
  const char* colorStr  = NULL;

  switch (COLOR)
  {
    case LOG_LEVEL_FATAL    : colorStr = "1;97;41"; stream = stderr; break;
    case LOG_LEVEL_ERROR    : colorStr = "1;31";    stream = stderr; break;
    case LOG_LEVEL_WARNING  : colorStr = "33";      stream = stdout; break;
    case LOG_LEVEL_INFO     : colorStr = "32";      stream = stdout; break;
    case LOG_LEVEL_DEBUG    : colorStr = "36";      stream = stdout; break;
    case LOG_LEVEL_TRACE    : colorStr = "90";      stream = stdout; break;
    default                 : colorStr = "0";       stream = stdout; break;
  }

  #if PRINT_LOG_COLORS == 1
    fprintf(stream, "\033[%sm%s\033[0m\n", colorStr, MESSAGE);
  #else 
    (void) colorStr;
    fprintf(stream, "%s\n", MESSAGE); // - - -This looks scary
  #endif
  /*Here is how it works:
  \033[     - This is the escape character
  %s        - This is the color code
  m         - This is the end of the color code
  %s        - This is the message
  \033[0m   - This is the end of the color
  This is how you print colored text in the terminal*/
}


// - - - API Controls - - -


/**
 * @brief constructs a output string and log it
 * @param LEVEL the log severity 
 * @param MESSAGE the main message given by the user 
 * @warning this function is internal to forge, do not use directly 
 * @see FORGE_LOG macros for usage
 */
JUST_API void justLogOutput(LogLevel LEVEL, const char* MESSAGE, ...)
{
  const int   messageLength     = 1024 * 4;
  char        outputMessage     [messageLength];
  memset(outputMessage, 0, sizeof(outputMessage));

  // - - - Add the rest of the arguments
  __builtin_va_list argumentPointer;
  va_start(argumentPointer, MESSAGE);
  vsnprintf(outputMessage, sizeof(outputMessage),
            MESSAGE, argumentPointer);
  va_end(argumentPointer);

  // - - - Prepend with level header
  char finalMessage[messageLength];
  #if PRINT_LOG_TYPES == 1
    const char* levelStrings[6]   = {"[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]: ", "[TRACE]: "};
    sprintf(finalMessage, "%s\t%s", levelStrings[LEVEL], outputMessage);
  #else 
    sprintf(finalMessage, "%s", outputMessage);
  #endif

  writeConsole(finalMessage, LEVEL);    
}


// - - - | Assert Functions | - - -


JUST_API _Noreturn void justReportAssertionFailure(const char* EXPRESSION, const char* MESSAGE, const char* FILE, const char* FUNCTION, size_t LINE)
{
  justLogOutput(LOG_LEVEL_FATAL,
            "ASSERT FAIL :     %s\n"
            "MESSAGE     :     %s\n"
            "LOCATION    :     file: %s at line: %zu in function: %s\n", 
            EXPRESSION, MESSAGE, FILE, LINE, FUNCTION);
  abort();
}

JUST_API _Noreturn void reportTODO(const char* COMMENT, const char* FILE, const char* FUNCTION, size_t LINE)
{
  justLogOutput(LOG_LEVEL_ERROR, "Oopsie!, you have a TODO!!!");
  justLogOutput(LOG_LEVEL_ERROR, "At: %s:%zu -> Function: %s", FILE, LINE, FUNCTION);
  if (COMMENT) justLogOutput(LOG_LEVEL_WARNING, "COMMENT: %s", COMMENT);
  abort();
}
