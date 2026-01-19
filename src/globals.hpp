// src/globals.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2004 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_GLOBALS_H
#define SUPERTUX_GLOBALS_H

#include <string>
#include <SDL2/SDL.h>
#include "text.hpp"
#include "menu.hpp"
#include "mousecursor.hpp"

// Loading Screen as Supertux takes a long, long time to load on Wii
extern Surface* loading_surf;

extern std::string datadir;

struct JoystickKeymap
{
  int a_button{0};
  int b_button{1};
  int start_button{2};

  int x_axis{0};
  int y_axis{1};

  int dead_zone{8192};

  JoystickKeymap();
};

extern JoystickKeymap joystick_keymap;

extern SDL_Surface* screen;
extern Text* black_text;
extern Text* gold_text;
extern Text* white_text;
extern Text* white_small_text;
extern Text* white_big_text;
extern Text* blue_text;

extern MouseCursor* mouse_cursor;

extern bool use_gl;
extern bool use_joystick;
extern bool use_fullscreen;
extern bool debug_mode;
extern bool show_fps;
extern bool show_mouse;
extern bool tv_overscan_enabled;
extern int offset_y;

/** The number of the joystick that will be use in the game */
extern int joystick_num;
extern std::string level_startup_file;

/* SuperTux directory ($HOME/.supertux) and save directory($HOME/.supertux/save) */
extern std::string st_dir;
extern std::string st_save_dir;

extern float game_speed;
extern SDL_Joystick* js;

// Flag to track Wii Remote controller state
extern bool is_nunchuk_connected;

// Helper to rotate D-Pad if Nunchuk is missing from Wii Remote
Uint8 adjust_joystick_hat(Uint8 hat);

int wait_for_event(SDL_Event& event, unsigned int min_delay = 0, unsigned int max_delay = 0, bool empty_events = false);

void draw_player_hud();

void st_wii_input_init();
int st_poll_event(SDL_Event *event);

#endif /* SUPERTUX_GLOBALS_H */

// EOF
