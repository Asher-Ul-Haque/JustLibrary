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
 * @file : filesystem.h 
 * @brief : UNIX only file handling utilities
 */


#pragma once

#include <justUtils/defines.h>
#include <justUtils/memory/linearAlloc.h>
#include <justUtils/dataStructures/dynamicArray.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#if JUST_PLATFORM == JUST_PLATFORM_WINDOWS
  #error "Currently openly support POSIX compliant systems"
#endif 

#ifdef __cplusplus
extern "C" {
#endif


// - - - File Handling Enums - - - 

/// @brief : Type of file
typedef enum justFileType
{
  JUST_FILE_TYPE_UNKOWN    = 0,  ///< No idea 
  JUST_FILE_TYPE_REGULAR   = 1,  ///< Its a regular, everyday normal 
  JUST_FILE_TYPE_DIRECTORY = 2,  ///< Its a folder 
  JUST_FILE_TYPE_SYMLINK   = 3   ///< Its a shortcut
} JustFileType;

///@brief : File modes
typedef enum justFileMode 
{
  JUST_FILE_READ,        ///< Open existing file for reading
  JUST_FILE_WRITE,       ///< Truncate or create file for writing
  JUST_FILE_APPEND,      ///< Append to end or create file for writing
  JUST_FILE_READ_WRITE   ///< Open for both reading and writing (no truncation)
} JustFileMode;

///@brief : Cursor positions
typedef enum justSeekOrigin 
{
  JUST_SEEK_SET,  ///< From beginning of file
  JUST_SEEK_CUR,  ///< From current cursor position
  JUST_SEEK_END   ///< From end of file
} JustSeekOrigin;

///@brief : What a file is
typedef struct justFile 
{
  int32_t fd;     ///< POSIX file descriptor
  bool    isOpen; ///< True if handle is currently open
} JustFile;

/// @brief : Callback type for zero-allocation directory iteration
typedef void (*justDirIterCallback)(const char* ENTRY_NAME, bool IS_DIRECTORY, void* USER_DATA);


// - - - Path Manipulation - - - 

/**
 * @brief : Normalizes a path by collapsing duplicate slashes and resolving . / .. tokens.
 * @param PATH : Raw input path.
 * @param OUT_BUFFER : Output buffer to write normalized string to.
 * @param BUFFER_SIZE : Capacity of out_buffer.
 * @return : true if successful, false if buffer is too small.
 */
JUST_API bool justPathNormalize(
  const char* PATH, 
  char*       OUT_BUFFER, 
  size_t      BUFFER_SIZE);

/**
 * @brief : Joins two path components with a single path separator.
 * @param PART_1: First path 
 * @param PART_2 : Second part 
 * @param OUT_BUFFER : A string buffer big enough to store the join
 * @param BUFFER_SIZE : The size of the buffer
 * @return : true if successful, false if buffer is too small.
 */
JUST_API bool justPathJoin(
  const char* PART_1,
  const char* PART_2,
  char*       OUT_BUFFER,
  size_t      BUFFER_SIZE);

/**
 * @brief : Extracts the parent directory component from a path.
 * @param PATH : Child path
 * @param OUT_BUFFER : Buffer that stores the parent path 
 * @param BUFFER_SIZE : The size of the buffer
 * @return : true if successful, false if buffer is too small.
 */
JUST_API bool justPathParent(
  const char* PATH, 
  char*       OUT_BUFFER, 
  size_t      BUFFER_SIZE);

/**
 * @brief : Returns a pointer to the filename component within the path string (no copy).
 * @param PATH : The path 
 * @return : pointer to the filename within the path
 */
JUST_API const char* justPathFilename(const char* PATH);

/**
 * @brief : Returns a pointer to the extension within the path string (e.g. "txt" or "png", no dot).
 * @param PATH : The file path 
 * @return : pointer to the extension back in the PATH
 */
JUST_API const char* justPathExtension(const char* PATH);


// - - - Handle-Based File I/O - - - 

/**
 * @brief : opens a file 
 * @param FILE : Pointer to a file 
 * @param PATH : File path 
 * @param MODE : Open mode 
 * @return : true if opened, false if not
 */
JUST_API bool justFileOpen(
  JustFile*    FILE,
  const char*  PATH,
  JustFileMode MODE);

/**
 * @brief : closes a file 
 * @param FILE : pointer to the file to be closed
 */
JUST_API void justFileClose(JustFile* FILE);

/**
 * @brief : reads a file 
 * @param FILE : Pointer to the file to be read 
 * @param OUT_BUFFER : Where to write file contents 
 * @param BYTES_TO_READ : How many bytes at max to read 
 * @param OUT_BYTES_READ : The function will fill this with how many bytes were read 
 * @return : true is successful, false if not
 */
JUST_API bool justFileRead(
  JustFile*  FILE,
  void*      OUT_BUFFER,
  size_t     BYTES_TO_READ,
  size_t*    OUT_BYTES_READ);

/**
 * @brief : writes a file 
 * @param FILE : Pointer to the file to be written 
 * @param BUFFER : What to write 
 * @param BYTES_TO_WRITE : How many bytes to write 
 * @param OUT_BYTES_WRITTEN : Function will fill this with how many bytes were written 
 * @return : true if successful, false if not
 */
JUST_API bool justFileWrite(
  JustFile*   FILE, 
  const void* BUFFER, 
  size_t      BYTES_TO_WRITE, 
  size_t*     OUT_BYTES_WRITTEN);

/**
 * @brief : sets cursor position in file 
 * @param FILE : Which file to set cursor in 
 * @param OFFSET : How much to move the cursor 
 * @param ORIGIN : Where to move the cursor from
 * @return : true if successful, false if not
 */
JUST_API bool justFileSeek(
  JustFile*       FILE, 
  int64_t         OFFSET, 
  JustSeekOrigin  ORIGIN);

/**
 * @brief : Tells where the cursor is 
 * @param FILE : Which file's cursor to check 
 * @return : position of the cursor 
 */
JUST_API size_t justFileTell(JustFile* FILE);

/**
 * @brief : Tells the size of the file 
 * @param FILE : The file whose size is to be checked
 * @return : size of the file in bytes
 */
JUST_API size_t justFileSize(JustFile* FILE);

/**
 * @brief : Commits all changes to the file now 
 * @param FILE : Which file to commit to disk 
 * @return : true if successful, false otherwise
 */
JUST_API bool justFileFlush(JustFile* FILE);


// - - - Entire FILE IO - - - 

/**
 * @brief : Reads an entire file intoa  heap buffer via the tracker (+1 null-terminator)
 * @param PATH : Path to file
 * @param OUT_SIZE : Pointer to receive exact byte count read (exclusing null terminator)
 * @param TAG : Memory tracker tag
 * @return : Allocated buffer, (must be freed with JUST_fREE), or NULL on failure
 */
JUST_API void* justFileReadEntire(
  const char* PATH,
  size_t*     OUT_SIZE,
  const char* TAG);

/**
 * @brief : Writes an entire buffer to a file (Creates or truncates).
 * @param PATH : Path to file
 * @param BUFFER : What to write
 * @param BYTES : How much to write, excluding null-terminator
 */
JUST_API bool justFileWriteEntire(
  const char* PATH,
  const void* BUFFER,
  size_t      BYTES);

/**
 * @brief : Appends an entire buffer to the end of a file (Creates if absent)
 * @param PATH : Path to the file
 * @param BUFFER : What to write
 * @param BYTES : How much to append, excluding null-terminator
 */
JUST_API bool justFileAppendEntire(
  const char* PATH,
  const void* BUFFER,
  size_t      BYTES);

// - - - File System Checks & Operations - - - 

/**
 * @brief : Find whether a file at a path exists 
 * @param PATH : The file path 
 * @return : true if it exists, false otherwise
 */
JUST_API bool justFileExists(const char* PATH);

/**
 * @brief : Find whether a path is a file or not 
 * @param PATH : The file path 
 * @return : true if it is a file, false otherwise
 */
JUST_API bool justIsFile(const char* PATH);

/**
 * @brief : Find whether a path is a folder or not 
 * @param PATH : The folder path 
 * @return : true if it is a folder, false otherwise
 */
JUST_API bool justIsDirectory(const char* PATH);

/**
 * @brief : Provides size of the file at the given path 
 * @param PATH : The file path 
 * @return : size of the file in bytes
 */
JUST_API size_t justGetFileSize(const char* PATH);

/**
 * @brief : Make directory 
 * @param PATH : Directory path 
 * @return : true if successful, false otherwise
 */
JUST_API bool justMkdir(const char* PATH);

/**
 * @brief : Make directory but recursive
 * @param PATH : Directory path
 * @return : true if successful, false otherwise
 */
JUST_API bool justMkdirRecursive(const char* PATH);

/**
 * @brief : removes a file at the given path 
 * @param PATH : File path 
 * @return : true if delted, false otherwise
 */
JUST_API bool justFileRemove(const char* PATH);

/**
 * @brief : Renames a file at the given path 
 * @param OLD_PATH : current path 
 * @param NEW_PATH : path to move to 
 * @return : true if successful, false otherwise
 */
JUST_API bool justFileRename(const char* OLD_PATH, const char* NEW_PATH);

/**
 * @brief : Zero-allocation directory iteration calling callback for each entry.
 * @param PATH : Directory path 
 * @param CALLBACK : Directory iteration callback function 
 * @param USER_DATA : Optional user data to hold around 
 */
JUST_API bool justListDir(
  const char*          PATH,
  justDirIterCallback  CALLBACK,
  void*                USER_DATA);

#ifdef __cplusplus
}
#endif
