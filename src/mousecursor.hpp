// src/mousecursor.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2004 Ricardo Cruz <rick2@aeiou.pt>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_MOUSECURSOR_H
#define SUPERTUX_MOUSECURSOR_H

#include <string>
#include "timer.hpp"
#include "texture.hpp"

inline constexpr int MC_FRAME_PERIOD = 800;  // Frame period in ms
inline constexpr int MC_STATES_NB = 3;       // Number of cursor states

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
