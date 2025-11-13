//  physic.cpp
//
//  SuperTux
//  Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
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

#include "physic.h"
#include "world.h"
#include "level.h"

// The constructor and destructor are defaulted in the header file.
// All simple getters and setters have been inlined for performance.

/**
 * Applies the physical simulation to an object's coordinates.
 * This function updates the position and velocity based on the current
 * acceleration and gravity over a given time delta.
 * @param frame_ratio The time delta for the current frame.
 * @param x The horizontal position of the object (passed by reference).
 * @param y The vertical position of the object (passed by reference).
 */
void Physic::apply(float frame_ratio, float &x, float &y)
{
  // Determine the gravitational force for this frame.
  // Note: This logic is unchanged to maintain behavioral equivalence.
  const float gravity = World::current()->get_level()->gravity;
  const float grav = gravity_enabled ? (gravity / 100.0f) : 0.0f;

  // Update velocity based on acceleration and gravity.
  // This uses a semi-implicit Euler integration method.
  vx += ax * frame_ratio;
  vy += (ay + grav) * frame_ratio;

  // Update position using the *newly calculated* velocity.
  // This is more stable than using the velocity from the previous frame.
  x += vx * frame_ratio;
  y += vy * frame_ratio;
}

// EOF
