// src/filesystem.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <limits.h>
#include <unistd.h>
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <sys/stat.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include "globals.hpp"
#include "setup.hpp"
#include "defines.hpp"

#ifdef WIN32
#define mkdir(dir, mode)    mkdir(dir)
#undef DATA_PREFIX
#define DATA_PREFIX "./data/"
#endif

namespace fs = std::filesystem;

/**
 * Checks if the given file exists and is accessible.
 * @param filename Path to the file.
 * @return true if the file exists and is accessible, false otherwise.
 */
bool faccessible(const char* filename)
{
  return fs::exists(filename) && fs::is_regular_file(filename);
}

/**
 * Opens a file located in the SuperTux data directory.
 * @param rel_filename Relative path of the file within the data directory.
 * @param mode Mode in which the file should be opened (read/write).
 * @return Pointer to the opened file, or nullptr if the file could not be opened.
 */
FILE* opendata(const char* rel_filename, const char* mode)
{
  fs::path filename = fs::path(st_dir) / rel_filename;

  FILE* fi = fopen(filename.c_str(), mode);

  if (fi == nullptr && verbose)
  {
    std::cerr << "Warning: Unable to open the file \"" << filename.string() << "\" ";
    if (strcmp(mode, "r") == 0)
    {
      std::cerr << "for read!!!\n";
    }
    else if (strcmp(mode, "w") == 0)
    {
      std::cerr << "for write!!!\n";
    }
  }

  return fi;
}

/**
 * Function to process both directories and files.
 * This function handles the logic for scanning a directory (or subdirectory).
 * @param base_path Base path to the directory.
 * @param rel_path Relative path to the directory.
 * @param expected_file The expected file to be found in the directory (optional).
 * @param is_subdir Flag indicating if the entry is a subdirectory.
 * @param glob Optional pattern to match files.
 * @param exception_str Optional substring that if found, excludes entries.
 * @param sdirs List to store the results (files or directories).
 */
static void process_directory(const std::string& base_path, const std::string& rel_path,
  const char* expected_file, bool is_subdir, const char* glob,
  const char* exception_str, StringList& sdirs)
{
  // Construct the full path
  fs::path path = fs::path(base_path) / rel_path;

  if (verbose)
  {
    std::cerr << "Accessing directory: " << path << std::endl;
  }

  try
  {
    if (!fs::exists(path) || !fs::is_directory(path))
    {
      return;
    }

    for (const auto& entry: fs::directory_iterator(path))
    {
      // Check if entry matches directory or file based on is_subdir flag
      if ((is_subdir && entry.is_directory()) || (!is_subdir && entry.is_regular_file()))
      {
        // Cache the path string to avoid multiple conversions and allocations in the loop
        const std::string path_str = entry.path().string();

        if (expected_file != nullptr)
        {
          if (!faccessible((entry.path() / expected_file).c_str()))
          {
            continue;
          }
        }

        if (exception_str != nullptr && path_str.find(exception_str) != std::string::npos)
        {
          continue;
        }

        if (glob != nullptr && path_str.find(glob) == std::string::npos)
        {
          continue;
        }

        sdirs.push_back(entry.path().filename().string());
      }
    }
  }
  catch (const fs::filesystem_error& e)
  {
    // Silently ignore errors, as directories might not be readable.
  }
}

/**
 * Function to get subdirectories within a relative path.
 * @param rel_path Relative path to the directory.
 * @param expected_file The expected file to be found in the subdirectories (optional).
 * @return List of subdirectories.
 */
StringList dsubdirs(const char* rel_path, const char* expected_file)
{
  StringList sdirs; // Creates an empty vector
  process_directory(st_dir, rel_path, expected_file, true, nullptr, nullptr, sdirs);
  process_directory(datadir, rel_path, expected_file, true, nullptr, nullptr, sdirs);
  return sdirs;
}

/**
 * Gets files within a relative path.
 * @param rel_path Relative path to the directory.
 * @param glob Optional pattern to match files.
 * @param exception_str Optional substring that if found, excludes entries.
 * @return List of files.
 */
StringList dfiles(const char* rel_path, const char* glob, const char* exception_str)
{
  StringList sdirs; // Creates an empty vector
  process_directory(st_dir, rel_path, nullptr, false, glob, exception_str, sdirs);
  process_directory(datadir, rel_path, nullptr, false, glob, exception_str, sdirs);
  return sdirs;
}

#ifdef __WII__

/**
 * Set SuperTux configuration and save directories (Wii specific)
 * Uses the Current Working Directory set by the Homebrew Channel.
 */
void st_directory_setup(void)
{
  char local_path[PATH_MAX];

  // The Homebrew Channel sets the CWD to the folder containing the .dol file.
  // e.g., "sd:/apps/supertux"
  if (getcwd(local_path, PATH_MAX))
  {
    st_dir = local_path;
  }
  else
  {
    // Fallback if CWD fails (unlikely)
    st_dir = "/apps/supertux";
  }

  // Set paths relative to the installation folder
  datadir = st_dir + "/data";
  st_save_dir = st_dir + "/save";

  // Ensure directories exist
  fs::create_directories(st_save_dir.c_str());
  fs::create_directories((st_dir + "/levels").c_str());

  if (verbose)
  {
    printf("Wii Setup: Root=%s\nData=%s\nSave=%s\n", st_dir.c_str(), datadir.c_str(), st_save_dir.c_str());
  }
}

#else // #ifndef __WII__

/**
 * Reads $HOME, canonicalized to strip any path-traversal sequences (e.g. "..")
 * before it is used to build st_dir. realpath() only resolves paths that
 * already exist, so a $HOME that has yet to be created falls back to the raw
 * value and create_directories() makes it later. An unset $HOME gives ".".
 * @return The home directory to build st_dir from.
 */
static std::string resolve_home_directory()
{
  const char* home_env = getenv("HOME");

  if (home_env == nullptr)
  {
    return ".";
  }

  char* resolved_home = realpath(home_env, nullptr);
  std::string home = (resolved_home != nullptr) ? resolved_home : home_env;
  free(resolved_home);

  return home;
}

#ifndef WIN32

/**
 * Asks the operating system where this executable lives.
 * @param buffer Receives the path, always NUL terminated on success.
 * @param size The size of buffer in bytes.
 * @return true if the path was retrieved. A platform with no supported way of
 *         asking always returns false, and the caller falls back to
 *         DATA_PREFIX.
 */
static bool get_executable_path(char* buffer, size_t size)
{
#if defined(__linux__)
  // readlink does not NUL-terminate; we do so explicitly.
  ssize_t len = readlink("/proc/self/exe", buffer, size - 1);
  if (len > 0)
  {
    buffer[len] = '\0';
    return true;
  }
#elif defined(__APPLE__)
  uint32_t buffer_size = static_cast<uint32_t>(size);
  if (_NSGetExecutablePath(buffer, &buffer_size) == 0)
  {
    return true;
  }
#elif defined(__FreeBSD__)
  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
  size_t len = size;
  if (sysctl(mib, 4, buffer, &len, NULL, 0) == 0)
  {
    return true;
  }
#else
  (void)buffer;
  (void)size;
#endif

  return false;
}

/**
 * Looks for the game data in the usual places around the executable.
 * @param exedir The directory holding the executable.
 * @return The first candidate that exists and is a directory, or DATA_PREFIX
 *         when none of them do.
 */
static std::string find_datadir_near(const fs::path& exedir)
{
  const std::vector<fs::path> search_paths = {
      exedir / "data",
      exedir / "../data",
      exedir / "../share/supertux",
  };

  for (const auto& path : search_paths)
  {
    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    {
      try
      {
        return fs::canonical(path).string();
      }
      catch (const fs::filesystem_error&)
      {
        // The path was invalid or unreadable, so we just try the next path.
        continue;
      }
    }
  }

  return DATA_PREFIX;
}

/**
 * Works out where the game data lives, starting from the executable's own
 * location. Falls back to DATA_PREFIX at every step that can fail.
 */
static void detect_datadir()
{
  char exe_file[PATH_MAX] = {0};

  if (!get_executable_path(exe_file, sizeof(exe_file)))
  {
    if (verbose)
    {
      puts("Couldn't determine executable path, using default: " DATA_PREFIX);
    }
    datadir = DATA_PREFIX;
    return;
  }

  /* Use NULL destination so realpath allocates the buffer to avoid
   * potential overflow if the resolved path exceeds PATH_MAX. */
  char* resolved_raw = realpath(exe_file, nullptr);

  if (resolved_raw == nullptr)
  {
    if (verbose)
    {
      puts("Couldn't resolve executable path, using default: " DATA_PREFIX);
    }
    datadir = DATA_PREFIX;
    return;
  }

  const std::string resolved_path(resolved_raw);
  free(resolved_raw);

  datadir = find_datadir_near(fs::path(resolved_path).parent_path());
}

#endif // ifndef WIN32

/**
 * Set SuperTux configuration and save directories (non HBC Wii)
 * This sets up the directory structure, including the base directory and
 * save directory. It handles home directory detection, creation of
 * necessary directories, and datadir detection on Linux systems.
 */
void st_directory_setup(void)
{
  st_dir = resolve_home_directory() + "/.supertux";

  /* Remove .supertux config-file from old SuperTux versions */
  if (faccessible(st_dir.c_str()))
  {
    fs::remove(st_dir.c_str());
  }

  st_save_dir = st_dir + "/save";

  /* Create directories. If they exist, they won't be destroyed. */
  fs::create_directories(st_dir.c_str());
  fs::create_directories(st_save_dir.c_str());

  fs::create_directories((st_dir + "/levels").c_str()); // Ensure 'levels' directory exists

#ifndef WIN32
  // Handle datadir detection logic (Linux, macOS, BSD)
  if (datadir.empty())
  {
    detect_datadir();
  }
#else // #ifdef WIN32
  // For Windows, use default data path
  datadir = "data";
#endif

  if (verbose)
  {
    printf("st_dir: %s\n", st_dir.c_str());
    printf("st_save_dir: %s\n", st_save_dir.c_str());
    printf("Datadir: %s\n", datadir.c_str());
  }
}

#endif // def __WII__

// EOF
