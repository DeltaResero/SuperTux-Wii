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
#include <cmath>   // For M_PI, sin, cos

#ifndef M_PI // Define M_PI if it's not pulled in from <cmath>...
#define M_PI 3.14159265358979323846
#endif // ...as apparently M_PI isn't part of the official C++ standard and isn't guaranteed to be here


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
      float angle = (2.0f * M_PI * static_cast<float>(i)) / ANGLE_COUNT;
      sin_table[i] = std::sin(angle);
      cos_table[i] = std::cos(angle);
    }
  }
} // namespace Trig

// EOF
