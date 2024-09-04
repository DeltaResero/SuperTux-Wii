//  $Id: collision.h 760 2004-04-26 19:11:54Z grumbel $
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

#ifndef SUPERTUX_COLLISION_H
#define SUPERTUX_COLLISION_H

#include "type.h"

class Tile;
class World;

/* Collision objects */
enum
{
  CO_BULLET,
  CO_BADGUY,
  CO_PLAYER
};

enum CollisionType {
  COLLISION_NORMAL,
  COLLISION_BUMP,
  COLLISION_SQUISH
};

/**
 * Checks if two rectangles are colliding.
 * @param one The first rectangle.
 * @param two The second rectangle.
 * @return bool True if the rectangles are colliding, false otherwise.
 */
bool rectcollision(const base_type& one, const base_type& two);

/**
 * Checks if two rectangles are colliding with an offset.
 * @param one The first rectangle.
 * @param two The second rectangle.
 * @param off_x The x-axis offset.
 * @param off_y The y-axis offset.
 * @return bool True if the rectangles are colliding, false otherwise.
 */
bool rectcollision_offset(const base_type& one, const base_type& two, float off_x, float off_y);

/**
 * Performs swept collision detection for an object moving from an old to a new position.
 * @param old The object's previous position.
 * @param current The object's current position.
 */
void collision_swept_object_map(base_type* old, base_type* current);

/**
 * Checks for a collision between an object and the map's solid tiles.
 * @param object The object's base rectangle.
 * @return bool True if there is a collision, false otherwise.
 */
bool collision_object_map(const base_type& object);

/**
 * Returns a pointer to the tile at the given x/y coordinates.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return Tile* A pointer to the tile at the specified coordinates.
 */
Tile* gettile(float x, float y);

/**
 * Some little helper functions to check for tile properties.
 * Checks if a tile at the specified coordinates is solid.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile is solid, false otherwise.
 */
bool issolid(float x, float y);

/**
 * Checks if a tile at the specified coordinates is a brick.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile is a brick, false otherwise.
 */
bool isbrick(float x, float y);

/**
 * Checks if a tile at the specified coordinates is ice.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile is ice, false otherwise.
 */
bool isice(float x, float y);

/**
 * Checks if a tile at the specified coordinates is a full box.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile is a full box, false otherwise.
 */
bool isfullbox(float x, float y);

/**
 * Checks if a tile at the specified coordinates contains a distro.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return bool True if the tile contains a distro, false otherwise.
 */
bool isdistro(float x, float y);

typedef void* (*tiletestfunction)(Tile* tile);

/**
 * Invokes the function for each tile the base rectangle collides with.
 * The function aborts and returns true as soon as the tiletestfunction returns != 0, then this value is returned.
 * Returns 0 if all tests failed.
 * @param base The object's base rectangle.
 * @param function The function to apply to each tile.
 * @return void* A pointer to the tile if a collision is detected, otherwise 0.
 */
void* collision_func(const base_type& base, tiletestfunction function);

/**
 * Checks if an object collides with a goal tile.
 * @param base The object's base rectangle.
 * @return Tile* A pointer to the goal tile if a collision is detected, otherwise 0.
 */
Tile* collision_goal(const base_type& base);

#endif /* SUPERTUX_COLLISION_H */

// EOF
