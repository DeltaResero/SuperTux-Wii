//  $Id: setup.h 803 2004-04-28 13:18:54Z grumbel $
//
//  SuperTux -  A Jump'n Run
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

/**
 * Checks if the given file is accessible.
 * @param filename The path to the file to check.
 * @return int Returns 1 if accessible, 0 otherwise.
 */
int faccessible(const char *filename);

/**
 * Creates a directory relative to the current working directory.
 * @param relative_dir The relative path of the directory to create.
 * @return int Returns 0 on success, -1 on failure.
 */
int fcreatedir(const char* relative_dir);

/**
 * Checks if the given file is writable.
 * @param filename The path to the file to check.
 * @return int Returns 1 if writable, 0 otherwise.
 */
int fwriteable(const char *filename);

/**
 * Opens a data file with the specified mode.
 * @param filename The path to the file to open.
 * @param mode The mode to open the file in.
 * @return FILE* A file pointer on success, or NULL on failure.
 */
FILE * opendata(const char * filename, const char * mode);

/**
 * Retrieves a list of subdirectories within the specified path that contain an expected file.
 * @param rel_path The relative path to search within.
 * @param expected_file The name of the expected file in the subdirectories.
 * @return string_list_type A list of subdirectory names.
 */
string_list_type dsubdirs(const char *rel_path, const char* expected_file);

/**
 * Retrieves a list of files matching a pattern in the specified path.
 * @param rel_path The relative path to search within.
 * @param glob The pattern to match files against.
 * @param exception_str A string describing any exceptions to log.
 * @return string_list_type A list of file names.
 */
string_list_type dfiles(const char *rel_path, const char* glob, const char* exception_str);

/**
 * Frees an array of strings.
 * @param strings The array of strings to free.
 * @param num The number of strings in the array.
 */
void free_strings(char **strings, int num);

/**
 * Sets up the directory structure needed for the game.
 */
void st_directory_setup(void);

/**
 * General setup function for the game, including initializing subsystems.
 */
void st_general_setup(void);

/**
 * Frees resources allocated during general setup.
 */
void st_general_free();

/**
 * Sets up the SDL video subsystem.
 */
void st_video_setup_sdl(void);

/**
 * Sets up the OpenGL video subsystem.
 */
void st_video_setup_gl(void);

/**
 * Determines and sets up the appropriate video subsystem (SDL or OpenGL).
 */
void st_video_setup(void);

/**
 * Sets up the audio subsystem.
 */
void st_audio_setup(void);

/**
 * Sets up the joystick subsystem.
 */
void st_joystick_setup(void);

/**
 * Shuts down the game, freeing all resources.
 */
void st_shutdown(void);

/**
 * Displays the main menu of the game.
 */
void st_menu(void);

/**
 * Aborts the game with a given reason and detailed message.
 * @param reason A short reason for the abort.
 * @param details Additional details about the abort.
 */
void st_abort(const std::string& reason, const std::string& details);

/**
 * Processes the options menu, handling user input and updating settings.
 */
void process_options_menu(void);

/**
 * Returns true if the game loop was entered, false otherwise.
 * @return bool True if the game loop was entered, false otherwise.
 */
bool process_load_game_menu();

/**
 * Updates the load/save game menu with the latest available options.
 * @param pmenu Pointer to the menu to update.
 */
void update_load_save_game_menu(Menu* pmenu);

#ifndef _WII_
/**
 * Parses command-line arguments passed to the game.
 * @param argc The number of arguments.
 * @param argv The array of argument strings.
 */
void parseargs(int argc, char * argv[]);
#endif

#endif /* SUPERTUX_SETUP_H */
// EOF
