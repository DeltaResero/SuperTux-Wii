//  utils.h
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

#ifndef UTILS_H
#define UTILS_H

#include <cmath>   // For M_PI, sin, cos
#include <cstring> // For memcpy and strlen
#include <string>  // For std::string


// A namespace for pre-calculated trigonometry tables
namespace Trig
{
  constexpr int ANGLE_COUNT = 256; // Number of steps in the circle (must be a power of 2)

  // Declare the tables to be defined in utils.cpp
  extern float sin_table[ANGLE_COUNT];
  extern float cos_table[ANGLE_COUNT];

  // Function to fill the tables with values
  void initialize();

  // Fast inline lookup functions
  inline float fast_sin(int angle_index)
  {
    // Use a bitwise AND for a fast modulo operation (works as ANGLE_COUNT is a power of 2)
    return sin_table[angle_index & (ANGLE_COUNT - 1)];
  }

  inline float fast_cos(int angle_index)
  {
    return cos_table[angle_index & (ANGLE_COUNT - 1)];
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
std::string get_title_from_lisp_file(const std::string& path, const std::string& invalid_fallback, const std::string& untitled_fallback);


#endif // UTILS_H

// EOF
