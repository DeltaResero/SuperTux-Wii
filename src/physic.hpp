// src/physic.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_PHYSIC_H
#define SUPERTUX_PHYSIC_H

/**
 * @class Physic
 * @brief A very simplistic physics engine handling accelerated and constant
 *        movement along with gravity.
 *
 * This class is designed to be a "dumb" simulator. It only applies the
 * mathematical rules of motion and contains no game-specific logic. Its
 * methods are lightweight and suitable for frequent calls within a game loop.
 */
class Physic
{
public:
  // Use the default constructor and destructor.
  // In-class initializers set the default state.
  Physic() = default;
  ~Physic() = default;

  /** Resets all velocities and accelerations to 0 and re-enables gravity. */
  inline void reset();

  /** Sets velocity to a fixed value. */
  inline void set_velocity(float vx, float vy);

  /** Sets the horizontal velocity. */
  inline void set_velocity_x(float vx);

  /** Sets the vertical velocity. */
  inline void set_velocity_y(float vy);

  /** Inverts the horizontal velocity. */
  inline void inverse_velocity_x();

  /** Inverts the vertical velocity. */
  inline void inverse_velocity_y();

  /** Gets the current horizontal velocity. */
  inline float get_velocity_x() const;

  /** Gets the current vertical velocity. */
  inline float get_velocity_y() const;

  /** Sets acceleration applied to the object. */
  inline void set_acceleration(float ax, float ay);

  /** Sets the horizontal acceleration. */
  inline void set_acceleration_x(float ax);

  /** Sets the vertical acceleration. */
  inline void set_acceleration_y(float ay);

  /** Gets the current horizontal acceleration. */
  inline float get_acceleration_x() const;

  /** Gets the current vertical acceleration. */
  inline float get_acceleration_y() const;

  /** Enables or disables handling of gravity. */
  inline void enable_gravity(bool is_gravity_enabled);

  /**
   * Applies the physical simulation to given x and y coordinates.
   * This is the only function not inlined due to its complexity and dependency
   * on the World state.
   */
  void apply(float frame_ratio, float &x, float &y);

private:
  // Use C++11 in-class member initializers to define the default state.
  float ax{0.0f};          // horizontal acceleration
  float ay{0.0f};          // vertical acceleration
  float vx{0.0f};          // horizontal velocity
  float vy{0.0f};          // vertical velocity
  bool gravity_enabled{true}; // should we respect gravity in our calculations?
};

// Inlined function implementations for performance

inline void Physic::reset()
{
  ax = ay = vx = vy = 0.0f;
  gravity_enabled = true;
}

inline void Physic::set_velocity(float nvx, float nvy)
{
  vx = nvx;
  vy = nvy;
}

inline void Physic::set_velocity_x(float nvx)
{
  vx = nvx;
}

inline void Physic::set_velocity_y(float nvy)
{
  vy = nvy;
}

inline void Physic::inverse_velocity_x()
{
  vx = -vx;
}

inline void Physic::inverse_velocity_y()
{
  vy = -vy;
}

inline float Physic::get_velocity_x() const
{
  return vx;
}

inline float Physic::get_velocity_y() const
{
  return vy;
}

inline void Physic::set_acceleration(float nax, float nay)
{
  ax = nax;
  ay = nay;
}

inline void Physic::set_acceleration_x(float nax)
{
  ax = nax;
}

inline void Physic::set_acceleration_y(float nay)
{
  ay = nay;
}

inline float Physic::get_acceleration_x() const
{
  return ax;
}

inline float Physic::get_acceleration_y() const
{
  return ay;
}

inline void Physic::enable_gravity(bool is_gravity_enabled)
{
  gravity_enabled = is_gravity_enabled;
}

#endif /*SUPERTUX_PHYSIC_H*/

// EOF
