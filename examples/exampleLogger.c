// Uncomment to not see types
// #define PRINT_LOG_TYPES 0

// Uncomment to not see colors
//#define PRINT_LOG_COLORS 0

#include <justUtils/core/logger.h>

/**
 * @file exampleLogger.c
 * @brief Demonstration of JustLibrary's colored, level-gated logging system.
 */

int main(void)
{
  // - - - Direct display of all severity levels
  JUST_LOG_INFO("=== JustLibrary Logger Demonstration ===");
  
  JUST_LOG_TRACE("Trace level: Extremely verbose diagnostic steps");
  JUST_LOG_DEBUG("Debug level: Diagnostic data (pointers, sizes, offsets, loop counts)");
  JUST_LOG_INFO("Info level: High-level milestone reached (service up, file opened)");
  JUST_LOG_WARNING("Warning level: Suboptimal condition encountered, recovering");
  JUST_LOG_ERROR("Error level: Operation failed, but application continues");
  JUST_LOG_FATAL("Fatal level: Unrecoverable failure state");

  // - - - Formatted parameter logging
  const char* host      = "127.0.0.1";
  uint16_t    port      = 8080;
  double      latencyMs = 1.42;

  JUST_LOG_INFO("Connecting to %s:%u (probe latency: %.2f ms)", host, port, latencyMs);
  JUST_LOG_INFO("Logger demo completed successfully.");
  return 0;
}
