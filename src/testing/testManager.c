#include <justUtils/testing/testManager.h>
#include <justUtils/core/asserts.h>
#include <justUtils/core/logger.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if JUST_PLATFORM != JUST_PLATFORM_WINDOWS
  #include <unistd.h>
  #include <sys/wait.h>
#endif

typedef struct justTestEntry 
{
  justTestFunc func;
  const char*  description;
} justTestEntry;

typedef struct justTestGroup 
{
  justTestEntry tests[JUST_MAX_TESTS_PER_GROUP];
  size_t        count;
} justTestGroup;

static justTestGroup testGroups[JUST_MAX_TEST_GROUPS];
static size_t        maxGroupID = 0;

JUST_API void justTestRegister(justTestFunc TEST_FUNC, const char* DESCRIPTION, uint8_t GROUP_ID) 
{
  JUST_ASSERT_MESSAGE(TEST_FUNC != NULL, "[TEST MANAGER] : Test function cannot be NULL");
  JUST_ASSERT_MESSAGE(GROUP_ID < JUST_MAX_TEST_GROUPS, "[TEST MANAGER] : GROUP_ID out of bounds");

  if (GROUP_ID > maxGroupID) maxGroupID = GROUP_ID;

  justTestGroup* group = &testGroups[GROUP_ID];
  JUST_ASSERT_MESSAGE(group->count < JUST_MAX_TESTS_PER_GROUP, "[TEST MANAGER] : Group test capacity exceeded!");

  group->tests[group->count].func        = TEST_FUNC;
  group->tests[group->count].description = DESCRIPTION;
  group->count++;
}

#if !defined(_WIN32)
static bool runTestForked(justTestEntry TEST, JustTestResult* OUT_RESULT) 
{
  int32_t pipefd[2];
  if (pipe(pipefd) != 0) 
  {
    JUST_LOG_ERROR("[TEST MANAGER] : Failed to create process pipe");
    return false;
  }

  pid_t pid = fork();
  if (pid == 0) 
  {
    // - - - Redirect child stdout & stderr to pipe
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);

    JustTestResult res = TEST.func();
    exit((int)res);
  }

  // - - - Parent Process
  close(pipefd[1]);

  char    buffer[1024];
  ssize_t bytesRead;
  while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) 
  {
    buffer[bytesRead] = '\0';
    fputs(buffer, stdout);   
  }
  close(pipefd[0]);

  int32_t status = 0;
  waitpid(pid, &status, 0);

  // - - - Trapped by signal (SIGSEGV, SIGABRT, SIGBUS, etc.)
  if (WIFSIGNALED(status)) 
  {
    return false;
  }

  *OUT_RESULT = (JustTestResult)WEXITSTATUS(status);
  return true;
}
#endif

JUST_API size_t justTestRunAll(void) 
{
  size_t totalTests   = 0;
  size_t passedTests  = 0;
  size_t skippedTests = 0;
  size_t failedTests  = 0;
  size_t crashedTests = 0;

  JUST_LOG_INFO("==================================================");
  JUST_LOG_INFO("               RUNNING UNIT TESTS                 ");
  JUST_LOG_INFO("==================================================");

  for (size_t g = 0; g <= maxGroupID; ++g) 
  {
    justTestGroup* group = &testGroups[g];
    if (group->count == 0) continue;

    JUST_LOG_INFO("\n\n\n--- [TEST GROUP %zu] ---", g);

    for (size_t i = 0; i < group->count; ++i) 
    {
      justTestEntry test = group->tests[i];
      totalTests++;

      JustTestResult result          = JUST_TEST_FAIL;
      bool           executedSafely  = true;

    #if !defined(_WIN32)
      executedSafely = runTestForked(test, &result);
    #else
      result = test.func();
    #endif

      if (!executedSafely) 
      {
        crashedTests++;
        JUST_LOG_ERROR("  [CRASHED] %s\n", test.description);
      } 
      else if (result == JUST_TEST_PASS) 
      {
        passedTests++;
        JUST_LOG_INFO("  [PASS]    %s\n", test.description);
      } 
      else if (result == JUST_TEST_SKIP) 
      {
        skippedTests++;
        JUST_LOG_WARNING("  [SKIP]    %s\n", test.description);
      } 
      else 
      {
        failedTests++;
        JUST_LOG_ERROR("  [FAIL]    %s\n", test.description);
      }
    }
  }

  JUST_LOG_INFO("==================================================");
  JUST_LOG_INFO("                  TEST SUMMARY                    ");
  JUST_LOG_INFO("==================================================");
  JUST_LOG_INFO(" Total Executed : %zu", totalTests);
  JUST_LOG_INFO(" Passed         : %zu", passedTests);
  
  if (skippedTests > 0) JUST_LOG_WARNING(" Skipped        : %zu", skippedTests);
  if (failedTests  > 0) JUST_LOG_ERROR(" Failed         : %zu", failedTests);
  if (crashedTests > 0) JUST_LOG_ERROR(" Crashed        : %zu", crashedTests);

  JUST_LOG_INFO("==================================================\n");

  return (failedTests + crashedTests);
}
