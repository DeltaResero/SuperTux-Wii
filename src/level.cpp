// src/level.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2004 Ricardo Cruz <rick2@aeiou.pt>
// Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
// Copyright (C) 2004 Duong-Khang NGUYEN <neoneurone@users.sourceforge.net>
// Copyright (C) 2004 Matthias Braun <matze@braunis.de>
// Copyright (C) 2004 Ryan Flegel <xxdigitalhellxx@hotmail.com>
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <map>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string_view>
#include "globals.hpp"
#include "defines.hpp"
#include "setup.hpp"
#include "screen.hpp"
#include "level.hpp"
#include "physic.hpp"
#include "scene.hpp"
#include "tile.hpp"
#include "lispreader.hpp"
#include "resources.hpp"
#include "music_manager.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;  // Alias for ease of use
using namespace std;

/**
 * Constructs a LevelSubset object.
 * Initializes the image and levels to null and zero respectively.
 */
LevelSubset::LevelSubset()
  : image(nullptr), levels(0)
{
}

/**
 * Destructor for LevelSubset. Deletes the image if it was allocated.
 */
LevelSubset::~LevelSubset()
{
  delete image;
}

/**
 * Parses a Lisp object to extract LevelSubset information.
 * @param data Pointer to the Lisp object to parse.
 * Iterates through the object to extract the title and description.
 */
void LevelSubset::parse(lisp_object_t* data)
{
  LispReader reader(data);
  reader.read_string("title", &title);
  reader.read_string("description", &description);
}

/**
 * Loads a LevelSubset from disk.
 * @param subset The name of the subset to load.
 * Searches for and loads the level subset's info file, then parses the content.
 */
void LevelSubset::load(std::string_view subset)
{
  name = subset;

  // Construct the filename path using std::filesystem
  fs::path filename = fs::path(st_dir) / "levels" / subset / "info";

  if (!faccessible(filename.string().c_str()))
  {
    filename = fs::path(datadir) / "levels" / subset / "info";
  }

  // If the filename length exceeds the limit, log an error and return
  if (filename.string().length() >= 1020)
  {
    if (verbose)
    {
      fprintf(stderr, "Filename is too long: %s\n", filename.string().c_str());
    }
    return;
  }

  if (faccessible(filename.string().c_str()))
  {
    FILE* fi = fopen(filename.string().c_str(), "r");
    if (fi == nullptr)
    {
      if (verbose)
      {
        perror(filename.string().c_str());  // System-generated error message
      }
      return;
    }

    lisp_stream_t stream;
    lisp_stream_init_file(&stream, fi);
    lisp_object_t* root_obj = lisp_read(&stream);

    if (root_obj->type == LISP_TYPE_EOF || root_obj->type == LISP_TYPE_PARSE_ERROR)
    {
      if (verbose)
      {
        printf("World: Parse Error in file %s\n", filename.string().c_str());
      }
    }
    else
    {
      lisp_object_t* cur = lisp_car(root_obj);
      if (!lisp_symbol_p(cur))
      {
        if (verbose)
        {
          printf("World: Read error in %s\n", filename.string().c_str());
        }
      }
      else if (strcmp(lisp_symbol(cur), "supertux-level-subset") == 0)
      {
        parse(lisp_cdr(root_obj));
      }
    }

    fclose(fi);

    fs::path image_file = filename.parent_path() / (filename.stem().string() + ".png");
    if (faccessible(image_file.string().c_str()))
    {
      delete image;
      image = new Surface(image_file.string().c_str(), false);
    }
    else
    {
      fs::path default_image = fs::path(datadir) / "images/status/level-subset-info.png";
      delete image;
      image = new Surface(default_image.string().c_str(), false);
    }
  }

  // Cache the base paths to avoid recreating them in the loop
  fs::path level_dir_st = fs::path(st_dir) / "levels" / subset;
  fs::path level_dir_data = fs::path(datadir) / "levels" / subset;

  int i;  // Declare `i` outside the loop for later use
  for (i = 1; ; ++i)
  {
    // Construct only the changing part of the filename
    std::string level_name = "level" + to_string(i) + ".stl";

    // Get the number of levels in this subset
    fs::path level_filename = level_dir_st / level_name;
    if (!faccessible(level_filename.string().c_str()))
    {
      level_filename = level_dir_data / level_name;
      if (!faccessible(level_filename.string().c_str()))
      {
        break; // No more levels found
      }
    }
  }
  levels = i - 1;
}

/**
 * Constructs a Level object.
 * Initializes the level by setting default values.
 */
Level::Level()
  : img_bkgd(nullptr)
{
  init_defaults();
}

/**
 * Constructs a Level object by loading a specific level.
 * @param subset The subset name containing the level.
 * @param level The level number to load.
 */
Level::Level(std::string_view subset, int level)
  : img_bkgd(nullptr)
{
  if (load(subset, level) < 0)
  {
    st_abort("Couldn't load level from subset", std::string(subset).c_str());
  }
}

/**
 * Constructs a Level object by loading a specific level file.
 * @param filename The filename of the level to load.
 */
Level::Level(std::string_view filename)
  : img_bkgd(nullptr)
{
  if (load(filename) < 0)
  {
    st_abort("Couldn't load level ", std::string(filename).c_str());
  }
}

/**
 * Destructor for Level.
 * Cleans up allocated resources by deleting the background image.
 */
Level::~Level()
{
  delete img_bkgd;
}

/**
 * Initializes level attributes with default values.
 * Sets default values for level properties like name, author, gravity, and tiles.
 */
void Level::init_defaults()
{
  cleanup();

  name = "UnNamed";
  author = "UnNamed";
  song_title = "Mortimers_chipdisko.mod";
  bkgd_image = "arctis2.jpg";
  width = MIN_LEVEL_WIDTH;
  start_pos_x = 100;
  start_pos_y = 170;
  time_left = 100;
  gravity = 10.;
  back_scrolling = false;
  hor_autoscroll_speed = 0;
  bkgd_speed = 50;
  bkgd_top.red = 0;
  bkgd_top.green = 0;
  bkgd_top.blue = 0;
  bkgd_bottom.red = 255;
  bkgd_bottom.green = 255;
  bkgd_bottom.blue = 255;

  const int total_tiles = width * SCREEN_HEIGHT_TILES;
  ia_tiles.assign(total_tiles, 0);
  bg_tiles.assign(total_tiles, 0);
  fg_tiles.assign(total_tiles, 0);
}

/**
 * Loads a level from a specific subset.
 * @param subset The subset name containing the level.
 * @param level The level number to load.
 * @return Returns 0 on success, or -1 on failure.
 */
int Level::load(std::string_view subset, int level)
{
  // Construct the filename path using std::filesystem
  fs::path filename = fs::path(st_dir) / "levels" / subset / ("level" + to_string(level) + ".stl");
  if (!faccessible(filename.string().c_str()))
  {
    filename = fs::path(datadir) / "levels" / subset / ("level" + to_string(level) + ".stl");
  }

  return load(filename.string());
}

/**
 * Loads a level from a file.
 * @param filename The filename of the level to load.
 * @return Returns 0 on success, or -1 on failure.
 */
int Level::load(std::string_view filename)
{
  init_defaults();

  lisp_object_t* root_obj = lisp_read_from_file(filename);
  if (!root_obj)
  {
    if (verbose)
    {
      std::cout << "Level: Couldn't load file: " << filename << std::endl;
    }
    return -1;
  }

  if (root_obj->type == LISP_TYPE_EOF || root_obj->type == LISP_TYPE_PARSE_ERROR)
  {
    if (verbose)
    {
      printf("World: Parse Error in file %s", std::string(filename).c_str());
    }
    return -1;
  }

  if (strcmp(lisp_symbol(lisp_car(root_obj)), "supertux-level") == 0)
  {
    LispReader reader(lisp_cdr(root_obj));
    int version = 0;
    reader.read_int("version", &version);

    parseProperties(reader);
    parseTilemaps(reader, version);
    parseObjects(reader);
  }

  return 0;
}

/**
 * Parses the general properties of the level from the Lisp data.
 * @param reader The LispReader instance to use for parsing.
 */
void Level::parseProperties(LispReader& reader)
{
  if (!reader.read_int("width", &width))
  {
    st_abort("No width specified for level.", "");
  }

  if (!reader.read_int("start_pos_x", &start_pos_x))
  {
    start_pos_x = 100;
  }
  if (!reader.read_int("start_pos_y", &start_pos_y))
  {
    start_pos_y = 170;
  }

  time_left = 500;
  if (!reader.read_int("time", &time_left) && verbose)
  {
    printf("Warning no time specified for level.\n");
  }

  back_scrolling = false;
  reader.read_bool("back_scrolling", &back_scrolling);

  hor_autoscroll_speed = 0;
  reader.read_float("hor_autoscroll_speed", &hor_autoscroll_speed);

  bkgd_speed = 50;
  reader.read_int("bkgd_speed", &bkgd_speed);

  bkgd_top.red = bkgd_top.green = bkgd_top.blue = 0;
  reader.read_int("bkgd_red_top", &bkgd_top.red);
  reader.read_int("bkgd_green_top", &bkgd_top.green);
  reader.read_int("bkgd_blue_top", &bkgd_top.blue);

  bkgd_bottom.red = bkgd_bottom.green = bkgd_bottom.blue = 0;
  reader.read_int("bkgd_red_bottom", &bkgd_bottom.red);
  reader.read_int("bkgd_green_bottom", &bkgd_bottom.green);
  reader.read_int("bkgd_blue_bottom", &bkgd_bottom.blue);

  gravity = 10;
  reader.read_float("gravity", &gravity);
  name = "Noname";
  reader.read_string("name", &name);
  author = "unknown author";
  reader.read_string("author", &author);
  song_title = "";
  reader.read_string("music", &song_title);
  bkgd_image = "";
  reader.read_string("background", &bkgd_image);
  particle_system = "";
  reader.read_string("particle_system", &particle_system);
}

/**
 * Parses the tilemaps (background, interactive, foreground) from the Lisp data.
 * @param reader The LispReader instance to use for parsing.
 * @param version The version of the level file, used for compatibility.
 */
void Level::parseTilemaps(LispReader& reader, int version)
{
  vector<int> ia_tm;
  vector<int> bg_tm;
  vector<int> fg_tm;
  const int total_tiles = width * SCREEN_HEIGHT_TILES;

  // Reserve memory to prevent reallocations
  ia_tm.reserve(total_tiles);
  bg_tm.reserve(total_tiles);
  fg_tm.reserve(total_tiles);

  reader.read_int_vector("background-tm", &bg_tm);
  if (!reader.read_int_vector("interactive-tm", &ia_tm))
  {
    reader.read_int_vector("tilemap", &ia_tm);
  }
  reader.read_int_vector("foreground-tm", &fg_tm);

  // Convert old levels to the new tile numbers
  if (version == 0)
  {
    std::map<char, int> transtable;
    transtable['.'] = 0; transtable['x'] = 104; transtable['X'] = 77;
    transtable['y'] = 78; transtable['Y'] = 105; transtable['A'] = 83;
    transtable['B'] = 102; transtable['!'] = 103; transtable['a'] = 84;
    transtable['C'] = 85; transtable['D'] = 86; transtable['E'] = 87;
    transtable['F'] = 88; transtable['c'] = 89; transtable['d'] = 90;
    transtable['e'] = 91; transtable['f'] = 92; transtable['G'] = 93;
    transtable['H'] = 94; transtable['I'] = 95; transtable['J'] = 96;
    transtable['g'] = 97; transtable['h'] = 98; transtable['i'] = 99;
    transtable['j'] = 100; transtable['#'] = 11; transtable['['] = 13;
    transtable['='] = 14; transtable[']'] = 15; transtable['$'] = 82;
    transtable['^'] = 76; transtable['*'] = 80; transtable['|'] = 79;
    transtable['\\'] = 81; transtable['&'] = 75;

    int x = 0;
    int y = 0;
    for (auto& tile : ia_tm)
    {
      if (tile >= '0' && tile <= '2')
      {
        badguy_data.push_back(BadGuyData(static_cast<BadGuyKind>(tile - '0'), x * TILE_SIZE, y * TILE_SIZE, false));
        tile = 0;
      }
      else
      {
        auto it = transtable.find(tile);
        tile = (it != transtable.end()) ? it->second : 0;
      }
      if (++x >= width)
      {
        x = 0;
        ++y;
      }
    }
  }

  // Set the final size for our member vectors
  ia_tiles.resize(total_tiles);
  bg_tiles.resize(total_tiles);
  fg_tiles.resize(total_tiles);

  // Place interactive tiles
  for (int j = 0; j < SCREEN_HEIGHT_TILES; ++j)
  {
    for (int i = 0; i < width; ++i)
    {
      int index = j * width + i;
      if (static_cast<size_t>(index) >= ia_tm.size())
      {
        break;
      }
      unsigned int tile = ia_tm[index];
      ia_tiles[index] = tile;
      switch (tile)
      {
        case 102: case 103: case 104: case 105: case 128: case 77:
        case 78:  case 26:  case 82:  case 83:  case 44:  case 45: case 46:
          original_tiles.push_back({i, j, tile});
          break;
      }
    }
  }

  // Place background and foreground tiles
  for (int j = 0; j < SCREEN_HEIGHT_TILES; ++j)
  {
    for (int i = 0; i < width; ++i)
    {
      int index = j * width + i;
      if (static_cast<size_t>(index) < bg_tm.size())
      {
        bg_tiles[index] = bg_tm[index];
      }
      if (static_cast<size_t>(index) < fg_tm.size())
      {
        fg_tiles[index] = fg_tm[index];
      }
    }
  }
}

/**
 * Parses game objects like reset points and bad guys from the Lisp data.
 * @param reader The LispReader instance to use for parsing.
 */
void Level::parseObjects(LispReader& reader)
{
  // Read ResetPoints
  lisp_object_t* reset_points_list = nullptr;
  if (reader.read_lisp("reset-points", &reset_points_list))
  {
    while (!lisp_nil_p(reset_points_list))
    {
      lisp_object_t* data = lisp_car(reset_points_list);
      lisp_object_t* x_val = lisp_find_value(lisp_cdr(data), "x");
      lisp_object_t* y_val = lisp_find_value(lisp_cdr(data), "y");

      if (x_val && y_val && lisp_integer_p(lisp_car(x_val)) && lisp_integer_p(lisp_car(y_val)))
      {
          reset_points.push_back({lisp_integer(lisp_car(x_val)), lisp_integer(lisp_car(y_val))});
      }
      reset_points_list = lisp_cdr(reset_points_list);
    }
  }

  // Read BadGuys
  lisp_object_t* objects_list = nullptr;
  if (reader.read_lisp("objects", &objects_list))
  {
    while (!lisp_nil_p(objects_list))
    {
      lisp_object_t* data = lisp_car(objects_list);

      BadGuyData bg_data;
      bg_data.kind = badguykind_from_string(lisp_symbol(lisp_car(data)));

      lisp_object_t* val_x = lisp_find_value(lisp_cdr(data), "x");
      if (val_x)
      {
        bg_data.x = lisp_integer(lisp_car(val_x));
      }

      lisp_object_t* val_y = lisp_find_value(lisp_cdr(data), "y");
      if (val_y)
      {
        bg_data.y = lisp_integer(lisp_car(val_y));
      }

      lisp_object_t* val_stay = lisp_find_value(lisp_cdr(data), "stay-on-platform");
      if (val_stay)
      {
        bg_data.stay_on_platform = lisp_boolean(lisp_car(val_stay));
      }

      badguy_data.push_back(bg_data);
      objects_list = lisp_cdr(objects_list);
    }
  }
}


/**
 * Reloads bricks and coins in the level.
 * Resets interactive tiles to their original state.
 */
void Level::reload_bricks_and_coins()
{
  for (const auto& tile_info : original_tiles)
  {
    if (tile_info.y >= 0 && tile_info.y < SCREEN_HEIGHT_TILES && tile_info.x >= 0 && tile_info.x < width)
    {
      ia_tiles[tile_info.y * width + tile_info.x] = tile_info.tile;
    }
  }
}

/**
 * Cleans up the level, releasing resources.
 * Clears all tile vectors, reset points, and resets attributes to defaults.
 */
void Level::cleanup()
{
  bg_tiles.clear();
  ia_tiles.clear();
  fg_tiles.clear();

  original_tiles.clear();
  reset_points.clear();
  name = "";
  author = "";
  song_title = "";
  bkgd_image = "";
  badguy_data.clear();
}

/**
 * Loads level-specific graphics.
 * Loads background images based on the current level's background image attribute.
 */
void Level::load_gfx()
{
  // Always delete the existing background to prevent memory leaks.
  delete img_bkgd;
  img_bkgd = nullptr;

  if (!bkgd_image.empty())
  {
    fs::path fname = fs::path(st_dir) / "background" / bkgd_image;
    if (!faccessible(fname.string().c_str()))
    {
      fname = fs::path(datadir) / "images/background" / bkgd_image;
    }
    img_bkgd = new Surface(fname.string().c_str(), false);
  }
}

/**
 * Changes a specific tile in the level.
 * @param x The x-coordinate of the tile to change.
 * @param y The y-coordinate of the tile to change.
 * @param tm The tilemap (background, interactive, or foreground) to modify.
 * @param c The new tile value to set.
 */
void Level::change(float x, float y, int tm, unsigned int c)
{
  int yy = static_cast<int>(y) / TILE_SIZE;
  int xx = static_cast<int>(x) / TILE_SIZE;

  if (yy >= 0 && yy < SCREEN_HEIGHT_TILES && xx >= 0 && xx < width)
  {
    int index = yy * width + xx;
    switch (tm)
    {
      case TM_BG:
        bg_tiles[index] = c;
        break;
      case TM_IA:
        ia_tiles[index] = c;
        break;
      case TM_FG:
        fg_tiles[index] = c;
        break;
    }
  }
}

/**
 * Loads the level's background music.
 * Loads the standard and fast versions of the level's music based on the current level attributes.
 */
void Level::load_song()
{
  std::string song_path;
  std::string song_subtitle = song_title.substr(0, song_title.find_last_of('.'));

  level_song = music_manager->load_music(datadir + "/music/" + song_title);

  song_path = datadir + "/music/" + song_subtitle + "-fast" + song_title.substr(song_title.find_last_of('.'));
  if (!music_manager->exists_music(song_path))
  {
    level_song_fast = level_song;
  }
  else
  {
    level_song_fast = music_manager->load_music(song_path);
  }
}

/**
 * Gets the standard level music.
 * @return A reference to the standard level music.
 */
MusicRef Level::get_level_music() const
{
  return level_song;
}

/**
 * Gets the fast version of the level music.
 * @return A reference to the fast version of the level music.
 */
MusicRef Level::get_level_music_fast() const
{
  return level_song_fast;
}

/**
 * Gets the tile ID at a specific position.
 * @param x The x-coordinate to query.
 * @param y The y-coordinate to query.
 * @return The tile ID at the specified position.
 */
unsigned int Level::gettileid(float x, float y) const
{
  int xx = static_cast<int>(x) / TILE_SIZE;
  int yy = static_cast<int>(y) / TILE_SIZE;

  if (yy >= 0 && yy < SCREEN_HEIGHT_TILES && xx >= 0 && xx < width)
  {
    return ia_tiles[yy * width + xx];
  }

  return 0;
}

/**
 * Gets the tile at a specific grid position.
 * @param x The x-coordinate in the grid.
 * @param y The y-coordinate in the grid.
 * @return The tile ID at the specified grid position.
 */
unsigned int Level::get_tile_at(int x, int y) const
{
  if (x < 0 || x >= width || y < 0 || y >= SCREEN_HEIGHT_TILES)
  {
    return 0;
  }
  return ia_tiles[y * width + x];
}

/**
 * Draws the background of the level.
 * Chooses between drawing a background image with parallax scrolling
 * or a color gradient.
 */
void Level::draw_bg() const
{
  if (img_bkgd)
  {
    int s = (int)((float)scroll_x * ((float)bkgd_speed / 100.0f)) % screen->w;
    img_bkgd->draw_part(s, 0, 0, 0, img_bkgd->w - s, img_bkgd->h);
    img_bkgd->draw_part(0, 0, screen->w - s, 0, s, img_bkgd->h);
  }
  else
  {
    drawgradient(bkgd_top, bkgd_bottom);
  }
}

/**
 * A lightweight "peek" function that reads just enough of a level file
 * to extract its title, avoiding a full parse. This is much faster than
 * loading the entire level just to display its name in a menu.
 * @param level_filename The full path to the .stl file.
 * @return The title of the level.
 */
std::string Level::get_level_title_fast(std::string_view level_filename)
{
    // get_title_from_lisp_file now accepts string_view, so no conversion needed!
    return get_title_from_lisp_file(level_filename, "Invalid Level", "Untitled Level");
}

// EOF
