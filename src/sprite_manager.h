//  $Id: sprite_manager.h 737 2004-04-26 12:21:23Z grumbel $
//
//  SuperTux
//  Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_SPRITE_MANAGER_HXX
#define HEADER_SPRITE_MANAGER_HXX

#include <map>
#include <string>
#include "sprite.h"

/**
 * Manages loading and accessing sprites by name.
 * Stores sprites in a map for efficient retrieval.
 */
class SpriteManager
{
 private:
  typedef std::map<std::string, Sprite*> Sprites; // Map of sprite names to Sprite objects
  Sprites sprites;                                // Collection of sprites

 public:
  /**
   * Constructs a SpriteManager and loads sprites from a resource file.
   * @param filename Path to the resource file.
   */
  SpriteManager(const std::string& filename);

  /**
   * Destroys the SpriteManager and cleans up allocated sprites.
   */
  ~SpriteManager();

  /**
   * Loads sprite definitions from a resource file.
   * @param filename Path to the resource file.
   */
  void load_resfile(const std::string& filename);

  /**
   * Retrieves a Sprite by name.
   * WARNING: Do not delete the returned object.
   * @param name Name of the sprite to retrieve.
   * @return Sprite* Pointer to the Sprite object, or nullptr if not found.
   */
  Sprite* load(const std::string& name);
};

#endif

// EOF
