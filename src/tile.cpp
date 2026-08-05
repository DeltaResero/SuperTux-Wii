// src/tile.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "tile.hpp"
#include "scene.hpp"
#include <cassert>
#include <iostream>
#include <cstring>
#include <filesystem>
#include "render_batcher.hpp"

// Static member initialization
TileManager* TileManager::instance_ = nullptr;
std::set<TileGroup>* TileManager::tilegroups_ = nullptr;

namespace fs = std::filesystem;  // Alias for ease of use

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
 * Loads a tileset from a file, replacing whatever was loaded before.
 * @param filename The path to the tileset file.
 * If the filename matches the currently loaded tileset, it does nothing.
 */
void TileManager::load_tileset(const std::string& filename)
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

  /* A tileset may pull in others, and they all fill this same vector, so the
     discarding above belongs here and must not happen again further down.
     The visited set is what stops a file that includes itself, directly or
     through a chain, from recursing until the stack runs out. */
  std::set<std::string> visited;
  parse_tileset_file(filename, visited);

  current_tileset = filename;
}

/**
 * Parses one tileset file into the current tile vector, following any tilesets
 * it includes. Adds to what is already loaded rather than replacing it, so
 * only load_tileset may call this without an inherited visited set.
 * @param filename The path to the tileset file.
 * @param visited The files already open further up the include chain.
 */
void TileManager::parse_tileset_file(const std::string& filename,
                                     std::set<std::string>& visited)
{
  if (!visited.insert(filename).second)
  {
    if (verbose)
    {
      std::cerr << "Warning: Tileset " << filename
                << " is already being loaded, skipping.\n";
    }
    return;
  }

  lisp_object_t* root_obj = lisp_read_from_file(filename);

  if (!root_obj)
  {
    st_abort("Couldn't load file", filename);
  }

  if (strcmp(lisp_symbol(lisp_car(root_obj)), "supertux-tiles") != 0)
  {
    assert(0);
    return;
  }

  parse_tileset_elements(lisp_cdr(root_obj), visited);
}

/**
 * Walks the element list of one tileset file and dispatches on each symbol.
 * @param elements The list of elements following the supertux-tiles header.
 * @param visited The files already open further up the include chain.
 */
void TileManager::parse_tileset_elements(lisp_object_t* elements,
                                         std::set<std::string>& visited)
{
  /* Each file starts at offset zero and a properties element moves it, so
     an included file numbers its tiles independently of its includer. */
  int tileset_id = 0;
  const std::string base_path = datadir + "/images/tilesets/";

  for (lisp_object_t* cur = elements; !lisp_nil_p(cur); cur = lisp_cdr(cur))
  {
    lisp_object_t* element = lisp_car(cur);
    const char* symbol = lisp_symbol(lisp_car(element));

    if (strcmp(symbol, "tile") == 0)
    {
      parse_tile(element, tileset_id, base_path);
    }
    else if (strcmp(symbol, "tileset") == 0)
    {
      parse_nested_tileset(element, base_path, visited);
    }
    else if (strcmp(symbol, "tilegroup") == 0)
    {
      parse_tilegroup(element);
    }
    else if (strcmp(symbol, "properties") == 0)
    {
      // Handle properties such as tileset ID
      LispReader reader(lisp_cdr(element));
      reader.read_int("id", &tileset_id);
      tileset_id *= 1000;
    }
    else if (verbose)
    {
      puts("Unhandled symbol");
    }
  }
}

/**
 * Resolves a tileset element's filename and follows it.
 * @param element The lisp element naming the tileset to include.
 * @param base_path The directory tileset files are loaded from.
 * @param visited The files already open further up the include chain.
 */
void TileManager::parse_nested_tileset(lisp_object_t* element,
                                       const std::string& base_path,
                                       std::set<std::string>& visited)
{
  LispReader reader(lisp_cdr(element));
  std::string nested_filename;

  if (reader.read_string("file", &nested_filename))
  {
    parse_tileset_file(base_path + nested_filename, visited);
  }
  else if (verbose)
  {
    std::cerr << "Warning: Tileset missing required 'file' property, skipping.\n";
  }
}

/**
 * Parses one tile element and stores it at its id, offset by the tileset id.
 * @param element The lisp element describing the tile.
 * @param tileset_id The offset applied to every tile id in this file.
 * @param base_path The directory the tile's images are loaded from.
 */
void TileManager::parse_tile(lisp_object_t* element, int tileset_id,
                             const std::string& base_path)
{
  // Every field carries its default from the class, so only what the file
  // actually names is read below.
  Tile* tile = new Tile;

  LispReader reader(lisp_cdr(element));
  if (!reader.read_int("id", &tile->id))
  {
    if (verbose)
    {
      std::cerr << "Warning: Tile missing required 'id' property, skipping.\n";
    }
    delete tile;
    return;
  }

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

  for (const std::string& image_filename : tile->filenames)
  {
    Surface* cur_image = nullptr;
    tile->images.push_back(cur_image);
    tile->images.back() = new Surface(base_path + image_filename, true);
  }

  // Ensure the tiles vector is large enough
  if (tile->id + tileset_id >= int(tiles.size()))
  {
    tiles.resize(tile->id + tileset_id + 1);
  }

  tiles[tile->id + tileset_id] = tile;
}

/**
 * Parses one tilegroup element into the shared tilegroup set.
 * @param element The lisp element describing the tilegroup.
 */
void TileManager::parse_tilegroup(lisp_object_t* element)
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

  tilegroups_->insert(new_group);
}

/**
 * Updates animation frames for all loaded tiles.
 * This should be called once per frame to avoid recalculating the frame index
 * for every single tile drawn on screen.
 * @param frame_counter The global frame counter.
 */
void TileManager::update_animations(unsigned int frame_counter)
{
  for (Tile* tile : tiles)
  {
    if (tile && !tile->images.empty())
    {
      // Calculate once per tile type
      if (tile->anim_speed > 0)
      {
        tile->current_frame_index = ((frame_counter * 25) / tile->anim_speed) % tile->images.size();
      }
      else
      {
        tile->current_frame_index = 0;
      }
    }
  }
}

/**
 * Draws a tile at a specific position.
 * @param x The x-coordinate in the level, measured from the level's left edge.
 *          Scrolling is applied here, so the caller must not subtract scroll_x.
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

  // Use pre-calculated index
  int frame_index = ptile->current_frame_index;

  if (batcher)
  {
      batcher->add(ptile->images[frame_index], x, y);
  }
  else
  {
      ptile->images[frame_index]->draw(x - scroll_x, y, alpha);
  }
}

// EOF
