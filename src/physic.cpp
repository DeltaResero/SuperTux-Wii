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

#include <stdio.h>

#include "scene.h"
#include "defines.h"
#include "physic.h"
#include "timer.h"
#include "world.h"
#include "level.h"

/**
 * Constructs a Physic object.
 * Initializes all physical properties to zero or default values.
 */
Physic::Physic()
: ax(0), ay(0), vx(0), vy(0), gravity_enabled(true)
{
}

/**
 * Destructor for the Physic object.
 */
Physic::~Physic()
{
}

/**
 * Resets all velocities and accelerations to 0 and re-enables gravity.
 */
void Physic::reset()
{
  ax = ay = vx = vy = 0;
  gravity_enabled = true;
}

/**
 * Sets the horizontal velocity.
 * @param nvx The new horizontal velocity.
 */
void Physic::set_velocity_x(float nvx)
{
  vx = nvx;
}

/**
 * Sets the vertical velocity.
 * Note: This function uses a "Y-up" coordinate system where a positive
 * value means upward movement, which is inverted for internal storage.
 * @param nvy The new vertical velocity.
 */
void Physic::set_velocity_y(float nvy)
{
  vy = -nvy;
}

/**
 * Sets both the horizontal and vertical velocity.
 * @param nvx The new horizontal velocity.
 * @param nvy The new vertical velocity (Y-up).
 */
void Physic::set_velocity(float nvx, float nvy)
{
  vx = nvx;
  vy = -nvy;
}

/**
 * Inverts the horizontal velocity.
 */
void Physic::inverse_velocity_x()
{
  vx = -vx;
}

/**
 * Inverts the vertical velocity.
 */
void Physic::inverse_velocity_y()
{
  vy = -vy;
}

/**
 * Gets the current horizontal velocity.
 * @return The horizontal velocity.
 */
float Physic::get_velocity_x()
{
  return vx;
}

/**
 * Gets the current vertical velocity.
 * @return The vertical velocity in a "Y-up" coordinate system.
 */
float Physic::get_velocity_y()
{
  return -vy;
}

/**
 * Sets the horizontal acceleration.
 * @param nax The new horizontal acceleration.
 */
void Physic::set_acceleration_x(float nax)
{
  ax = nax;
}

/**
 * Sets the vertical acceleration.
 * Note: This function uses a "Y-up" coordinate system.
 * @param nay The new vertical acceleration.
 */
void Physic::set_acceleration_y(float nay)
{
  ay = -nay;
}

/**
 * Sets both the horizontal and vertical acceleration.
 * @param nax The new horizontal acceleration.
 * @param nay The new vertical acceleration (Y-up).
 */
void Physic::set_acceleration(float nax, float nay)
{
  ax = nax;
  ay = -nay;
}

/**
 * Gets the current horizontal acceleration.
 * @return The horizontal acceleration.
 */
float Physic::get_acceleration_x()
{
  return ax;
}

/**
 * Gets the current vertical acceleration.
 * @return The vertical acceleration in a "Y-up" coordinate system.
 */
float Physic::get_acceleration_y()
{
  return -ay;
}

/**
 * Enables or disables the effect of gravity on this object.
 * @param enable_gravity True to enable gravity, false to disable.
 */
void Physic::enable_gravity(bool enable_gravity)
{
  gravity_enabled = enable_gravity;
}

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
  float gravity = World::current()->get_level()->gravity;
  float grav;
  if (gravity_enabled)
  {
    grav = gravity / 100.0f;
  }
  else
  {
    grav = 0;
  }

  // Update the object's position based on its current velocity and acceleration.
  // This uses a basic Euler integration method.
  x += vx * frame_ratio + ax * frame_ratio * frame_ratio;
  y += vy * frame_ratio + (ay + grav) * frame_ratio * frame_ratio;

  // Update the object's velocity for the next frame.
  vx += ax * frame_ratio;
  vy += (ay + grav) * frame_ratio;
}

// EOF
