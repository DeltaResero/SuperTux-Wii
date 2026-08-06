// src/globals.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2004 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
// Copyright (C) 2025-2026 DeltaResero
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
extern std::unique_ptr<Surface> loading_surf;

extern std::string datadir;

struct JoystickKeymap
{
  int x_axis{0};
  int y_axis{1};

  int dead_zone{8192};
};

extern JoystickKeymap joystick_keymap;

extern SDL_Surface* screen;
extern std::unique_ptr<Text> black_text;
extern std::unique_ptr<Text> gold_text;
extern std::unique_ptr<Text> white_text;
extern std::unique_ptr<Text> white_small_text;
extern std::unique_ptr<Text> white_big_text;
extern std::unique_ptr<Text> blue_text;

extern std::unique_ptr<MouseCursor> mouse_cursor;

extern bool use_gl;
extern bool use_joystick;
extern bool use_fullscreen;
extern bool debug_mode;
extern bool show_fps;
extern bool tv_overscan_enabled;
extern int offset_y;

/** Developer console output, compiled in for debug builds only.
    Unrelated to debug_mode above, which unlocks cheats. */
#ifdef DEBUG
  inline constexpr bool verbose = true;
#else
  inline constexpr bool verbose = false;
#endif

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

/** Set once the player asks for the game to close: the window's close
    button on desktop, or the reset or power button on Wii. Every game loop
    checks it so the request unwinds all the way out of main() instead of
    being handled locally by whichever loop happened to see the event.
    Volatile because the Wii power button sets it from an interrupt. */
extern volatile bool quit_requested;

int st_poll_event(SDL_Event *event);

#endif /* SUPERTUX_GLOBALS_H */

// EOF
