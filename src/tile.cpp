// src/tile.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "tile.h"
#include "scene.h"
#include "assert.h"
#include <cstring>
#include <filesystem>
#include "render_batcher.h"

// Static member initialization
TileManager* TileManager::instance_ = nullptr;
std::set<TileGroup>* TileManager::tilegroups_ = nullptr;

namespace fs = std::filesystem;  // Alias for ease of use

/**
 * Constructor for Tile.
 * Initializes a Tile object.
 */
Tile::Tile()
{
  // Constructor: Initializes a Tile object
}

/**
 * Destructor for Tile.
 * Cleans up allocated memory for image surfaces.
 */
Tile::~Tile()
{
  // Destructor: Clean up allocated memory for image surfaces
  for (Surface* image : images)
  {
    delete image;
  }
}

/**
 * Constructor for TileManager.
 * Loads the initial tileset from the default location.
 */
TileManager::TileManager()
{
  // Constructor: Load the initial tileset
  std::string filename = datadir + "/images/tilesets/supertux.stgt";
  load_tileset(filename);
}

/**
 * Destructor for TileManager.
 * Cleans up allocated memory for tiles managed by this manager.
 */
TileManager::~TileManager()
{
  // Destructor: Clean up instance-specific resources (the tiles).
  for (Tile* tile : tiles)
  {
    delete tile;
  }
  // The static tilegroups_ pointer is now cleaned up in destroy_instance().
}

/**
 * Loads a tileset from a file.
 * @param filename The path to the tileset file.
 * If the filename matches the currently loaded tileset, it does nothing.
 */
void TileManager::load_tileset(std::string filename)
{
  if (filename == current_tileset)
  {
    return;
  }

  // Free old tiles to avoid memory leaks
  for (Tile* tile : tiles)
  {
    delete tile;
  }

  tiles.clear();

  lisp_object_t* root_obj = lisp_read_from_file(filename);

  if (!root_obj)
  {
    st_abort("Couldn't load file", filename);
  }

  if (strcmp(lisp_symbol(lisp_car(root_obj)), "supertux-tiles") == 0)
  {
    lisp_object_t* cur = lisp_cdr(root_obj);
    int tileset_id = 0;
    std::string base_path = datadir + "/images/tilesets/";

    while (!lisp_nil_p(cur))
    {
      lisp_object_t* element = lisp_car(cur);

      if (strcmp(lisp_symbol(lisp_car(element)), "tile") == 0)
      {
        Tile* tile = new Tile;

        // Initialize all fields to safe defaults
        tile->id = -1;
        tile->solid = false;
        tile->brick = false;
        tile->ice = false;
        tile->water = false;
        tile->fullbox = false;
        tile->distro = false;
        tile->goal = false;
        tile->data = 0;
        tile->next_tile = 0;
        tile->anim_speed = 25;

        // Parse the tile properties from the file
        LispReader reader(lisp_cdr(element));
#ifndef DDEBUG
        void(reader.read_int("id", &tile->id));
#else
        assert(reader.read_int("id", &tile->id));
#endif

        reader.read_bool("solid", &tile->solid);
        reader.read_bool("brick", &tile->brick);
        reader.read_bool("ice", &tile->ice);
        reader.read_bool("water", &tile->water);
        reader.read_bool("fullbox", &tile->fullbox);
        reader.read_bool("distro", &tile->distro);
        reader.read_bool("goal", &tile->goal);
        reader.read_int("data", &tile->data);
        reader.read_int("anim-speed", &tile->anim_speed);
        reader.read_int("next-tile", &tile->next_tile);
        reader.read_string_vector("images", &tile->filenames);

        // Load images and associate them with the tile
        tile->images.reserve(tile->filenames.size());

        for (const std::string& filename : tile->filenames)
        {
          Surface* cur_image = nullptr;
          tile->images.push_back(cur_image);
          tile->images.back() = new Surface(base_path + filename, true);
        }

        // Ensure the tiles vector is large enough
        if (tile->id + tileset_id >= int(tiles.size()))
        {
          tiles.resize(tile->id + tileset_id + 1);
        }

        tiles[tile->id + tileset_id] = tile;
      }
      else if (strcmp(lisp_symbol(lisp_car(element)), "tileset") == 0)
      {
        // Load a nested tileset file
        LispReader reader(lisp_cdr(element));
        std::string filename;
        reader.read_string("file", &filename);
        filename = datadir + "/images/tilesets/" + filename;
        load_tileset(filename);
      }
      else if (strcmp(lisp_symbol(lisp_car(element)), "tilegroup") == 0)
      {
        // Handle tilegroup properties
        TileGroup new_group;
        LispReader reader(lisp_cdr(element));
        reader.read_string("name", &new_group.name);
        reader.read_int_vector("tiles", &new_group.tiles);

        if (!tilegroups_)
        {
          tilegroups_ = new std::set<TileGroup>;
        }

        tilegroups_->insert(new_group).first;
      }
      else if (strcmp(lisp_symbol(lisp_car(element)), "properties") == 0)
      {
        // Handle properties such as tileset ID
        LispReader reader(lisp_cdr(element));
        reader.read_int("id", &tileset_id);
        tileset_id *= 1000;
      }
      else
      {
        puts("Unhandled symbol");
      }

      cur = lisp_cdr(cur);
    }
  }
  else
  {
    assert(0);
  }

  current_tileset = filename;
}

/**
 * Draws a tile at a specific position.
 * @param x The x-coordinate for drawing.
 * @param y The y-coordinate for drawing.
 * @param c The tile code.
 * @param alpha The alpha transparency value.
 */
void Tile::draw(RenderBatcher* batcher, float x, float y, unsigned int c, Uint8 alpha)
{
  if (c == 0)
  {
    return;
  }

  Tile* ptile = TileManager::instance()->get(c);

  if (!ptile || ptile->images.empty())
  {
    return;
  }

  int frame_index = ((global_frame_counter * 25) / ptile->anim_speed) % ptile->images.size();

  if (batcher)
  {
      // RenderBatcher expects World Coordinates (it subtracts scroll_x internally)
      // Tile::draw receives Screen Coordinates (scroll_x already subtracted)
      // We must add scroll_x back to convert Screen -> World for the batcher
      batcher->add(ptile->images[frame_index], x + scroll_x, y, 0, 0);
  }
  else
  {
      ptile->images[frame_index]->draw(x, y, alpha);
  }
}

/**
 * Draws a tile stretched to a specific size.
 * @param x The x-coordinate for drawing.
 * @param y The y-coordinate for drawing.
 * @param w The width of the drawn tile.
 * @param h The height of the drawn tile.
 * @param c The tile code.
 * @param alpha The alpha transparency value.
 */
void Tile::draw_stretched(float x, float y, int w, int h, unsigned int c, Uint8 alpha)
{
  if (c == 0)
  {
    return;
  }

  Tile* ptile = TileManager::instance()->get(c);

  if (!ptile || ptile->images.empty())
  {
    return;
  }

  int frame_index = ((global_frame_counter * 25) / ptile->anim_speed) % ptile->images.size();
  ptile->images[frame_index]->draw_stretched(x, y, w, h, alpha);
}

// EOF
