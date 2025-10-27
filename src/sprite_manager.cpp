//  sprite_manager.cpp
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

#include <iostream>
#include <cstring>
#include "lispreader.h"
#include "sprite_manager.h"

/**
 * Constructs a SpriteManager and loads sprites from a resource file.
 * @param filename The path to the resource file containing sprite definitions.
 */
SpriteManager::SpriteManager(const std::string& filename)
{
  load_resfile(filename);
}

/**
 * Destroys the SpriteManager and cleans up allocated sprites.
 * Deletes all dynamically allocated Sprite objects managed by this SpriteManager.
 */
SpriteManager::~SpriteManager()
{
  for (auto& pair : sprites)
  {
    delete pair.second;
  }
}

/**
 * Loads sprite definitions from a resource file.
 * This function reads the resource file, parses the sprite definitions, and creates Sprite objects.
 * @param filename The path to the resource file.
 */
void SpriteManager::load_resfile(const std::string& filename)
{
  lisp_object_t* root_obj = lisp_read_from_file(filename);
  if (!root_obj)
  {
    std::cerr << "SpriteManager: Couldn't load: " << filename << std::endl;
    return;
  }

  lisp_object_t* cur = root_obj;

  if (std::strcmp(lisp_symbol(lisp_car(cur)), "supertux-resources") != 0)
  {
    return;
  }

  cur = lisp_cdr(cur);

  while (cur)
  {
    lisp_object_t* el = lisp_car(cur);

    if (std::strcmp(lisp_symbol(lisp_car(el)), "sprite") == 0)
    {
      Sprite* sprite = new Sprite(lisp_cdr(el));
      const std::string& sprite_name = sprite->get_name();

      auto result = sprites.insert({sprite_name, sprite});
      if (!result.second) // If insertion failed, key already exists
      {
        delete result.first->second;
        result.first->second = sprite;
        std::cerr << "Warning: duplicate entry: '" << sprite_name << "'" << std::endl;
      }
    }
    else
    {
      std::cerr << "SpriteManager: Unknown tag" << std::endl;
    }

    cur = lisp_cdr(cur);
  }
}

/**
 * Retrieves a Sprite by name.
 * @param name The name of the sprite to retrieve.
 * @return Sprite* Pointer to the Sprite object, or nullptr if not found.
 */
Sprite* SpriteManager::load(const std::string& name)
{
  auto it = sprites.find(name);
  if (it != sprites.end())
  {
    return it->second;
  }
  else
  {
    std::cerr << "SpriteManager: Sprite '" << name << "' not found" << std::endl;
    return nullptr;
  }
}

// EOF
