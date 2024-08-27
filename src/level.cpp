//  $Id: level.cpp 1634 2004-07-27 16:43:23Z rmcruz $
//
//  SuperTux
//  Copyright (C) 2004 SuperTux Development Team, see AUTHORS for details
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

#include <map>
#include <iostream>
#include <filesystem>
#include "globals.h"
#include "setup.h"
#include "screen.h"
#include "level.h"
#include "physic.h"
#include "scene.h"
#include "tile.h"
#include "lispreader.h"
#include "resources.h"
#include "music_manager.h"

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
 * Creates and saves a new level subset.
 * @param subset_name The name of the subset to create.
 * Initializes and saves a new LevelSubset with the provided name.
 */
void LevelSubset::create(const std::string& subset_name)
{
  Level new_lev;
  LevelSubset new_subset;
  new_subset.name = subset_name;
  new_subset.title = "Unknown Title";
  new_subset.description = "No description so far.";
  new_subset.save();
  new_lev.init_defaults();
  new_lev.save(subset_name, 1);
}

/**
 * Parses a Lisp object to extract LevelSubset information.
 * @param cursor Pointer to the Lisp object to parse.
 * Iterates through the object to extract the title and description.
 */
void LevelSubset::parse(lisp_object_t* cursor)
{
  while (!lisp_nil_p(cursor))
  {
    lisp_object_t* cur = lisp_car(cursor);
    char* s;

    if (!lisp_cons_p(cur) || !lisp_symbol_p(lisp_car(cur)))
    {
      printf("Not good");
    }
    else
    {
      if (strcmp(lisp_symbol(lisp_car(cur)), "title") == 0)
      {
        if ((s = lisp_string(lisp_car(lisp_cdr(cur)))) != nullptr)
        {
          title = s;
        }
      }
      else if (strcmp(lisp_symbol(lisp_car(cur)), "description") == 0)
      {
        if ((s = lisp_string(lisp_car(lisp_cdr(cur)))) != nullptr)
        {
          description = s;
        }
      }
    }
    cursor = lisp_cdr(cursor);
  }
}

/**
 * Loads a LevelSubset from disk.
 * @param subset Pointer to the name of the subset to load.
 * Searches for and loads the level subset's info file, then parses the content.
 */
void LevelSubset::load(char* subset)
{
  name = subset;

  // Construct the filename path using std::filesystem for better safety and clarity
  fs::path filename = fs::path(st_dir) / "levels" / subset / "info";
  if (!faccessible(filename.string().c_str()))
  {
    filename = fs::path(datadir) / "levels" / subset / "info";
  }

  // If the filename length exceeds the limit, log an error and return
  if (filename.string().length() >= 1020)
  {
    fprintf(stderr, "Filename is too long: %s\n", filename.string().c_str());
    return;
  }

  if (faccessible(filename.string().c_str()))
  {
    FILE* fi = fopen(filename.string().c_str(), "r");
    if (fi == nullptr)
    {
      perror(filename.string().c_str());  // System-generated error message
      return;
    }

    lisp_stream_t stream;
    lisp_stream_init_file(&stream, fi);
    lisp_object_t* root_obj = lisp_read(&stream);

    if (root_obj->type == LISP_TYPE_EOF || root_obj->type == LISP_TYPE_PARSE_ERROR)
    {
      printf("World: Parse Error in file %s\n", filename.string().c_str());
    }

    lisp_object_t* cur = lisp_car(root_obj);

    if (!lisp_symbol_p(cur))
    {
      printf("World: Read error in %s\n", filename.string().c_str());
    }

    if (strcmp(lisp_symbol(cur), "supertux-level-subset") == 0)
    {
      parse(lisp_cdr(root_obj));
    }

    lisp_free(root_obj);
    fclose(fi);

    fs::path image_file = filename.string() + ".png";
    if (faccessible(image_file.string().c_str()))
    {
      delete image;
      image = new Surface(image_file.string().c_str(), IGNORE_ALPHA);
    }
    else
    {
      filename = fs::path(datadir) / "images/status/level-subset-info.png";
      delete image;
      image = new Surface(filename.string().c_str(), IGNORE_ALPHA);
    }
  }

  int i;  // Declare `i` outside the loop for later use

  for (i = 1; i != -1; ++i)
  {
    // Get the number of levels in this subset
    filename = fs::path(st_dir) / "levels" / subset / ("level" + to_string(i) + ".stl");
    if (!faccessible(filename.string().c_str()))
    {
      filename = fs::path(datadir) / "levels" / subset / ("level" + to_string(i) + ".stl");
      if (!faccessible(filename.string().c_str()))
      {
        break;
      }
    }
  }
  levels = --i;
}

/**
 * Saves the LevelSubset data to disk.
 * Saves information such as title, description, and levels to a file.
 */
void LevelSubset::save()
{
  // Construct the filename path using std::filesystem
  fs::path filename = fs::path("/levels") / name;

  fcreatedir(filename.c_str());
  filename = fs::path(st_dir) / "levels" / name / "info";
  if (!fwriteable(filename.string().c_str()))
  {
    filename = fs::path(datadir) / "levels" / name / "info";
  }

  if (fwriteable(filename.string().c_str()))
  {
    FILE* fi = fopen(filename.string().c_str(), "w");
    if (fi == nullptr)
    {
      perror(filename.string().c_str());
    }

    // Write header:
    fprintf(fi, ";SuperTux-Level-Subset\n");
    fprintf(fi, "(supertux-level-subset\n");

    // Save title info:
    fprintf(fi, "  (title \"%s\")\n", title.c_str());

    // Save the description:
    fprintf(fi, "  (description \"%s\")\n", description.c_str());

    fprintf(fi, ")");
    fclose(fi);
  }
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
Level::Level(const std::string& subset, int level)
  : img_bkgd(nullptr)
{
  if (load(subset, level) < 0)
  {
    st_abort("Couldn't load level from subset", subset.c_str());
  }
}

/**
 * Constructs a Level object by loading a specific level file.
 * @param filename The filename of the level to load.
 */
Level::Level(const std::string& filename)
  : img_bkgd(nullptr)
{
  if (load(filename) < 0)
  {
    st_abort("Couldn't load level ", filename.c_str());
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
  bkgd_image = "arctis.png";
  width = 21;
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

  for (int i = 0; i < 15; ++i)
  {
    ia_tiles[i].resize(width + 1, 0);
    ia_tiles[i][width] = '\0';

    for (int y = 0; y < width; ++y)
    {
      ia_tiles[i][y] = 0;
    }

    bg_tiles[i].resize(width + 1, 0);
    bg_tiles[i][width] = '\0';
    for (int y = 0; y < width; ++y)
    {
      bg_tiles[i][y] = 0;
    }

    fg_tiles[i].resize(width + 1, 0);
    fg_tiles[i][width] = '\0';
    for (int y = 0; y < width; ++y)
    {
      fg_tiles[i][y] = 0;
    }
  }
}

/**
 * Loads a level from a specific subset.
 * @param subset The subset name containing the level.
 * @param level The level number to load.
 * @return Returns 0 on success, or -1 on failure.
 */
int Level::load(const std::string& subset, int level)
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
int Level::load(const std::string& filename)
{
  init_defaults();

  lisp_object_t* root_obj = lisp_read_from_file(filename.c_str());
  if (!root_obj)
  {
    std::cout << "Level: Couldn't load file: " << filename << std::endl;
    return -1;
  }

  if (root_obj->type == LISP_TYPE_EOF || root_obj->type == LISP_TYPE_PARSE_ERROR)
  {
    printf("World: Parse Error in file %s", filename.c_str());
    return -1;
  }

  vector<int> ia_tm;
  vector<int> bg_tm;
  vector<int> fg_tm;

  int version = 0;
  if (strcmp(lisp_symbol(lisp_car(root_obj)), "supertux-level") == 0)
  {
    LispReader reader(lisp_cdr(root_obj));
    version = 0;
    reader.read_int("version", &version);
    if (!reader.read_int("width", &width))
    {
      st_abort("No width specified for level.", "");
    }
    if (!reader.read_int("start_pos_x", &start_pos_x)) start_pos_x = 100;
    if (!reader.read_int("start_pos_y", &start_pos_y)) start_pos_y = 170;
    time_left = 500;
    if (!reader.read_int("time", &time_left))
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

    reader.read_int_vector("background-tm", &bg_tm);

    if (!reader.read_int_vector("interactive-tm", &ia_tm))
    {
      reader.read_int_vector("tilemap", &ia_tm);
    }

    reader.read_int_vector("foreground-tm", &fg_tm);

    // Read ResetPoints
    lisp_object_t* cur = nullptr;
    if (reader.read_lisp("reset-points", &cur))
    {
      while (!lisp_nil_p(cur))
      {
        lisp_object_t* data = lisp_car(cur);

        ResetPoint pos;

        LispReader reader(lisp_cdr(data));
        if (reader.read_int("x", &pos.x) && reader.read_int("y", &pos.y))
        {
          reset_points.push_back(pos);
        }

        cur = lisp_cdr(cur);
      }
    }

    // Read BadGuys
    cur = nullptr;
    if (reader.read_lisp("objects", &cur))
    {
      while (!lisp_nil_p(cur))
      {
        lisp_object_t* data = lisp_car(cur);

        BadGuyData bg_data;
        bg_data.kind = badguykind_from_string(lisp_symbol(lisp_car(data)));
        LispReader reader(lisp_cdr(data));
        reader.read_int("x", &bg_data.x);
        reader.read_int("y", &bg_data.y);
        reader.read_bool("stay-on-platform", &bg_data.stay_on_platform);

        badguy_data.push_back(bg_data);

        cur = lisp_cdr(cur);
      }
    }

    // Convert old levels to the new tile numbers
    if (version == 0)
    {
      std::map<char, int> transtable;
      transtable['.'] = 0;
      transtable['x'] = 104;
      transtable['X'] = 77;
      transtable['y'] = 78;
      transtable['Y'] = 105;
      transtable['A'] = 83;
      transtable['B'] = 102;
      transtable['!'] = 103;
      transtable['a'] = 84;
      transtable['C'] = 85;
      transtable['D'] = 86;
      transtable['E'] = 87;
      transtable['F'] = 88;
      transtable['c'] = 89;
      transtable['d'] = 90;
      transtable['e'] = 91;
      transtable['f'] = 92;

      transtable['G'] = 93;
      transtable['H'] = 94;
      transtable['I'] = 95;
      transtable['J'] = 96;

      transtable['g'] = 97;
      transtable['h'] = 98;
      transtable['i'] = 99;
      transtable['j'] = 100;
      transtable['#'] = 11;
      transtable['['] = 13;
      transtable['='] = 14;
      transtable[']'] = 15;
      transtable['$'] = 82;
      transtable['^'] = 76;
      transtable['*'] = 80;
      transtable['|'] = 79;
      transtable['\\'] = 81;
      transtable['&'] = 75;

      int x = 0;
      int y = 0;
      for (auto& tile : ia_tm)
      {
        if (tile == '0' || tile == '1' || tile == '2')
        {
          badguy_data.push_back(BadGuyData(static_cast<BadGuyKind>(tile - '0'), x * 32, y * 32, false));
          tile = 0;
        }
        else
        {
          auto it = transtable.find(tile);
          if (it != transtable.end())
          {
            tile = it->second;
          }
          else
          {
            printf("Error: conversion will fail, unsupported char: '%c' (%d)\n", tile, tile);
          }
        }
        ++x;
        if (x >= width)
        {
          x = 0;
          ++y;
        }
      }
    }
  }

  for (int i = 0; i < 15; ++i)
  {
    ia_tiles[i].resize(width + 1, 0);
    bg_tiles[i].resize(width + 1, 0);
    fg_tiles[i].resize(width + 1, 0);
  }

  // Place interactive tiles
  int i = 0;
  int j = 0;
  for (auto& tile : ia_tm)
  {
    if (j >= 15)
    {
      std::cerr << "Warning: Level higher than 15 interactive tiles."
                   "Ignoring by cutting tiles.\n"
                   "The level might not be finishable anymore!\n";
      break;
    }

    ia_tiles[j][i] = tile;

    switch (tile)
    {
      case 102:
      case 103:
      case 104:
      case 105:
      case 128:
      case 77:
      case 78:
      case 26:
      case 82:
      case 83:
      case 44:
      case 45:
      case 46:
      {
        // add this tile as a restorable tile
        OriginalTileInfo info;
        info.x = i;
        info.y = j;
        info.tile = tile;
        original_tiles.push_back(info);
      }
      break;
    }

    ++i;
    if (i >= width)
    {
      i = 0; // Reset i after reaching the end of the width
      ++j;  // Move to the next row
    }
  }

  // Place background tiles
  i = j = 0;
  for (auto& tile : bg_tm)
  {
    if (j >= 15)
    {
      std::cerr << "Warning: Level higher than 15 background tiles."
                   "Ignoring by cutting tiles.\n";
      break;
    }

    bg_tiles[j][i] = tile;
    ++i;
    if (i >= width)
    {
      i = 0; // Reset i after reaching the end of the width
      ++j;  // Move to the next row
    }
  }

  // Place foreground tiles
  i = j = 0;
  for (auto& tile : fg_tm)
  {
    if (j >= 15)
    {
      std::cerr << "Warning: Level higher than 15 foreground tiles."
                   "Ignoring by cutting tiles.\n";
      break;
    }

    fg_tiles[j][i] = tile;
    ++i;
    if (i >= width)
    {
      i = 0; // Reset i after reaching the end of the width
      ++j;  // Move to the next row
    }
  }

  lisp_free(root_obj);
  return 0;
}

/**
 * Reloads bricks and coins in the level.
 * Resets interactive tiles to their original state.
 */
void Level::reload_bricks_and_coins()
{
  for (auto& tile_info : original_tiles)
  {
    ia_tiles[tile_info.y][tile_info.x] = tile_info.tile;
  }
}

/**
 * Saves level data to a file.
 * @param subset The subset name where the level is saved.
 * @param level The level number to save.
 * Writes all level data including tiles and objects to a file.
 */
void Level::save(const std::string& subset, int level)
{
  // Construct the filename path using std::filesystem
  fs::path filename = fs::path(st_dir) / "levels" / subset / ("level" + to_string(level) + ".stl");
  if (!fwriteable(filename.string().c_str()))
  {
    filename = fs::path(datadir) / "levels" / subset / ("level" + to_string(level) + ".stl");
  }

  FILE* fi = fopen(filename.string().c_str(), "w");
  if (fi == nullptr)
  {
    perror(filename.string().c_str());
    st_shutdown();
    exit(-1);
  }

  // Write header:
  fprintf(fi, ";SuperTux-Level\n");
  fprintf(fi, "(supertux-level\n");

  fprintf(fi, "  (version %d)\n", 1);
  fprintf(fi, "  (name \"%s\")\n", name.c_str());
  fprintf(fi, "  (author \"%s\")\n", author.c_str());
  fprintf(fi, "  (music \"%s\")\n", song_title.c_str());
  fprintf(fi, "  (background \"%s\")\n", bkgd_image.c_str());
  fprintf(fi, "  (particle_system \"%s\")\n", particle_system.c_str());
  fprintf(fi, "  (bkgd_speed %d)\n", bkgd_speed);
  fprintf(fi, "  (bkgd_red_top %d)\n", bkgd_top.red);
  fprintf(fi, "  (bkgd_green_top %d)\n", bkgd_top.green);
  fprintf(fi, "  (bkgd_blue_top %d)\n", bkgd_top.blue);
  fprintf(fi, "  (bkgd_red_bottom %d)\n", bkgd_bottom.red);
  fprintf(fi, "  (bkgd_green_bottom %d)\n", bkgd_bottom.green);
  fprintf(fi, "  (bkgd_blue_bottom %d)\n", bkgd_bottom.blue);
  fprintf(fi, "  (time %d)\n", time_left);
  fprintf(fi, "  (width %d)\n", width);
  if (back_scrolling)
  {
    fprintf(fi, "  (back_scrolling #t)\n");
  }
  else
  {
    fprintf(fi, "  (back_scrolling #f)\n");
  }
  fprintf(fi, "  (hor_autoscroll_speed %2.1f)\n", hor_autoscroll_speed);
  fprintf(fi, "  (gravity %2.1f)\n", gravity);
  fprintf(fi, "  (background-tm ");

  for (int y = 0; y < 15; ++y)
  {
    for (int i = 0; i < width; ++i)
    {
      fprintf(fi, " %d ", bg_tiles[y][i]);
    }
  }

  fprintf(fi, ")\n");
  fprintf(fi, "  (interactive-tm ");

  for (int y = 0; y < 15; ++y)
  {
    for (int i = 0; i < width; ++i)
    {
      fprintf(fi, " %d ", ia_tiles[y][i]);
    }
  }

  fprintf(fi, ")\n");
  fprintf(fi, "  (foreground-tm ");

  for (int y = 0; y < 15; ++y)
  {
    for (int i = 0; i < width; ++i)
    {
      fprintf(fi, " %d ", fg_tiles[y][i]);
    }
  }

  fprintf(fi, ")\n");

  fprintf(fi, "(reset-points\n");
  for (auto& reset_point : reset_points)
  {
    fprintf(fi, "(point (x %d) (y %d))\n", reset_point.x, reset_point.y);
  }
  fprintf(fi, ")\n");

  fprintf(fi, "(objects\n");

  for (auto& badguy : badguy_data)
  {
    fprintf(fi, "(%s (x %d) (y %d) (stay-on-platform %s))\n",
      badguykind_to_string(badguy.kind).c_str(),
      badguy.x,
      badguy.y,
      badguy.stay_on_platform ? "#t" : "#f");
  }

  fprintf(fi, ")\n");

  fprintf(fi, ")\n");

  fclose(fi);
}

/**
 * Cleans up the level, releasing resources.
 * Clears all tile vectors, reset points, and resets attributes to defaults.
 */
void Level::cleanup()
{
  for (int i = 0; i < 15; ++i)
  {
    bg_tiles[i].clear();
    ia_tiles[i].clear();
    fg_tiles[i].clear();
  }

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
  if (!bkgd_image.empty())
  {
    fs::path fname = fs::path(st_dir) / "background" / bkgd_image;
    if (!faccessible(fname.string().c_str()))
    {
      fname = fs::path(datadir) / "images/background" / bkgd_image;
    }
    if (!img_bkgd)
    {
      img_bkgd = new Surface(fname.string().c_str(), IGNORE_ALPHA);
    }
  }
  else
  {
    delete img_bkgd;
    img_bkgd = nullptr;
  }
}

/**
 * Loads a level-specific image.
 * @param ptexture Pointer to the Surface object to store the loaded image.
 * @param theme The theme name to load the image from.
 * @param file The filename of the image to load.
 * @param use_alpha Whether to use alpha channel for the image.
 */
void Level::load_image(Surface** ptexture, string theme, const char* file, int use_alpha)
{
  fs::path fname = fs::path(st_dir) / "themes" / theme / file;
  if (!faccessible(fname.string().c_str()))
  {
    fname = fs::path(datadir) / "images/themes" / theme / file;
  }

  *ptexture = new Surface(fname.string().c_str(), use_alpha);
}

/**
 * Changes the size (width) of the level.
 * @param new_width The new width of the level
 * Resizes all tile layers to the new width, ensuring a minimum width of 21.
 */
void Level::change_size(int new_width)
{
  if (new_width < 21)
  {
    new_width = 21;
  }

  for (int y = 0; y < 15; ++y)
  {
    ia_tiles[y].resize(new_width, 0);
    bg_tiles[y].resize(new_width, 0);
    fg_tiles[y].resize(new_width, 0);
  }

  width = new_width;
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
  int yy = static_cast<int>(y) / 32;
  int xx = static_cast<int>(x) / 32;

  if (yy >= 0 && yy < 15 && xx >= 0 && xx <= width)
  {
    switch (tm)
    {
      case TM_BG:
        bg_tiles[yy][xx] = c;
        break;
      case TM_IA:
        ia_tiles[yy][xx] = c;
        break;
      case TM_FG:
        fg_tiles[yy][xx] = c;
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
 * Frees the level's music resources.
 * Sets the current level music to the end level music.
 */
void Level::free_song()
{
  level_song = level_end_song;
  level_song_fast = level_end_song;
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
  int xx = static_cast<int>(x) / 32;
  int yy = static_cast<int>(y) / 32;

  if (yy >= 0 && yy < 15 && xx >= 0 && xx <= width)
  {
    return ia_tiles[yy][xx];
  }
  else
  {
    return 0;
  }
}

/**
 * Gets the tile at a specific grid position.
 * @param x The x-coordinate in the grid.
 * @param y The y-coordinate in the grid.
 * @return The tile ID at the specified grid position.
 */
unsigned int Level::get_tile_at(int x, int y) const
{
  if (x < 0 || x > width || y < 0 || y > 14)
  {
    return 0;
  }

  return ia_tiles[y][x];
}

/* EOF */
