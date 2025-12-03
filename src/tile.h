// src/tile.h
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

#ifndef SUPERTUX_TILE_H
#define SUPERTUX_TILE_H

#include <set>
#include <map>
#include <vector>
#include "texture.h"
#include "globals.h"
#include "lispreader.h"
#include "setup.h"

/**
Tile Class
*/
class Tile
{
public:
  Tile();
  ~Tile();

  int id;

  std::vector<Surface*> images;

  std::vector<std::string>  filenames;

  /** solid tile that is indestructable by Tux */
  bool solid;

  /** a brick that can be destroyed by jumping under it */
  bool brick;

  /** FIXME: ? */
  bool ice;

  /** water */
  bool water;

  /** Bonusbox, content is stored in \a data */
  bool fullbox;

  /** Tile is a distro/coin */
  bool distro;

  /** the level should be finished when touching a goaltile.
   * if data is 0 then the endsequence should be triggered, if data is 1
   * then we can finish the level instantly.
   */
  bool goal;

  /** General purpose data attached to a tile (content of a box, type of coin) */
  int data;

  /** Id of the tile that is going to replace this tile once it has
      been collected or jumped at */
  int next_tile;

  int anim_speed;

  /** Draw a tile on the screen: */
  static void draw(float x, float y, unsigned int c, Uint8 alpha = 255);
  static void draw_stretched(float x, float y, int w, int h, unsigned int c, Uint8 alpha = 255);
};

struct TileGroup
{
  friend bool operator<(const TileGroup& lhs, const TileGroup& rhs)
  {
    return lhs.name < rhs.name;
  };
  friend bool operator>(const TileGroup& lhs, const TileGroup& rhs)
  {
    return lhs.name > rhs.name;
  };

  std::string name;
  std::vector<int> tiles;
};

class TileManager
{
private:
  TileManager();
  ~TileManager();

  std::vector<Tile*> tiles;
  static TileManager* instance_ ;
  static std::set<TileGroup>* tilegroups_;
  void load_tileset(std::string filename);

  std::string current_tileset;

public:
  static TileManager* instance()
  {
    if (!instance_)
    {
      instance_ = new TileManager();
    }
    return instance_;
  }

  static void destroy_instance()
  {
    // The static destroy function is responsible for cleaning up ALL static resources.
    delete tilegroups_;
    tilegroups_ = 0;

    delete instance_;
    instance_ = 0;
  }

  static std::set<TileGroup>* tilegroups()
  {
    if (!instance_)
    {
      instance_ = new TileManager();
    }
    if (!tilegroups_)
    {
      tilegroups_ = new std::set<TileGroup>;
    }
    return tilegroups_;
  }

  Tile* get(unsigned int id)
  {
    if (id < tiles.size())
    {
      return tiles[id];
    }
    else
    {
      // Never return 0, but return the 0th tile instead so that
      // user code doesn't have to check for NULL pointers all over
      // the place
      return tiles[0];
    }
  }
};

#endif /*SUPERTUX_TILE_H*/

// EOF
