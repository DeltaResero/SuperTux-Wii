// src/collision.cpp
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

#include "defines.hpp"
#include "collision.hpp"
#include "scene.hpp"
#include "world.hpp"
#include "level.hpp"
#include "tile.hpp"
#include <functional>
#include <cmath>
#include <algorithm>

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
  int starttilex = static_cast<int>(base.x + 1) / 32;
  int starttiley = static_cast<int>(base.y + 1) / 32;
  int max_x = static_cast<int>(base.x + base.width);
  int max_y = static_cast<int>(base.y + base.height);

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
 * Checks tiles within the object's bounding box against a given predicate.
 * @param base The object's base rectangle.
 * @param predicate The function to apply to each tile, returning the tile if it matches.
 * @return A pointer to the first tile that matches the predicate, otherwise nullptr.
 */
Tile* collision_func(const base_type& base, std::function<Tile*(Tile*)> predicate)
{
  if (!World::current() || !World::current()->get_level()) return nullptr;

  const Level& level = *World::current()->get_level();
  TileManager& tilemanager = *TileManager::instance();

  int starttilex = static_cast<int>(base.x) / 32;
  int starttiley = static_cast<int>(base.y) / 32;
  int max_x = static_cast<int>(base.x + base.width);
  int max_y = static_cast<int>(base.y + base.height);

  for (int x = starttilex; x * 32 < max_x; ++x)
  {
    for (int y = starttiley; y * 32 < max_y; ++y)
    {
      Tile* tile = tilemanager.get(level.get_tile_at(x, y));
      Tile* result = predicate(tile);
      if (result != nullptr)
        return result;
    }
  }

  return nullptr;
}

/**
 * Checks if an object collides with a goal tile.
 * @param base The object's base rectangle.
 * @return Tile* A pointer to the goal tile if a collision is detected, otherwise nullptr.
 */
Tile* collision_goal(const base_type& base)
{
  // Use a lambda for the predicate, eliminating the need for a separate test function.
  return collision_func(base, [](Tile* tile) -> Tile* {
    return (tile && tile->goal) ? tile : nullptr;
  });
}

/**
 * Performs swept collision detection for an object moving from an old to a new position.
 * This prevents fast-moving objects from passing through solid tiles.
 * @param old The object's previous position and dimensions.
 * @param current The object's target position and dimensions (will be modified on collision).
 */
void collision_swept_object_map(base_type* old, base_type* current)
{
  // If there's no movement, no collision can occur.
  if (old->x == current->x && old->y == current->y)
  {
    return;
  }

  // --- 1. Determine dominant axis and step increments ---
  float longest_path;
  float x_step, y_step;
  enum AxisCase { VERTICAL = 1, HORIZONTAL = 2, DIAGONAL = 3 } axis_case;

  if (old->x == current->x)
  {
    axis_case = VERTICAL;
    longest_path = current->y - old->y;
    x_step = 0;
    y_step = (longest_path > 0) ? 1.0f : -1.0f;
    longest_path = std::abs(longest_path);
  }
  else if (old->y == current->y)
  {
    axis_case = HORIZONTAL;
    longest_path = current->x - old->x;
    y_step = 0;
    x_step = (longest_path > 0) ? 1.0f : -1.0f;
    longest_path = std::abs(longest_path);
  }
  else
  {
    axis_case = DIAGONAL;
    float dx = current->x - old->x;
    float dy = current->y - old->y;
    longest_path = std::max(std::abs(dx), std::abs(dy));
    x_step = dx / longest_path;
    y_step = dy / longest_path;
  }

  // --- 2. Step along the movement path and check for collision ---
  const float original_pos_x = old->x;
  const float original_pos_y = old->y;

  // We iterate pixel by pixel along the longest path.
  for (float i = 0; i <= longest_path; ++i)
  {
    old->x += x_step;
    old->y += y_step;

    if (collision_object_map(*old))
    {
      // --- 3. Resolve collision ---
      if (axis_case == VERTICAL)
      {
        current->y = old->y - y_step;
        while (collision_object_map(*current))
          current->y -= y_step;
      }
      else if (axis_case == HORIZONTAL)
      {
        current->x = old->x - x_step;
        while (collision_object_map(*current))
          current->x -= x_step;
      }
      else if (axis_case == DIAGONAL)
      {
        const float target_x = current->x;
        const float target_y = current->y;

        // Back out to the last known non-colliding position.
        current->x = old->x - x_step;
        current->y = old->y - y_step;
        while (collision_object_map(*current))
        {
          current->x -= x_step;
          current->y -= y_step;
        }
        const float safe_x = current->x;
        const float safe_y = current->y;

        // Attempt to slide along the X-axis (horizontal movement).
        current->x = target_x;
        if (!collision_object_map(*current))
        {
          // X-slide succeeded, stay here
          break;
        }

        // X-slide failed. Revert X and attempt to slide along the Y-axis.
        current->x = safe_x;
        current->y = target_y;
        if (!collision_object_map(*current))
        {
          // Y-slide succeeded, stay here
          break;
        }

        // Both slides failed (corner hit). Need to push away from the wall.
        current->y = safe_y;
        while (!collision_object_map(*current))
        {
          current->y += y_step;
        }
        current->y -= y_step;
      }
      // Collision found and resolved, so we can exit the loop.
      break;
    }
  }

  // Final sanity check to prevent the resolution logic from pushing the object backwards.
  if ((x_step > 0 && current->x < original_pos_x) || (x_step < 0 && current->x > original_pos_x))
  {
    current->x = original_pos_x;
  }
  if ((y_step > 0 && current->y < original_pos_y) || (y_step < 0 && current->y > original_pos_y))
  {
    current->y = original_pos_y;
  }

  *old = *current;
}

/**
 * Returns the tile at the specified coordinates.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return Tile* A pointer to the tile at the specified coordinates, or nullptr if world is not loaded.
 */
Tile* gettile(float x, float y)
{
  if (!World::current() || !World::current()->get_level())
    return nullptr;
  return TileManager::instance()->get(World::current()->get_level()->gettileid(x, y));
}

/**
 * Private helper function to check a property of a tile at a given position.
 * This avoids duplicating the gettile logic in multiple public functions.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param predicate A function that takes a Tile* and returns true if it has the desired property.
 * @return True if the tile exists and the predicate returns true, false otherwise.
 */
static bool checkTilePropertyAt(float x, float y, const std::function<bool(const Tile*)>& predicate)
{
    const Tile* tile = gettile(x, y);
    return tile && predicate(tile);
}

/**
 * Checks if a tile at the specified coordinates is solid.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile is solid, false otherwise.
 */
bool issolid(float x, float y)
{
    return checkTilePropertyAt(x, y, [](const Tile* t) { return t->solid; });
}

/**
 * Checks if a tile at the specified coordinates is a brick.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile is a brick, false otherwise.
 */
bool isbrick(float x, float y)
{
    return checkTilePropertyAt(x, y, [](const Tile* t) { return t->brick; });
}

/**
 * Checks if a tile at the specified coordinates is ice.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile is ice, false otherwise.
 */
bool isice(float x, float y)
{
    return checkTilePropertyAt(x, y, [](const Tile* t) { return t->ice; });
}

/**
 * Checks if a tile at the specified coordinates is a full box.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile is a full box, false otherwise.
 */
bool isfullbox(float x, float y)
{
    return checkTilePropertyAt(x, y, [](const Tile* t) { return t->fullbox; });
}

/**
 * Checks if a tile at the specified coordinates contains a distro.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile contains a distro, false otherwise.
 */
bool isdistro(float x, float y)
{
    return checkTilePropertyAt(x, y, [](const Tile* t) { return t->distro; });
}

// EOF
