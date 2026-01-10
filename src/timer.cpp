// src/timer.cpp
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

#include <SDL.h>
#include "defines.hpp"
#include "timer.hpp"

// Static member definitions for the Ticks class.
Uint32 Ticks::pause_ticks = 0, Ticks::pause_count = 0;

/**
 * Get the current game time in ticks, accounting for any time the game has been paused.
 * This function should be used for all game logic that needs to pause, such as
 * animations, physics, and character movement, to ensure they freeze correctly
 * when a menu is opened or the game is paused.
 * @return The adjusted number of ticks, as if the game was never paused.
 */
Uint32 Ticks::get(void)
{
  if (Ticks::pause_count != 0)
  {
    // If the game is currently paused, the flow of game time is frozen.
    // Return a constant value representing the moment the pause began.
    return Ticks::pause_count - Ticks::pause_ticks;
  }
  else
  {
    // If the game is running, the total elapsed game time is the current raw
    // hardware time minus the total accumulated duration of all previous pauses.
    return SDL_GetTicks() - Ticks::pause_ticks;
  }
}

/**
 * Initializes or resets the global pause timer variables. This should be called
 * once at game startup to ensure the pause system starts in a clean state.
 */
void Ticks::pause_init(void)
{
  Ticks::pause_ticks = 0;
  Ticks::pause_count = 0;
}

/**
 * Starts a pause session. This function is called whenever the game enters a
 * state where game time should stop advancing (e.g., opening a menu).
 * It records the current time as the moment the pause began.
 */
void Ticks::pause_start(void)
{
  if (Ticks::pause_count == 0)
  {
    Ticks::pause_count = SDL_GetTicks();
  }
}

/**
 * Stops a pause session and updates the total accumulated pause time. This is
 * called when the game resumes (e.g., closing a menu).
 */
void Ticks::pause_stop(void)
{
  if (Ticks::pause_count == 0)
  {
    // Can't stop a pause that hasn't started.
    return;
  }

  // Add the duration of this pause session to the total accumulator.
  Ticks::pause_ticks += SDL_GetTicks() - Ticks::pause_count;

  // Reset pause_count to 0 to indicate the game is no longer paused.
  Ticks::pause_count = 0;
}

/**
 * Checks if the game is currently in a paused state.
 * @return True if the game is paused, false otherwise.
 */
bool Ticks::pause_started(void)
{
  return Ticks::pause_count != 0;
}

/**
 * Constructor for the Timer class.
 * Initializes the timer to a stopped, default state.
 */
Timer::Timer()
: period(0), time(0), use_game_ticks(true)
{
  // All members are initialized in the initializer list.
}

/**
 * Initialize or re-initialize the timer, setting its timing mode.
 * @param game_ticks If true, the timer will use the pausable game clock (Ticks::get).
 *                   If false, it will use the raw, unpausable hardware clock (SDL_GetTicks).
 */
void Timer::init(bool game_ticks)
{
  period = 0;
  time = 0;
  use_game_ticks = game_ticks;
}

/**
 * Starts the timer with a given period.
 * @param period_ The duration for which the timer should run, in milliseconds.
 */
void Timer::start(Uint32 period_)
{
  time = get_current_ticks();
  period = period_;
}

/**
 * Stops the timer immediately.
 */
void Timer::stop()
{
  time = 0;
  period = 0;
}

/**
 * Checks if the timer is still running and has time remaining.
 * @return True if the timer is active and has not yet expired.
 */
bool Timer::check()
{
  if (time != 0 && (time + period > get_current_ticks()))
  {
    return true;
  }
  else
  {
    time = 0; // Ensure the timer is now considered stopped if expired.
    return false;
  }
}

/**
 * Checks if the timer has been started, without modifying its state.
 * @return True if the timer has been started (even if it has since expired).
 */
bool Timer::started() const
{
  return time != 0;
}

/**
 * Gets the time remaining on the timer before it expires.
 * @return The time left in milliseconds (can be negative).
 */
int Timer::get_left() const
{
  if (time == 0)
  {
    return 0;
  }
  return period - (get_current_ticks() - time);
}

/**
 * Gets the time that has elapsed since the timer was started.
 * @return The elapsed time in milliseconds.
 */
int Timer::get_gone() const
{
  if (time == 0)
  {
    return 0;
  }
  return get_current_ticks() - time;
}

/**
 * Saves the timer's current state to a file.
 * @param fi The file pointer to write to.
 */
void Timer::fwrite(FILE* fi) const
{
  Uint32 diff_ticks = (time != 0) ? (get_current_ticks() - time) : 0;
  Uint32 tick_mode = use_game_ticks ? 1 : 0;

  ::fwrite(&period, sizeof(Uint32), 1, fi);
  ::fwrite(&diff_ticks, sizeof(Uint32), 1, fi);
  ::fwrite(&tick_mode, sizeof(Uint32), 1, fi);
}

/**
 * Loads the timer's state from a file.
 * @param fi The file pointer to read from.
 */
void Timer::fread(FILE* fi)
{
  Uint32 diff_ticks;
  Uint32 tick_mode;

  ::fread(&period, sizeof(Uint32), 1, fi);
  ::fread(&diff_ticks, sizeof(Uint32), 1, fi);
  ::fread(&tick_mode, sizeof(Uint32), 1, fi);

  use_game_ticks = (tick_mode != 0);

  if (diff_ticks != 0)
  {
    time = get_current_ticks() - diff_ticks;
  }
  else
  {
    time = 0;
  }
}

// EOF
