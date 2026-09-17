/**
 * @file exampleAsserts.c
 * @brief Demonstration of compile-time asserts, runtime asserts, and TODO traps.
 */

// Uncomment ONE of these to observe failure diagnostics and abort behavior:
// #define TRIGGER_RUNTIME_ASSERT
// #define TRIGGER_ASSERT_MESSAGE
// #define TRIGGER_DEBUG_ASSERT
// #define TRIGGER_TODO
// #define TRIGGER_TODO_COMMENT
// #define TRIGGER_COMPILE_TIME_FAIL

#include <justUtils/core/logger.h>
#include <justUtils/core/asserts.h>

// - - - Compile-Time Assertions (Evaluated at compilation)
JUST_COMPILE_TIME_ASSERT(sizeof(uint32_t) == 4);
JUST_COMPILE_TIME_ASSERT_MESSAGE(sizeof(uint64_t) == 8, "Expected 64-bit word size");

// - - - This stops compilation immediately if enabled:
#ifdef TRIGGER_COMPILE_TIME_FAIL
  JUST_COMPILE_TIME_ASSERT_MESSAGE(sizeof(int) == 128, "Forced compile-time failure demo");
#endif

int main(void)
{
  JUST_LOG_INFO("=== JustLibrary Asserts Demonstration ===");

  // - - - Passing Assertions (Proceeds without interruption)
  int32_t bufferCapacity = 64;
  int32_t activeElements = 12;

  JUST_ASSERT(bufferCapacity > 0);
  JUST_ASSERT_MESSAGE(activeElements <= bufferCapacity, "Active count must fit in capacity");
  JUST_ASSERT_DEBUG(activeElements >= 0);
  JUST_ASSERT_DEBUG_MESSAGE(bufferCapacity == 64, "Initial buffer capacity mismatch");

  JUST_LOG_INFO("All passing assertions verified successfully.");

  // - - - Triggerable Runtime Failures
  #ifdef TRIGGER_RUNTIME_ASSERT
    int32_t invalidIndex = 128;
    JUST_ASSERT(invalidIndex < bufferCapacity);
  #endif

  #ifdef TRIGGER_ASSERT_MESSAGE
    void* nullPointer = NULL;
    JUST_ASSERT_MESSAGE(nullPointer != NULL, "Expected non-null handle from resource loader");
  #endif

  #ifdef TRIGGER_DEBUG_ASSERT
    int32_t negativeSize = -5;
    JUST_ASSERT_DEBUG_MESSAGE(negativeSize >= 0, "Size must be non-negative in debug mode");
  #endif

  // - - - Triggerable Unimplemented Markers
  #ifdef TRIGGER_TODO
    TODO;
  #endif

  #ifdef TRIGGER_TODO_COMMENT
    TODO_COMMENT("Implement serialization routine before release");
  #endif

  JUST_LOG_INFO("Asserts demo completed without errors.");
  return 0;
}
