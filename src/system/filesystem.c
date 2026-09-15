#include <justUtils/system/filesystem.h>
#include <justUtils/memory/tracker.h>
#include <justUtils/core/asserts.h>
#include <justUtils/core/logger.h>

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>


// - - - Path Manipulation - - -

JUST_API const char* justPathFilename(const char* PATH)
{
  if (!PATH) return NULL;
  const char* slash = strrchr(PATH, '/');
  return slash ? (slash + 1) : PATH;
}

JUST_API const char* justPathExtension(const char* PATH)
{
  if (!PATH) return NULL;
  const char* filename = justPathFilename(PATH);
  const char* dot = strrchr(filename, '.');
  return (dot && dot != filename && *(dot + 1) != '\0') ? (dot + 1) : NULL;
}

JUST_API bool justPathJoin(
  const char* PART_1, 
  const char* PART_2, 
  char*       OUT_BUFFER, 
  size_t      BUFFER_SIZE)
{
  JUST_ASSERT_DEBUG(PART_1 != NULL && PART_2 != NULL && OUT_BUFFER != NULL);
  if (BUFFER_SIZE == 0) return false;

  size_t len1 = strlen(PART_1);
  size_t len2 = strlen(PART_2);

  bool needsSlash = (len1 > 0 && PART_1[len1 - 1] != '/') && (len2 == 0 || PART_2[0] != '/');
  size_t totalLen = len1 + (needsSlash ? 1 : 0) + len2;

  if (totalLen >= BUFFER_SIZE) return false;

  int32_t written;
  if (needsSlash)
  {
    written = snprintf(OUT_BUFFER, BUFFER_SIZE, "%s/%s", PART_1, PART_2);
  }
  else
  {
    written = snprintf(OUT_BUFFER, BUFFER_SIZE, "%s%s", PART_1, PART_2);
  }

  return (written >= 0 && (size_t)written < BUFFER_SIZE);
}

JUST_API bool justPathParent(
  const char* PATH, 
  char*       OUT_BUFFER, 
  size_t      BUFFER_SIZE)
{
  JUST_ASSERT_DEBUG(PATH != NULL && OUT_BUFFER != NULL);
  if (BUFFER_SIZE == 0) return false;

  const char* slash = strrchr(PATH, '/');
  if (!slash)
  {
    if (BUFFER_SIZE < 2) return false;
    OUT_BUFFER[0] = '.';
    OUT_BUFFER[1] = '\0';
    return true;
  }

  if (slash == PATH)
  {
    if (BUFFER_SIZE < 2) return false;
    OUT_BUFFER[0] = '/';
    OUT_BUFFER[1] = '\0';
    return true;
  }

  size_t parentLen = (size_t)(slash - PATH);
  if (parentLen >= BUFFER_SIZE) return false;

  memcpy(OUT_BUFFER, PATH, parentLen);
  OUT_BUFFER[parentLen] = '\0';
  return true;
}

JUST_API bool justPathNormalize(
  const char* PATH, 
  char*       OUT_BUFFER, 
  size_t      BUFFER_SIZE)
{
  JUST_ASSERT_DEBUG(PATH != NULL && OUT_BUFFER != NULL);

  if (BUFFER_SIZE == 0) return false;

  char    temp[1024];
  size_t  pathLen = strlen(PATH);
  if (pathLen >= sizeof(temp)) return false;
  memcpy(temp, PATH, pathLen + 1);

  bool        isAbsolute = (temp[0] == '/');
  const char* segments[64];
  size_t      segCount = 0;

  char* cursor = temp;
  while (*cursor)
  {
    while (*cursor == '/') cursor++;
    if (!*cursor) break;

    char* start = cursor;
    while (*cursor && *cursor != '/') cursor++;
    if (*cursor)
    {
      *cursor = '\0';
      cursor++;
    }

    if (strcmp(start, ".") == 0)
    {
      continue;
    }
    else if (strcmp(start, "..") == 0)
    {
      if (segCount > 0 && strcmp(segments[segCount - 1], "..") != 0)
      {
        segCount--;
      }
      else if (!isAbsolute)
      {
        if (segCount < 64) segments[segCount++] = "..";
      }
    }
    else
    {
      if (segCount < 64) segments[segCount++] = start;
    }
  }

  OUT_BUFFER[0] = '\0';
  size_t outPos = 0;

  if (isAbsolute)
  {
    if (outPos + 1 >= BUFFER_SIZE) return false;
    OUT_BUFFER[outPos++] = '/';
    OUT_BUFFER[outPos] = '\0';
  }

  for (size_t i = 0; i < segCount; ++i)
  {
    size_t  segLen      = strlen(segments[i]);
    bool    needsSlash  = (outPos > 0 && OUT_BUFFER[outPos - 1] != '/');
    if (outPos + (needsSlash ? 1 : 0) + segLen >= BUFFER_SIZE) return false;

    if (needsSlash) OUT_BUFFER[outPos++] = '/';
    memcpy(OUT_BUFFER + outPos, segments[i], segLen);
    outPos += segLen;
    OUT_BUFFER[outPos] = '\0';
  }

  if (outPos == 0)
  {
    if (BUFFER_SIZE < 2) return false;
    OUT_BUFFER[0] = '.';
    OUT_BUFFER[1] = '\0';
  }

  return true;
}


// - - - Handle-Based File I/O - - -

JUST_API bool justFileOpen(
  JustFile*    FILE_HANDLE,
  const char*  PATH,
  JustFileMode MODE)
{
  JUST_ASSERT_DEBUG(FILE_HANDLE != NULL && PATH != NULL);

  int32_t flags       = 0;
  mode_t  createMode  = 0644; // rw-r--r--

  switch (MODE)
  {
    case JUST_FILE_READ:
      flags = O_RDONLY;
      break;
    case JUST_FILE_WRITE:
      flags = O_WRONLY | O_CREAT | O_TRUNC;
      break;
    case JUST_FILE_APPEND:
      flags = O_WRONLY | O_CREAT | O_APPEND;
      break;
    case JUST_FILE_READ_WRITE:
      flags = O_RDWR | O_CREAT;
      break;
    default:
      return false;
  }

  int32_t fd = open(PATH, flags, createMode);
  if (fd < 0)
  {
    FILE_HANDLE->fd     = -1;
    FILE_HANDLE->isOpen = false;
    return false;
  }

  FILE_HANDLE->fd     = fd;
  FILE_HANDLE->isOpen = true;
  return true;
}

JUST_API void justFileClose(JustFile* FILE_HANDLE)
{
  if (!FILE_HANDLE || !FILE_HANDLE->isOpen) return;

  close(FILE_HANDLE->fd);
  FILE_HANDLE->fd = -1;
  FILE_HANDLE->isOpen = false;
}

JUST_API bool justFileRead(
  JustFile* FILE_HANDLE,
  void*     OUT_BUFFER,
  size_t    BYTES_TO_READ,
  size_t*   OUT_BYTES_READ)
{
  JUST_ASSERT_DEBUG(FILE_HANDLE != NULL && OUT_BUFFER != NULL);
  if (!FILE_HANDLE->isOpen) return false;

  ssize_t result = read(FILE_HANDLE->fd, OUT_BUFFER, BYTES_TO_READ);
  if (result < 0)
  {
    if (OUT_BYTES_READ) *OUT_BYTES_READ = 0;
    return false;
  }

  if (OUT_BYTES_READ) *OUT_BYTES_READ = (size_t)result;
  return true;
}

JUST_API bool justFileWrite(
  JustFile*  FILE_HANDLE, 
  const void* BUFFER, 
  size_t      BYTES_TO_WRITE, 
  size_t*     OUT_BYTES_WRITTEN)
{
  JUST_ASSERT_DEBUG(FILE_HANDLE != NULL && BUFFER != NULL);
  if (!FILE_HANDLE->isOpen) return false;

  ssize_t result = write(FILE_HANDLE->fd, BUFFER, BYTES_TO_WRITE);
  if (result < 0)
  {
    if (OUT_BYTES_WRITTEN) *OUT_BYTES_WRITTEN = 0;
    return false;
  }

  if (OUT_BYTES_WRITTEN) *OUT_BYTES_WRITTEN = (size_t)result;
  return true;
}

JUST_API bool justFileSeek(
  JustFile*      FILE_HANDLE, 
  int64_t         OFFSET, 
  JustSeekOrigin ORIGIN)
{
  JUST_ASSERT_DEBUG(FILE_HANDLE != NULL);
  if (!FILE_HANDLE->isOpen) return false;

  int whence = SEEK_SET;
  switch (ORIGIN)
  {
    case JUST_SEEK_SET: whence = SEEK_SET; break;
    case JUST_SEEK_CUR: whence = SEEK_CUR; break;
    case JUST_SEEK_END: whence = SEEK_END; break;
    default: return false;
  }

  return (lseek(FILE_HANDLE->fd, (off_t)OFFSET, whence) != (off_t)-1);
}

JUST_API size_t justFileTell(JustFile* FILE_HANDLE)
{
  JUST_ASSERT_DEBUG(FILE_HANDLE != NULL);
  if (!FILE_HANDLE->isOpen) return 0;

  off_t pos = lseek(FILE_HANDLE->fd, 0, SEEK_CUR);
  return (pos == (off_t)-1) ? 0 : (uint64_t)pos;
}

JUST_API size_t justFileSize(JustFile* FILE_HANDLE)
{
  JUST_ASSERT_DEBUG(FILE_HANDLE != NULL);
  if (!FILE_HANDLE->isOpen) return 0;

  struct stat st;
  if (fstat(FILE_HANDLE->fd, &st) != 0) return 0;
  return (uint64_t)st.st_size;
}

JUST_API bool justFileFlush(JustFile* FILE_HANDLE)
{
  JUST_ASSERT_DEBUG(FILE_HANDLE != NULL);
  if (!FILE_HANDLE->isOpen) return false;
  return (fsync(FILE_HANDLE->fd) == 0);
}


// - - - Whole-File Buffer I/O - - -

JUST_API void* justFileReadEntire(
  const char* PATH, 
  size_t*     OUT_SIZE, 
  const char* TAG)
{
  JUST_ASSERT_DEBUG(PATH != NULL);
  if (OUT_SIZE) *OUT_SIZE = 0;

  JustFile file;
  if (!justFileOpen(&file, PATH, JUST_FILE_READ))
  {
    JUST_LOG_WARNING("[FILESYS] : Failed to open file for whole read '%s'", PATH);
    return NULL;
  }

  uint64_t size = justFileSize(&file);
  const char* allocTag = (TAG && TAG[0] != '\0') ? TAG : "FILE_BUFFER";

  uint8_t* buffer = (uint8_t*) JUST_MALLOC_TAGGED((size_t)size + 1, allocTag);
  if (!buffer)
  {
    justFileClose(&file);
    JUST_LOG_ERROR("[FILESYS] : Out of memory reading file '%s' (%zu bytes)", PATH, (size_t)size);
    return NULL;
  }

  size_t totalRead = 0;
  while (totalRead < size)
  {
    size_t chunkRead = 0;
    if (!justFileRead(&file, buffer + totalRead, (size_t)(size - totalRead), &chunkRead) || chunkRead == 0)
    {
      break;
    }
    totalRead += chunkRead;
  }

  buffer[totalRead] = '\0';
  justFileClose(&file);

  if (OUT_SIZE) *OUT_SIZE = totalRead;
  return buffer;
}

JUST_API bool justFileWriteEntire(
  const char* PATH, 
  const void* BUFFER, 
  size_t      BYTES)
{
  JUST_ASSERT_DEBUG(PATH != NULL);
  if (BYTES > 0) JUST_ASSERT_DEBUG(BUFFER != NULL);

  JustFile file;
  if (!justFileOpen(&file, PATH, JUST_FILE_WRITE))
  {
    JUST_LOG_ERROR("[FILESYS] : Failed to open file for whole write '%s'", PATH);
    return false;
  }

  size_t totalWritten = 0;
  while (totalWritten < BYTES)
  {
    size_t chunkWritten = 0;
    if (!justFileWrite(&file, (const uint8_t*)BUFFER + totalWritten, BYTES - totalWritten, &chunkWritten) || chunkWritten == 0)
    {
      justFileClose(&file);
      return false;
    }
    totalWritten += chunkWritten;
  }

  justFileClose(&file);
  return (totalWritten == BYTES);
}

JUST_API bool justFileAppendEntire(
  const char* PATH, 
  const void* BUFFER, 
  size_t      BYTES)
{
  JUST_ASSERT_DEBUG(PATH != NULL);
  if (BYTES == 0) return true;
  JUST_ASSERT_DEBUG(BUFFER != NULL);

  JustFile file;
  if (!justFileOpen(&file, PATH, JUST_FILE_APPEND))
  {
    JUST_LOG_ERROR("[FILESYS] : Failed to open file for whole append '%s'", PATH);
    return false;
  }

  size_t totalWritten = 0;
  while (totalWritten < BYTES)
  {
    size_t chunkWritten = 0;
    if (!justFileWrite(&file, (const uint8_t*)BUFFER + totalWritten, BYTES - totalWritten, &chunkWritten) || chunkWritten == 0)
    {
      justFileClose(&file);
      return false;
    }
    totalWritten += chunkWritten;
  }

  justFileClose(&file);
  return (totalWritten == BYTES);
}


// - - - File System Checks & Operations - - -

JUST_API bool justFileExists(const char* PATH)
{
  JUST_ASSERT_DEBUG(PATH != NULL && PATH[0] != '\0');
  return (access(PATH, F_OK) == 0);
}

JUST_API bool justIsFile(const char* PATH)
{
  JUST_ASSERT_DEBUG(PATH != NULL);
  struct stat st;
  if (stat(PATH, &st) != 0) return false;
  return S_ISREG(st.st_mode);
}

JUST_API bool justIsDirectory(const char* PATH)
{
  JUST_ASSERT_DEBUG(PATH != NULL);

  struct stat st;
  if (stat(PATH, &st) != 0) return false;
  return S_ISDIR(st.st_mode);
}

JUST_API size_t justGetFileSize(const char* PATH)
{
  JUST_ASSERT_DEBUG(PATH != NULL);
  struct stat st;
  if (stat(PATH, &st) != 0) return 0;
  return (uint64_t)st.st_size;
}

JUST_API bool justMkdir(const char* PATH)
{
  JUST_ASSERT_DEBUG(PATH != NULL);
  if (mkdir(PATH, 0755) == 0) return true;
  return (errno == EEXIST) && justIsDirectory(PATH);
}

JUST_API bool justMkdirRecursive(const char* PATH)
{
  JUST_ASSERT_DEBUG(PATH != NULL && PATH[0] != '\0');

  char temp[1024];
  size_t len = strlen(PATH);
  if (len >= sizeof(temp)) return false;
  memcpy(temp, PATH, len + 1);

  for (char* p = temp + 1; *p; p++)
  {
    if (*p == '/')
    {
      *p = '\0';
      if (mkdir(temp, 0755) != 0 && errno != EEXIST)
      {
        return false;
      }
      *p = '/';
    }
  }

  if (mkdir(temp, 0755) != 0 && errno != EEXIST)
  {
    return false;
  }

  return true;
}

JUST_API bool justFileRemove(const char* PATH)
{
  JUST_ASSERT_DEBUG(PATH != NULL);
  return (unlink(PATH) == 0);
}

JUST_API bool justFileRename(const char* OLD_PATH, const char* NEW_PATH)
{
  JUST_ASSERT_DEBUG(OLD_PATH != NULL && NEW_PATH != NULL);
  return (rename(OLD_PATH, NEW_PATH) == 0);
}

JUST_API bool justListDir(
  const char*          PATH,
  justDirIterCallback CALLBACK,
  void*                USER_DATA)
{
  JUST_ASSERT_DEBUG(PATH != NULL && CALLBACK != NULL);

  DIR* dir = opendir(PATH);
  if (!dir) return false;

  struct dirent* entry = NULL;
  while ((entry = readdir(dir)) != NULL)
  {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
    {
      continue;
    }

    bool isDir = false;
    char subPath[1024];
    if (justPathJoin(PATH, entry->d_name, subPath, sizeof(subPath)))
    {
      isDir = justIsDirectory(subPath);
    }

    CALLBACK(entry->d_name, isDir, USER_DATA);
  }

  closedir(dir);
  return true;
}
