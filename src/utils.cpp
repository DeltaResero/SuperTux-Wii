// utils.cpp
//
//  SuperTux
//  Copyright (C) 2024 DeltaResero
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//  02111-1307, USA.

#include "utils.h"
#include <cstring> // For memcpy, memchr
#include <cmath>   // For std::sin, std::cos
#include <numbers> // For std::numbers::pi_v
#include <fstream> // For std::ifstream

namespace Trig
{
  // Define the actual storage for the tables
  float sin_table[ANGLE_COUNT];
  float cos_table[ANGLE_COUNT];

  // Fills the tables with pre-calculated values
  void initialize()
  {
    for (int i = 0; i < ANGLE_COUNT; ++i)
    {
      // Calculate the angle in radians for the current index
      float angle = (2.0f * std::numbers::pi_v<float> * static_cast<float>(i)) / ANGLE_COUNT;
      sin_table[i] = std::sin(angle);
      cos_table[i] = std::cos(angle);
    }
  }
} // namespace Trig

/**
 * A lightweight "peek" function that reads just enough of a lisp-like file
 * to extract its title, avoiding a full parse.
 * @param path The full path to the file.
 * @param invalid_fallback The string to return if the file can't be opened.
 * @param untitled_fallback The string to return if the title isn't found.
 * @return The title of the file.
 */
std::string get_title_from_lisp_file(const std::string& path, const std::string& invalid_fallback, const std::string& untitled_fallback)
{
  std::ifstream file(path);
  if (!file.is_open())
  {
    return invalid_fallback;
  }

  std::string line;
  // Search only the first 20 lines for performance. The name
  // is always in the properties section near the top.
  for (int i = 0; i < 20 && std::getline(file, line); ++i)
  {
    // Find the line containing "(name"
    size_t pos = line.find("(name");
    if (pos != std::string::npos)
    {
      // Find the first quote after "(name"
      size_t start_quote = line.find('"', pos);
      if (start_quote != std::string::npos)
      {
        // Find the second quote that closes the string
        size_t end_quote = line.find('"', start_quote + 1);
        if (end_quote != std::string::npos)
        {
          // We found the title! Extract it and return immediately.
          return line.substr(start_quote + 1, end_quote - start_quote - 1);
        }
      }
    }
  }

  return untitled_fallback; // Fallback title
}

// EOF
