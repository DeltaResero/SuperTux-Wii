//  supertux.cpp
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

#include <sys/types.h>
#include <ctype.h>

#include "defines.h"
#include "globals.h"
#include "setup.h"
#include "intro.h"
#include "title.h"
#include "gameloop.h"
#include "screen.h"
#include "worldmap.h"
#include "resources.h"
#include "texture.h"
#include "tile.h"
#ifdef _WII_
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

#ifdef _WII_
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

#ifndef _WII_
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
  if (level_startup_file)
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

  // Unload shared resources and clean up game state
  unloadshared();
  st_general_free();  // Free general game resources
  TileManager::destroy_instance();  // Destroy the singleton instance of TileManager

#ifdef DEBUG
  Surface::debug_check();  // Check for any debug issues in surfaces
#endif

  // Perform system shutdown and cleanup
  st_shutdown();

  return 0;
}

// EOF
