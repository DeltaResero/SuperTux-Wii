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

Uint32 st_pause_ticks = 0, st_pause_count = 0;

/**
 * Get the current ticks, adjusting for paused time.
 * @return Adjusted SDL ticks.
 */
Uint32 st_get_ticks(void)
{
  if (st_pause_count != 0)
  {
    return /*SDL_GetTicks()*/ - st_pause_ticks /*- SDL_GetTicks()*/ + st_pause_count;
  }
  else
  {
    return SDL_GetTicks() - st_pause_ticks;
  }
}

/**
 * Initialize the paused ticks.
 */
void st_pause_ticks_init(void)
{
  st_pause_ticks = 0;
  st_pause_count = 0;
}

/**
 * Start tracking pause time.
 */
void st_pause_ticks_start(void)
{
  if (st_pause_count == 0)
  {
    st_pause_count = SDL_GetTicks();
  }
}

/**
 * Stop tracking pause time and adjust the pause ticks accordingly.
 */
void st_pause_ticks_stop(void)
{
  if (st_pause_count == 0)
  {
    return;
  }

  st_pause_ticks += SDL_GetTicks() - st_pause_count;
  st_pause_count = 0;
}

/**
 * Check if the ticks are currently paused.
 * @return True if paused, false otherwise.
 */
bool st_pause_ticks_started(void)
{
  return st_pause_count != 0;
}

/**
 * Constructor for Timer class.
 */
Timer::Timer()
{
  init(true);
}

/**
 * Initialize the timer with either SDL or SuperTux ticks.
 * @param st_ticks True for SuperTux ticks, false for SDL ticks.
 */
void Timer::init(bool st_ticks)
{
  period = 0;
  time = 0;
  get_ticks = st_ticks ? st_get_ticks : SDL_GetTicks;
}

/**
 * Start the timer with a given period.
 * @param period_ The duration in milliseconds.
 */
void Timer::start(Uint32 period_)
{
  time = get_ticks();
  period = period_;
}

/**
 * Stop the timer and reinitialize it based on its tick source.
 */
void Timer::stop()
{
  init(get_ticks == st_get_ticks);
}

/**
 * Check if the timer is still running.
 * @return True if running, false otherwise.
 */
int Timer::check()
{
  if ((time != 0) && (time + period > get_ticks()))
  {
    return true;
  }
  else
  {
    time = 0;
    return false;
  }
}

/**
 * Check if the timer has been started.
 * @return True if started, false otherwise.
 */
int Timer::started()
{
  return time != 0;
}

/**
 * Get the time remaining on the timer.
 * @return Time left in milliseconds, can be negative if expired.
 */
int Timer::get_left()
{
  return period - (get_ticks() - time);
}

/**
 * Get the time elapsed since the timer started.
 * @return Time elapsed in milliseconds.
 */
int Timer::get_gone()
{
  return get_ticks() - time;
}

/**
 * Save the timer's current state to a file.
 * @param fi The file pointer to write to.
 */
void Timer::fwrite(FILE* fi)
{
  Uint32 diff_ticks = (time != 0) ? (get_ticks() - time) : 0;
  int tick_mode = (get_ticks == st_get_ticks);

  ::fwrite(&period, sizeof(Uint32), 1, fi);
  ::fwrite(&diff_ticks, sizeof(Uint32), 1, fi);
  ::fwrite(&tick_mode, sizeof(Uint32), 1, fi);
}

/**
 * Load the timer's state from a file.
 * @param fi The file pointer to read from.
 */
void Timer::fread(FILE* fi)
{
  Uint32 diff_ticks;
  int tick_mode;

  ::fread(&period, sizeof(Uint32), 1, fi);
  ::fread(&diff_ticks, sizeof(Uint32), 1, fi);
  ::fread(&tick_mode, sizeof(Uint32), 1, fi);

  get_ticks = tick_mode ? st_get_ticks : SDL_GetTicks;
  time = (diff_ticks != 0) ? (get_ticks() - diff_ticks) : 0;
}

// EOF
