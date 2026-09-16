/**
 * @file testFilesystem.c
 * @brief Unit test suite for POSIX filesystem and path utilities.
 */

#include <justUtils/testing/testManager.h>
#include <justUtils/testing/expect.h>
#include <justUtils/system/filesystem.h>
#include <justUtils/memory/tracker.h>
#include <stdint.h>
#include <string.h>

#define GROUP_FS_PATHS 0
#define GROUP_FS_FILE  1
#define GROUP_FS_SLURP 2
#define GROUP_FS_DIR   3

#define TEST_SANDBOX_DIR "bin/test_sandbox"

// =============================================================================
// 1. PATH UTILITIES
// =============================================================================

static JustTestResult testPathUtilities(void)
{
  // Test filename extraction (zero copy)
  const char* path1 = "/var/log/just_engine/crash.dump";
  JUST_EXPECT_C_STRING_TO_BE("crash.dump", justPathFilename(path1));
  JUST_EXPECT_C_STRING_TO_BE("nodir.txt", justPathFilename("nodir.txt"));

  // Test extension extraction
  JUST_EXPECT_C_STRING_TO_BE("dump", justPathExtension(path1));
  JUST_EXPECT_TO_BE_NULL(justPathExtension("/var/log/noext"));

  // Test path join
  char joined[256];
  JUST_EXPECT_TO_BE_TRUE(justPathJoin("home/user", "project/file.c", joined, sizeof(joined)));
  JUST_EXPECT_C_STRING_TO_BE("home/user/project/file.c", joined);

  // Join with trailing slash
  JUST_EXPECT_TO_BE_TRUE(justPathJoin("home/user/", "config.json", joined, sizeof(joined)));
  JUST_EXPECT_C_STRING_TO_BE("home/user/config.json", joined);

  // Test path parent
  char parent[256];
  JUST_EXPECT_TO_BE_TRUE(justPathParent("/usr/local/bin", parent, sizeof(parent)));
  JUST_EXPECT_C_STRING_TO_BE("/usr/local", parent);

  JUST_EXPECT_TO_BE_TRUE(justPathParent("file_in_current_dir.txt", parent, sizeof(parent)));
  JUST_EXPECT_C_STRING_TO_BE(".", parent);

  // Test path normalization (collapses duplicate slashes and relative components)
  char normalized[256];
  JUST_EXPECT_TO_BE_TRUE(justPathNormalize("/usr//local/../bin/./app", normalized, sizeof(normalized)));
  JUST_EXPECT_C_STRING_TO_BE("/usr/bin/app", normalized);

  return JUST_TEST_PASS;
}

// =============================================================================
// 2. HANDLE-BASED FILE I/O
// =============================================================================

static JustTestResult testHandleBasedIO(void)
{
  justMkdirRecursive(TEST_SANDBOX_DIR);
  const char* filePath = TEST_SANDBOX_DIR "/handle_test.bin";

  justFileRemove(filePath);

  JustFile file;
  // Open for write
  JUST_EXPECT_TO_BE_TRUE(justFileOpen(&file, filePath, JUST_FILE_WRITE));
  JUST_EXPECT_TO_BE_TRUE(file.isOpen);

  uint8_t writeBytes[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
  size_t bytesWritten = 0;
  JUST_EXPECT_TO_BE_TRUE(justFileWrite(&file, writeBytes, sizeof(writeBytes), &bytesWritten));
  JUST_EXPECT_TO_BE((size_t)16, bytesWritten);
  JUST_EXPECT_TO_BE((size_t)16, justFileTell(&file));
  JUST_EXPECT_TO_BE((size_t)16, justFileSize(&file));

  justFileFlush(&file);
  justFileClose(&file);
  JUST_EXPECT_TO_BE_FALSE(file.isOpen);

  // Open for read
  JUST_EXPECT_TO_BE_TRUE(justFileOpen(&file, filePath, JUST_FILE_READ));
  JUST_EXPECT_TO_BE_TRUE(file.isOpen);
  JUST_EXPECT_TO_BE((size_t)16, justFileSize(&file));

  // Seek halfway through
  JUST_EXPECT_TO_BE_TRUE(justFileSeek(&file, 8, JUST_SEEK_SET));
  JUST_EXPECT_TO_BE((size_t)8, justFileTell(&file));

  uint8_t readBuffer[8];
  size_t bytesRead = 0;
  JUST_EXPECT_TO_BE_TRUE(justFileRead(&file, readBuffer, sizeof(readBuffer), &bytesRead));
  JUST_EXPECT_TO_BE((size_t)8, bytesRead);

  // Verify byte range [8..15]
  for (size_t i = 0; i < 8; ++i)
  {
    JUST_EXPECT_TO_BE(writeBytes[8 + i], readBuffer[i]);
  }

  justFileClose(&file);
  justFileRemove(filePath);

  return JUST_TEST_PASS;
}

// =============================================================================
// 3. WHOLE-FILE SLURRY I/O
// =============================================================================

static JustTestResult testWholeFileIO(void)
{
  const char* tag = "FS_TEST";
  justMemorySetLimit(8192, tag);

  justMkdirRecursive(TEST_SANDBOX_DIR);
  const char* textPath = TEST_SANDBOX_DIR "/slurp_test.txt";
  const char* payload = "Hello, JustLibrary File System!";
  size_t payloadLen = strlen(payload);

  // Write entire file
  JUST_EXPECT_TO_BE_TRUE(justFileWriteEntire(textPath, payload, payloadLen));
  JUST_EXPECT_TO_BE_TRUE(justFileExists(textPath));
  JUST_EXPECT_TO_BE((size_t)payloadLen, justGetFileSize(textPath));

  // Append payload
  const char* appendPayload = " - Appended Content";
  size_t appendLen = strlen(appendPayload);
  JUST_EXPECT_TO_BE_TRUE(justFileAppendEntire(textPath, appendPayload, appendLen));
  JUST_EXPECT_TO_BE((size_t)(payloadLen + appendLen), justGetFileSize(textPath));

  // Read entire file back
  size_t readSize = 0;
  char* readData = (char*)justFileReadEntire(textPath, &readSize, tag);
  JUST_EXPECT_TO_BE_NOT_NULL(readData);
  JUST_EXPECT_TO_BE((size_t)(payloadLen + appendLen), readSize);

  // Null terminator is automatically guaranteed by justFileReadEntire
  JUST_EXPECT_TO_BE('\0', readData[readSize]);
  JUST_EXPECT_C_STRING_TO_BE("Hello, JustLibrary File System! - Appended Content", readData);

  JUST_FREE(readData);
  justFileRemove(textPath);
  justMemorySetLimit(0, tag);

  return JUST_TEST_PASS;
}

// =============================================================================
// 4. DIRECTORY OPERATIONS & ITERATION
// =============================================================================

typedef struct DirWalkContext
{
  size_t fileCount;
  size_t dirCount;
} DirWalkContext;

static void dirWalker(const char* entryName, bool isDirectory, void* userData)
{
  (void)entryName;
  DirWalkContext* ctx = (DirWalkContext*)userData;
  if (isDirectory)
  {
    ctx->dirCount++;
  }
  else
  {
    ctx->fileCount++;
  }
}

static JustTestResult testDirectoryOperations(void)
{
  const char* dirA = TEST_SANDBOX_DIR "/dirA";
  const char* dirB = TEST_SANDBOX_DIR "/dirA/dirB/dirC";
  const char* f1   = TEST_SANDBOX_DIR "/dirA/file1.dat";
  const char* f2   = TEST_SANDBOX_DIR "/dirA/file2.dat";

  // Create recursive directories
  JUST_EXPECT_TO_BE_TRUE(justMkdirRecursive(dirB));
  JUST_EXPECT_TO_BE_TRUE(justIsDirectory(dirA));
  JUST_EXPECT_TO_BE_TRUE(justIsDirectory(dirB));

  // Create test files
  const char* dummy = "test";
  JUST_EXPECT_TO_BE_TRUE(justFileWriteEntire(f1, dummy, 4));
  JUST_EXPECT_TO_BE_TRUE(justFileWriteEntire(f2, dummy, 4));
  JUST_EXPECT_TO_BE_TRUE(justIsFile(f1));
  JUST_EXPECT_TO_BE_TRUE(justIsFile(f2));

  // Rename test file
  const char* f1Renamed = TEST_SANDBOX_DIR "/dirA/file1_renamed.dat";
  JUST_EXPECT_TO_BE_TRUE(justFileRename(f1, f1Renamed));
  JUST_EXPECT_TO_BE_FALSE(justFileExists(f1));
  JUST_EXPECT_TO_BE_TRUE(justFileExists(f1Renamed));

  // Iterate over dirA: should contain 2 files and 1 directory (dirB)
  DirWalkContext ctx = { .fileCount = 0, .dirCount = 0 };
  JUST_EXPECT_TO_BE_TRUE(justListDir(dirA, dirWalker, &ctx));
  JUST_EXPECT_TO_BE((size_t)2, ctx.fileCount);
  JUST_EXPECT_TO_BE((size_t)1, ctx.dirCount);

  // Cleanup
  justFileRemove(f1Renamed);
  justFileRemove(f2);
  justFileRemove(dirB); // Removes empty dirC
  justFileRemove(TEST_SANDBOX_DIR "/dirA/dirB");
  justFileRemove(dirA);

  return JUST_TEST_PASS;
}

// =============================================================================
// MAIN ENTRY POINT
// =============================================================================

int main(void)
{
  justTestRegister(testPathUtilities,       "Filesystem: Path Normalize, Join, and Extract", GROUP_FS_PATHS);
  justTestRegister(testHandleBasedIO,       "Filesystem: Handle-Based Open, Seek, and Read", GROUP_FS_FILE);
  justTestRegister(testWholeFileIO,         "Filesystem: Whole-File Slurry Read and Append", GROUP_FS_SLURP);
  justTestRegister(testDirectoryOperations, "Filesystem: Mkdir Recursive and Directory Walk",GROUP_FS_DIR);

  return (int)justTestRunAll();
}
