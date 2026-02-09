// src/supertux.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux - SuperTux
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <sys/types.h>
#include <ctype.h>

#include "defines.hpp"
#include "globals.hpp"
#include "setup.hpp"
#include "intro.hpp"
#include "title.hpp"
#include "gameloop.hpp"
#include "screen.hpp"
#include "worldmap.hpp"
#include "resources.hpp"
#include "texture.hpp"
#include "tile.hpp"
#ifdef __WII__
  #include <unistd.h>
  #include <wiiuse/wpad.h>
  #include <ogc/lwp_watchdog.h>
  #include <fat.h>
#endif

// Loading Screen as SuperTux on Wii takes a long, long time to load
Surface* loading_surf = NULL;


/**
 * Main function of SuperTux.
 * This function sets up the game environment, handles configuration, initializes audio, video,
 * and input systems, and either loads the game session or starts the title screen.
 * For Wii builds, it also handles FAT library initialization and system setup.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return int Program exit code.
 */
int main(int argc, char ** argv)
{

#ifdef __WII__
  // Wii-specific setup for FAT library and USB disk handling.
  sleep(1);  // Delay to allow USB disks behind hubs to initialize.
  bool res = fatInitDefault();
  if (res == 0)
  {
    st_abort("FAT Library Initialization Failed", "Unable to initialize FAT library for SD/USB access.");
  }

#endif

  // Setup directory paths and load configuration
  st_directory_setup();
  load_config_file();  // Load configuration file

#ifndef __WII__
  parseargs(argc, argv);  // Parse command-line arguments
#endif

  // Setup audio and video
  st_audio_setup();
  st_video_setup();
  SDL_ShowCursor(false);  // Hide SDL cursor (SuperTux has it's own cursor)

  // Initialize and show the loading screen
  clearscreen(0, 0, 0);
  loading_surf = new Surface(datadir + "/images/title/loading.png", true);
  loading_surf->draw(160, 30);
  updatescreen();  // Refresh screen to show the loading screen

  // Initialize input systems, game settings, and menus
  st_joystick_setup();
  st_general_setup();
  st_menu();
  loadshared();  // Load shared game resources (graphics, sounds, etc.)

  // Check if a level startup file is specified (start a game session), otherwise show the title screen
  if (!level_startup_file.empty())
  {
    GameSession session(level_startup_file, 1, ST_GL_LOAD_LEVEL_FILE);
    session.run();  // Run the specified game session
  }
  else
  {
    title();  // Start the title screen, loading_surf is deleted inside the title() function
  }

  // Clear the screen (but don't flip the buffer at shutdown)
  clearscreen(0, 0, 0);

  // Unload all high-level game resources BEFORE running the debug check.
  unloadshared();
  st_general_free();
  TileManager::destroy_instance();

#ifdef DEBUG
  Surface::debug_check();  // Now this check should report an empty list.
#endif

  // Perform the final, low-level system shutdown.
  st_shutdown();

  return 0;
}

// EOF
