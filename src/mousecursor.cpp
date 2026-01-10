// src/mousecursor.cpp
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

#include "screen.hpp"
#include "mousecursor.hpp"

MouseCursor* MouseCursor::current_ = nullptr;  // Initialize static member to nullptr

/**
 * Constructs a MouseCursor object.
 * @param cursor_file The file path to the cursor image.
 * @param frames The number of animation frames for the cursor.
 * Loads the cursor image and initializes its state and position.
 */
MouseCursor::MouseCursor(std::string cursor_file, int frames)
  : mid_x(0), mid_y(0), cur_state(MC_NORMAL), cur_frame(0), tot_frames(frames),
    frame_w(0), frame_h(0)  // Initialize cached dimensions
{
  cursor = new Surface(cursor_file, true);
  if (!cursor)
  {
#ifdef DEBUG
    printf("Failed to load cursor image: %s\n", cursor_file.c_str());
#endif
    return;
  }

  // Calculate and cache the dimensions of a single animation frame
  // Avoids doing this division on every draw() call
  if (tot_frames > 0)
  {
    frame_w = cursor->w / tot_frames;
  }
  frame_h = cursor->h / MC_STATES_NB;

  timer.init(false);  // Initialize the timer without starting it
  timer.start(MC_FRAME_PERIOD);  // Start the timer with the frame period

  SDL_ShowCursor(SDL_DISABLE);  // Disable the default SDL cursor
}

/**
 * Destructor for MouseCursor.
 * Frees the cursor surface memory and re-enables the default SDL cursor.
 */
MouseCursor::~MouseCursor()
{
  delete cursor;  // Free the cursor surface memory

  SDL_ShowCursor(SDL_ENABLE);  // Re-enable the default SDL cursor
}

/**
 * Returns the current state of the cursor.
 * @return The current state of the cursor as an integer.
 */
int MouseCursor::state() const
{
  return cur_state;
}

/**
 * Sets the cursor's state.
 * @param nstate The new state to set for the cursor.
 */
void MouseCursor::set_state(int nstate)
{
  cur_state = nstate;
}

/**
 * Sets the midpoint of the cursor.
 * @param x The x-coordinate of the cursor midpoint.
 * @param y The y-coordinate of the cursor midpoint.
 * This determines the cursor's anchor point on the screen.
 */
void MouseCursor::set_mid(int x, int y)
{
  mid_x = x;
  mid_y = y;
}

/**
 * Draws the cursor on the screen.
 * The cursor is drawn based on its current state and position.
 * The cursor's frame is updated periodically based on a timer.
 */
void MouseCursor::draw()
{
  int x, y;
  Uint8 ispressed = SDL_GetMouseState(&x, &y);  // Get the mouse position and button state

  // Note: The width and height of the cursor frame ('frame_w', 'frame_h') are now
  // pre-calculated and cached in the constructor, so we don't recalculate them here.

  // Check if any mouse button is pressed
  if(ispressed & SDL_BUTTON(1) || ispressed & SDL_BUTTON(2))
  {
    if(cur_state != MC_CLICK)
    {
      state_before_click = cur_state;
      cur_state = MC_CLICK;  // Switch to the "click" state
    }
  }
  else
  {
    if(cur_state == MC_CLICK)
    {
      cur_state = state_before_click;  // Revert to the previous state if no buttons are pressed
    }
  }

  // Update the current frame based on the timer
  if(timer.get_left() < 0 && tot_frames > 1)
  {
    cur_frame++;
    if(cur_frame >= tot_frames)
    {
      cur_frame = 0;
    }

    timer.start(MC_FRAME_PERIOD);  // Restart the timer for the next frame
  }

  // Draw the appropriate part of the cursor using the cached frame dimensions.
  cursor->draw_part(frame_w * cur_frame, frame_h * cur_state, x - mid_x, y - mid_y, frame_w, frame_h);
}

// EOF
