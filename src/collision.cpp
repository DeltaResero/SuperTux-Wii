//  collision.cpp
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "defines.h"
#include "collision.h"
#include "scene.h"
#include "world.h"
#include "level.h"
#include "tile.h"

/**
 * Checks if two rectangles are colliding.
 * @param one The first rectangle.
 * @param two The second rectangle.
 * @return bool True if the rectangles are colliding, false otherwise.
 */
bool rectcollision(const base_type& one, const base_type& two)
{
  return (one.x >= two.x - one.width + 1 &&
          one.x <= two.x + two.width - 1 &&
          one.y >= two.y - one.height + 1 &&
          one.y <= two.y + two.height - 1);
}

/**
 * Checks if two rectangles are colliding with an offset.
 * @param one The first rectangle.
 * @param two The second rectangle.
 * @param off_x The x-axis offset.
 * @param off_y The y-axis offset.
 * @return bool True if the rectangles are colliding, false otherwise.
 */
bool rectcollision_offset(const base_type& one, const base_type& two, float off_x, float off_y)
{
  return (one.x >= two.x - one.width + off_x + 1 &&
          one.x <= two.x + two.width + off_x - 1 &&
          one.y >= two.y - one.height + off_y + 1 &&
          one.y <= two.y + two.height + off_y - 1);
}

/**
 * Checks for a collision between an object and the map's solid tiles.
 * @param base The object's base rectangle.
 * @return bool True if there is a collision, false otherwise.
 */
bool collision_object_map(const base_type& base)
{
  if (!World::current()) return false;

  const Level& level = *World::current()->get_level();
  TileManager& tilemanager = *TileManager::instance();

  // we make the collision rectangle 1 pixel smaller
  int starttilex = int(base.x + 1) / 32;
  int starttiley = int(base.y + 1) / 32;
  int max_x = int(base.x + base.width);
  int max_y = int(base.y + base.height);

  for (int x = starttilex; x * 32 < max_x; ++x)
  {
    for (int y = starttiley; y * 32 < max_y; ++y)
    {
      Tile* tile = tilemanager.get(level.get_tile_at(x, y));
      if (tile && tile->solid)
        return true;
    }
  }

  return false;
}

/**
 * Performs a collision check using a custom function on each tile.
 * @param base The object's base rectangle.
 * @param function The function to apply to each tile.
 * @return void* A pointer to the tile if a collision is detected, otherwise 0.
 */
void* collision_func(const base_type& base, tiletestfunction function)
{
  const Level& level = *World::current()->get_level();
  TileManager& tilemanager = *TileManager::instance();

  int starttilex = int(base.x) / 32;
  int starttiley = int(base.y) / 32;
  int max_x = int(base.x + base.width);
  int max_y = int(base.y + base.height);

  for (int x = starttilex; x * 32 < max_x; ++x)
  {
    for (int y = starttiley; y * 32 < max_y; ++y)
    {
      Tile* tile = tilemanager.get(level.get_tile_at(x, y));
      void* result = function(tile);
      if (result != 0)
        return result;
    }
  }

  return 0;
}

/**
 * A function to test if a tile is a goal tile.
 * @param tile The tile to test.
 * @return void* A pointer to the tile if it is a goal, otherwise 0.
 */
static void* test_goal_tile_function(Tile* tile)
{
  if (tile && tile->goal)
    return tile;
  return 0;
}

/**
 * Checks if an object collides with a goal tile.
 * @param base The object's base rectangle.
 * @return Tile* A pointer to the goal tile if a collision is detected, otherwise 0.
 */
Tile* collision_goal(const base_type& base)
{
  return (Tile*)collision_func(base, test_goal_tile_function);
}

/**
 * Performs swept collision detection for an object moving from an old to a new position.
 * @param old The object's previous position.
 * @param current The object's current position.
 */
void collision_swept_object_map(base_type* old, base_type* current)
{
  int steps; /* Used to speed up the collision tests, by stepping every 16pixels in the path. */
  int h;
  float lpath; /* Holds the longest path, which is either in X or Y direction. */
  float xd, yd; /* Hold the smallest steps in X and Y directions. */
  float temp, xt, yt; /* Temporary variable. */

  lpath = 0;
  xd = 0;
  yd = 0;

  if (old->x == current->x && old->y == current->y)
  {
    return;
  }
  else if (old->x == current->x && old->y != current->y)
  {
    lpath = current->y - old->y;
    if (lpath < 0)
    {
      yd = -1;
      lpath = -lpath;
    }
    else
    {
      yd = 1;
    }

    h = 1;
    xd = 0;
  }
  else if (old->x != current->x && old->y == current->y)
  {
    lpath = current->x - old->x;
    if (lpath < 0)
    {
      xd = -1;
      lpath = -lpath;
    }
    else
    {
      xd = 1;
    }
    h = 2;
    yd = 0;
  }
  else
  {
    lpath = current->x - old->x;
    if (lpath < 0)
      lpath = -lpath;
    if (current->y - old->y > lpath || old->y - current->y > lpath)
      lpath = current->y - old->y;
    if (lpath < 0)
      lpath = -lpath;
    h = 3;
    xd = (current->x - old->x) / lpath;
    yd = (current->y - old->y) / lpath;
  }

  steps = (int)(lpath / (float)16);

  float orig_x = old->x;
  float orig_y = old->y;
  old->x += xd;
  old->y += yd;

  for (float i = 0; i <= lpath; old->x += xd, old->y += yd, ++i)
  {
    if (steps > 0)
    {
      old->y += yd * 16.;
      old->x += xd * 16.;
      steps--;
    }

    if (collision_object_map(*old))
    {
      switch (h)
      {
      case 1:
        current->y = old->y - yd;
        while (collision_object_map(*current))
          current->y -= yd;
        break;
      case 2:
        current->x = old->x - xd;
        while (collision_object_map(*current))
          current->x -= xd;
        break;
      case 3:
        xt = current->x;
        yt = current->y;
        current->x = old->x - xd;
        current->y = old->y - yd;
        while (collision_object_map(*current))
        {
          current->x -= xd;
          current->y -= yd;
        }

        temp = current->x;
        current->x = xt;
        if (!collision_object_map(*current))
          break;
        current->x = temp;
        temp = current->y;
        current->y = yt;

        if (!collision_object_map(*current))
        {
          break;
        }
        else
        {
          current->y = temp;
          while (!collision_object_map(*current))
            current->y += yd;
          current->y -= yd;
          break;
        }

        break;
      default:
        break;
      }
      break;
    }
  }

  if ((xd > 0 && current->x < orig_x) || (xd < 0 && current->x > orig_x))
    current->x = orig_x;
  if ((yd > 0 && current->y < orig_y) || (yd < 0 && current->y > orig_y))
    current->y = orig_y;

  *old = *current;
}

/**
 * Returns the tile at the specified coordinates.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return Tile* A pointer to the tile at the specified coordinates.
 */
Tile* gettile(float x, float y)
{
  return TileManager::instance()->get(World::current()->get_level()->gettileid(x, y));
}

/**
 * Checks if a tile at the specified coordinates is solid.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile is solid, false otherwise.
 */
bool issolid(float x, float y)
{
  Tile* tile = gettile(x, y);
  return tile && tile->solid;
}

/**
 * Checks if a tile at the specified coordinates is a brick.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile is a brick, false otherwise.
 */
bool isbrick(float x, float y)
{
  Tile* tile = gettile(x, y);
  return tile && tile->brick;
}

/**
 * Checks if a tile at the specified coordinates is ice.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile is ice, false otherwise.
 */
bool isice(float x, float y)
{
  Tile* tile = gettile(x, y);
  return tile && tile->ice;
}

/**
 * Checks if a tile at the specified coordinates is a full box.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile is a full box, false otherwise.
 */
bool isfullbox(float x, float y)
{
  Tile* tile = gettile(x, y);
  return tile && tile->fullbox;
}

/**
 * Checks if a tile at the specified coordinates contains a distro.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile contains a distro, false otherwise.
 */
bool isdistro(float x, float y)
{
  Tile* tile = gettile(x, y);
  return tile && tile->distro;
}

// EOF
