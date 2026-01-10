// src/configfile.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2004 Michael George <mike@georgetech.com>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <cstdlib>
#include <cstring>
#include "configfile.hpp"
#include "setup.hpp"
#include "globals.hpp"
#include "lispreader.hpp"
#include "player.hpp"

#ifdef WIN32
const char* config_filename = "st_config.dat";
#else
const char* config_filename = "config";
#endif

/**
 * Sets default configuration values.
 * Initializes the game settings to their default values before loading from the config file.
 */
static void defaults()
{
  debug_mode = false;
  audio_device = true;
  use_fullscreen = true;
  show_fps = false;
#ifdef NOOPENGL
  use_gl = false;
#else
  // Default to OpenGL if available as it's typically faster on most platforms
  use_gl = true;
#endif
  use_sound = true;
  use_music = true;
}

/**
 * Loads the game configuration from a file.
 * This function overrides the default settings with values from the configuration file.
 */
void loadconfig()
{
  FILE* file = nullptr;

  // Set default values
  defaults();

  // Try to open the config file
  file = opendata(config_filename, "r");

  if (file == nullptr)
    return;

  // Read the config file using Lisp-like syntax
  lisp_stream_t stream;
  lisp_object_t* root_obj = nullptr;

  lisp_stream_init_file(&stream, file);
  root_obj = lisp_read(&stream);

  if (root_obj->type == LISP_TYPE_EOF || root_obj->type == LISP_TYPE_PARSE_ERROR)
  {
    fclose(file);
    return;
  }

  if (strcmp(lisp_symbol(lisp_car(root_obj)), "supertux-config") != 0)
  {
    fclose(file);
    return;
  }

  // Parse the config values
  LispReader reader(lisp_cdr(root_obj));

#ifdef _WII_
  // On Wii Homebrew, fullscreen is mandatory, so we enforce it here.
  use_fullscreen = true;
  // We read the config value to advance the parser but ignore its value.
  bool dummy_fullscreen_setting;
  reader.read_bool("fullscreen", &dummy_fullscreen_setting);
#else
  // For other platforms, respect the user's fullscreen setting.
  reader.read_bool("fullscreen", &use_fullscreen);
#endif

  reader.read_bool("sound", &use_sound);
  reader.read_bool("music", &use_music);
  reader.read_bool("show_fps", &show_fps);
  reader.read_bool("tv_overscan", &tv_overscan_enabled);

#ifdef NOOPENGL
  // When OpenGL is disabled at compile time, always force SDL mode.
  use_gl = false;
  // We read the "video" setting to advance the parser but ignore its value.
  std::string dummy_video_setting;
  reader.read_string("video", &dummy_video_setting);
#else
  // When OpenGL is available, read the user's preference from the config.
  // Only update use_gl if the tag is actually present in the file.
  // This ensures that if the user deletes their config (or has an old one),
  // we default to OpenGL (set in defaults()) rather than falling back to SDL.
  std::string video;
  if (reader.read_string("video", &video))
  {
    use_gl = (video == "opengl");
  }
#endif

  reader.read_int("joystick", &joystick_num);
  use_joystick = (joystick_num >= 0);

  reader.read_int("joystick-x", &joystick_keymap.x_axis);
  reader.read_int("joystick-y", &joystick_keymap.y_axis);
  reader.read_int("joystick-a", &joystick_keymap.a_button);
  reader.read_int("joystick-b", &joystick_keymap.b_button);
  reader.read_int("joystick-start", &joystick_keymap.start_button);
  reader.read_int("joystick-deadzone", &joystick_keymap.dead_zone);

  reader.read_int("keyboard-jump", &keymap.jump);
  reader.read_int("keyboard-duck", &keymap.duck);
  reader.read_int("keyboard-left", &keymap.left);
  reader.read_int("keyboard-right", &keymap.right);
  reader.read_int("keyboard-fire", &keymap.fire);

  // Free the Lisp object and close the file
  fclose(file);
}

/**
 * Saves the current game configuration to a file.
 * This function writes the current settings to the config file in a Lisp-like format.
 */
void saveconfig()
{
  // Open the config file for writing
  FILE* config = opendata(config_filename, "w");

  if (config)
  {
    fprintf(config, "(supertux-config\n");
    fprintf(config, "\t;; the following options can be set to #t or #f:\n");
    fprintf(config, "\t(fullscreen %s)\n", use_fullscreen ? "#t" : "#f");
    fprintf(config, "\t(sound %s)\n", use_sound ? "#t" : "#f");
    fprintf(config, "\t(music %s)\n", use_music ? "#t" : "#f");
    fprintf(config, "\t(show_fps %s)\n", show_fps ? "#t" : "#f");
    fprintf(config, "\t(tv_overscan %s)\n", tv_overscan_enabled ? "#t" : "#f");

    fprintf(config, "\n\t;; either \"opengl\" or \"sdl\"\n");
    fprintf(config, "\t(video \"%s\")\n", use_gl ? "opengl" : "sdl");

    fprintf(config, "\n\t;; joystick number (-1 means no joystick):\n");
    fprintf(config, "\t(joystick %d)\n", use_joystick ? joystick_num : -1);

    fprintf(config, "\t(joystick-x %d)\n", joystick_keymap.x_axis);
    fprintf(config, "\t(joystick-y %d)\n", joystick_keymap.y_axis);
    fprintf(config, "\t(joystick-a %d)\n", joystick_keymap.a_button);
    fprintf(config, "\t(joystick-b %d)\n", joystick_keymap.b_button);
    fprintf(config, "\t(joystick-start %d)\n", joystick_keymap.start_button);
    fprintf(config, "\t(joystick-deadzone %d)\n", joystick_keymap.dead_zone);

    fprintf(config, "\t(keyboard-jump %d)\n", keymap.jump);
    fprintf(config, "\t(keyboard-duck %d)\n", keymap.duck);
    fprintf(config, "\t(keyboard-left %d)\n", keymap.left);
    fprintf(config, "\t(keyboard-right %d)\n", keymap.right);
    fprintf(config, "\t(keyboard-fire %d)\n", keymap.fire);

    fprintf(config, ")\n");

    fclose(config);
  }
}

// EOF
