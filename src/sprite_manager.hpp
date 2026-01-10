// src/sprite_manager.h
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

#ifndef HEADER_SPRITE_MANAGER_HXX
#define HEADER_SPRITE_MANAGER_HXX

#include <map>
#include <string>
#include "sprite.hpp"

// Manages loading and accessing sprites by name
class SpriteManager
{
 private:
  typedef std::map<std::string, Sprite*> Sprites; // Map of sprite names to Sprite objects
  Sprites sprites;                                // Collection of sprites

 public:
  SpriteManager(const std::string& filename); // Loads sprites from a resource file
  ~SpriteManager(); // Destructor, frees allocated sprites

  void load_resfile(const std::string& filename); // Loads sprite definitions from a resource file

  // Retrieves a Sprite by name, do not delete the returned object
  Sprite* load(const std::string& name);
};

#endif

// EOF
