//  timer.cpp
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

#include "SDL.h"
#include "defines.h"
#include "timer.h"

// Global variables to manage the game's pause state.
Uint32 st_pause_ticks = 0, st_pause_count = 0;

/**
 * Get the current game time in ticks, accounting for any time the game has been paused.
 * This function should be used for all game logic that needs to pause, such as
 * animations, physics, and character movement, to ensure they freeze correctly
 * when a menu is opened or the game is paused.
 * @return The adjusted number of ticks, as if the game was never paused.
 */
Uint32 st_get_ticks(void)
{
  if (st_pause_count != 0)
  {
    // If the game is currently paused (st_pause_count is non-zero), the flow of
    // game time is frozen. We return a constant value representing the exact moment
    // the game was paused. This is calculated as the timestamp when the pause began
    // (st_pause_count) minus the total duration of all *previous* pauses (st_pause_ticks).
    return st_pause_count - st_pause_ticks;
  }
  else
  {
    // If the game is running, the total elapsed game time is the current raw
    // hardware time (from SDL_GetTicks) minus the total accumulated duration
    // of all previous pause sessions.
    return SDL_GetTicks() - st_pause_ticks;
  }
}

/**
 * Initializes or resets the global pause timer variables. This should be called
 * once at game startup to ensure the pause system starts in a clean state.
 */
void st_pause_ticks_init(void)
{
  st_pause_ticks = 0;
  st_pause_count = 0;
}

/**
 * Starts a pause session. This function is called whenever the game enters a
 * state where game time should stop advancing (e.g., opening the main menu).
 * It records the current time as the moment the pause began.
 * It does nothing if the game is already in a paused state.
 */
void st_pause_ticks_start(void)
{
  if (st_pause_count == 0)
  {
    st_pause_count = SDL_GetTicks();
  }
}

/**
 * Stops a pause session and updates the total accumulated pause time. This is
 * called when the game resumes (e.g., closing a menu). It calculates the
 * duration of the pause that just ended and adds it to our total.
 */
void st_pause_ticks_stop(void)
{
  if (st_pause_count == 0)
  {
    // Can't stop a pause that hasn't started, so we do nothing.
    return;
  }

  // Add the duration of this pause session (current time minus start time)
  // to the total accumulator for paused time.
  st_pause_ticks += SDL_GetTicks() - st_pause_count;

  // Reset st_pause_count to 0. This non-zero check is how the rest of the
  // system knows that the game is no longer paused.
  st_pause_count = 0;
}

/**
 * Checks if the game is currently in a paused state.
 * @return True if the game is paused, false otherwise.
 */
bool st_pause_ticks_started(void)
{
  return st_pause_count != 0;
}

/**
 * Constructor for the Timer class.
 * Initializes the timer to a stopped, default state. By default, it's configured
 * to use the pausable game clock (st_get_ticks).
 */
Timer::Timer()
: period(0), time(0), use_st_ticks(true)
{
  // All members are initialized in the initializer list above for efficiency.
}

/**
 * Initialize or re-initialize the timer, setting its timing mode.
 * @param st_ticks If true, the timer will use the pausable game clock (st_get_ticks).
 *                 If false, it will use the raw, unpausable hardware clock (SDL_GetTicks).
 */
void Timer::init(bool st_ticks)
{
  period = 0;
  time = 0;
  use_st_ticks = st_ticks;
}

/**
 * Starts the timer with a given period. This records the current time and sets
 * the duration for which the timer should run.
 * @param period_ The duration for which the timer should run, in milliseconds.
 */
void Timer::start(Uint32 period_)
{
  time = get_current_ticks();
  period = period_;
}

/**
 * Stops the timer immediately.
 * This simply resets the timer's internal state to its initial, non-running values,
 * making it inactive until it is started again. This is more efficient than the
 * previous implementation that called init().
 */
void Timer::stop()
{
  time = 0;
  period = 0;
}

/**
 * Checks if the timer is still running and has time remaining.
 * @return True if the timer was started and its period has not yet elapsed.
 *         Returns false and stops the timer if it has expired. This is useful for
 *         one-shot timer checks in loops.
 */
bool Timer::check()
{
  // A timer is running if its 'time' is not zero.
  if (time != 0 && (time + period > get_current_ticks()))
  {
    // The timer is active and has time remaining.
    return true;
  }
  else
  {
    // The timer has expired or was never started.
    time = 0; // Ensure the timer is now considered stopped.
    return false;
  }
}

/**
 * Checks if the timer has been started, without modifying its state.
 * @return True if the timer has been started (even if it has since expired).
 *         Returns false only if the timer is stopped or has never been started.
 */
bool Timer::started() const
{
  return time != 0;
}

/**
 * Gets the time remaining on the timer before it expires.
 * @return The time left in milliseconds. This value can be negative if the timer
 *         has already expired.
 */
int Timer::get_left() const
{
  if (time == 0)
  {
    return 0; // If the timer is not running, there is no time left.
  }

  // The time remaining is the total period minus the time that has passed so far.
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
    return 0; // If the timer is not running, no time has gone by.
  }
  return get_current_ticks() - time;
}

/**
 * Saves the timer's current state to a file for use in savegames.
 * @param fi The file pointer to write to.
 */
void Timer::fwrite(FILE* fi) const
{
  // It is unsafe to save the absolute 'time' timestamp, as it would be invalid
  // when the game is loaded later. Instead, we save the amount of time that
  // has already passed ('diff_ticks').
  Uint32 diff_ticks = (time != 0) ? (get_current_ticks() - time) : 0;

  // We must also save whether the timer uses the pausable clock or the real-time clock.
  Uint32 tick_mode = use_st_ticks ? 1 : 0;

  ::fwrite(&period, sizeof(Uint32), 1, fi);
  ::fwrite(&diff_ticks, sizeof(Uint32), 1, fi);
  ::fwrite(&tick_mode, sizeof(Uint32), 1, fi);
}

/**
 * Loads the timer's state from a file, correctly resuming its countdown.
 * @param fi The file pointer to read from.
 */
void Timer::fread(FILE* fi)
{
  Uint32 diff_ticks;
  Uint32 tick_mode;

  ::fread(&period, sizeof(Uint32), 1, fi);
  ::fread(&diff_ticks, sizeof(Uint32), 1, fi);
  ::fread(&tick_mode, sizeof(Uint32), 1, fi);

  use_st_ticks = (tick_mode != 0);

  // To correctly resume the timer, we reconstruct the original start 'time'
  // by taking the current time and subtracting the elapsed time that was saved.
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
