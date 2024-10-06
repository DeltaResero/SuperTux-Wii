//  setup.h
//
//  SuperTux
//  Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
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

#ifndef SUPERTUX_SETUP_H
#define SUPERTUX_SETUP_H

#include "menu.h"
#include "sound.h"
#include "type.h"

bool faccessible(const char *filename); // Checks if the file is accessible
bool fcreatedir(const char* relative_dir); // Creates a directory relative to the current working directory
bool fwriteable(const char *filename); // Checks if the file is writable
FILE * opendata(const char * filename, const char * mode); // Opens a data file

string_list_type dsubdirs(const char *rel_path, const char* expected_file); // Retrieves a list of subdirectories with the expected file
string_list_type dfiles(const char *rel_path, const char* glob, const char* exception_str); // Retrieves a list of files matching a pattern
void free_strings(char **strings, int num); // Frees an array of strings

void st_directory_setup(void); // Sets up the directory structure
void st_general_setup(void); // General setup function, initializes subsystems
void st_general_free(); // Frees resources from general setup
void st_video_setup_sdl(void); // Sets up SDL video
void st_video_setup_gl(void); // Sets up OpenGL video
void st_video_setup(void); // Chooses and sets up the appropriate video system
void st_audio_setup(void); // Sets up the audio system
void st_joystick_setup(void); // Sets up the joystick system
void st_shutdown(void); // Shuts down the game and frees resources
void st_menu(void); // Displays the main menu
void st_abort(const std::string& reason, const std::string& details); // Aborts the game with a reason and details

void process_options_menu(void); // Processes the options menu
bool process_load_game_menu(); // Returns true if the game loop was entered
void update_load_save_game_menu(Menu* pmenu); // Updates the load/save game menu

void load_config_file(void);  // Load config file

#ifndef _WII_
void parseargs(int argc, char * argv[]); // Parses command-line arguments
#endif

void print_status(const char *st);

#endif /* SUPERTUX_SETUP_H */

// EOF
