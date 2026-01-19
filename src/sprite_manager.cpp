// src/sprite_manager.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <iostream>
#include <cstring>
#include <string_view>
#include "lispreader.hpp"
#include "sprite_manager.hpp"

/**
 * Constructs a SpriteManager and loads sprites from a resource file.
 * @param filename The path to the resource file containing sprite definitions.
 */
SpriteManager::SpriteManager(std::string_view filename)
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
void SpriteManager::load_resfile(std::string_view filename)
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
Sprite* SpriteManager::load(std::string_view name)
{
  // Thanks to std::less<> in the map definition, find() accepts string_view
  // without creating a temporary std::string.
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
