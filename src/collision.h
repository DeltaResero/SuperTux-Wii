//  collision.h
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
#include <functional>

class Tile;
class World;

// Collision objects
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

bool rectcollision(const base_type& one, const base_type& two); // Checks if two rectangles are colliding
bool rectcollision_offset(const base_type& one, const base_type& two, float off_x, float off_y); // Checks if rectangles collide with an offset
void collision_swept_object_map(base_type* old, base_type* current); // Swept collision detection for object movement
bool collision_object_map(const base_type& object); // Checks for object collision with map tiles
Tile* gettile(float x, float y); // Gets tile at specific coordinates
bool issolid(float x, float y); // Checks if the tile is solid
bool isbrick(float x, float y); // Checks if the tile is a brick
bool isice(float x, float y); // Checks if the tile is ice
bool isfullbox(float x, float y); // Checks if the tile is a full box
bool isdistro(float x, float y); // Checks if the tile contains a distro

// The predicate should return a pointer to the tile on a successful test, or nullptr otherwise.
Tile* collision_func(const base_type& base, std::function<Tile*(Tile*)> predicate);

Tile* collision_goal(const base_type& base); // Checks if an object collides with a goal tile

#endif /* SUPERTUX_COLLISION_H */

// EOF
