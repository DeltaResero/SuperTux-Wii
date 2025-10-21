//  mousecursor.h
//
//  SuperTux
//  Copyright (C) 2004 Ricardo Cruz <rick2@aeiou.pt>
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef SUPERTUX_MOUSECURSOR_H
#define SUPERTUX_MOUSECURSOR_H

#include <string>
#include "timer.h"
#include "texture.h"

#define MC_FRAME_PERIOD 800  // Frame period in ms
#define MC_STATES_NB 3       // Number of cursor states

enum {
  MC_NORMAL,
  MC_CLICK,
  MC_LINK
};

class MouseCursor
{
public:
  MouseCursor(std::string cursor_file, int frames);
  ~MouseCursor();

  int state() const; // Returns the current state of the cursor
  void set_state(int nstate); // Sets the cursor's state
  void set_mid(int x, int y); // Sets the midpoint of the cursor
  void draw(); // Draws the cursor on the screen

  static MouseCursor* current() { return current_; };
  static void set_current(MouseCursor* pcursor) { current_ = pcursor; };

private:
  int mid_x, mid_y;
  static MouseCursor* current_;
  int state_before_click;
  int cur_state;
  int cur_frame, tot_frames;
  Surface* cursor;
  Timer timer;

  // Cache frame dimensions to avoid recalculating every frame
  int frame_w;
  int frame_h;
};

#endif /*SUPERTUX_MOUSECURSOR_H*/

// EOF
