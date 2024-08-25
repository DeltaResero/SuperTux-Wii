//  $Id: configfile.cpp 2620 2005-06-18 12:12:10Z matzebraun $
//
//  SuperTux - A Jump'n Run
//  Copyright (C) 2004 Michael George <mike@georgetech.com>
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include <cstdlib>
#include <cstring>
#include "configfile.h"
#include "setup.h"
#include "globals.h"
#include "lispreader.h"
#include "player.h"

#ifdef WIN32
const char* config_filename = "/st_config.dat";
#else
const char* config_filename = "/config";
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
  use_gl = false;
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
    lisp_free(root_obj);
    fclose(file);
    return;
  }

  if (strcmp(lisp_symbol(lisp_car(root_obj)), "supertux-config") != 0)
  {
    lisp_free(root_obj);
    fclose(file);
    return;
  }

  // Parse the config values
  LispReader reader(lisp_cdr(root_obj));

  reader.read_bool("fullscreen", &use_fullscreen);
  reader.read_bool("sound", &use_sound);
  reader.read_bool("music", &use_music);
  reader.read_bool("show_fps", &show_fps);

  std::string video;
  reader.read_string("video", &video);
  use_gl = (video == "opengl");

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
  lisp_free(root_obj);
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

/* EOF */
