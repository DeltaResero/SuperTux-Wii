//  timer.h
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

#ifndef SUPERTUX_TIMER_H
#define SUPERTUX_TIMER_H

#include "SDL.h"
#include <stdio.h>

// Global variables used to track the total amount of time the game has been paused.
extern Uint32 st_pause_ticks, st_pause_count;

Uint32 st_get_ticks(void);
void st_pause_ticks_init(void);
void st_pause_ticks_start(void);
void st_pause_ticks_stop(void);
bool st_pause_ticks_started(void);

// A general-purpose timer for managing time-based events.
class Timer
{
private:
  // The duration of the timer in milliseconds.
  Uint32 period;

  // The timestamp (in ticks) when the timer was started.
  Uint32 time;

  // If true, the timer will pause along with the game (using st_get_ticks).
  // If false, it uses the raw SDL hardware timer and will not pause.
  bool use_st_ticks;

  // An inline helper function to get the current time from the correct source.
  inline Uint32 get_current_ticks() const
  {
    return use_st_ticks ? st_get_ticks() : SDL_GetTicks();
  }

public:
  Timer();

  void init(bool st_ticks);
  void start(Uint32 period);
  void stop();

  // Returns true if the timer is active and has not yet expired.
  bool check();

  // Returns true if the timer has been started (even if expired).
  bool started() const;

  // Returns the time left in milliseconds (can be negative).
  int get_left() const;

  // Returns the elapsed time in milliseconds since the timer started.
  int get_gone() const;

  void fwrite(FILE* fi) const;
  void fread(FILE* fi);
};

#endif /*SUPERTUX_TIMER_H*/

// EOF
