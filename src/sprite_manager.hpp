// src/sprite_manager.hpp
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
#include <string_view>
#include "sprite.hpp"

// Manages loading and accessing sprites by name
class SpriteManager
{
 private:
  // Use std::less<> to enable heterogeneous lookup (finding by string_view)
  typedef std::map<std::string, Sprite*, std::less<>> Sprites;
  Sprites sprites;                                // Collection of sprites

 public:
  SpriteManager(std::string_view filename); // Loads sprites from a resource file
  ~SpriteManager(); // Destructor, frees allocated sprites

  void load_resfile(std::string_view filename); // Loads sprite definitions from a resource file

  // Retrieves a Sprite by name, do not delete the returned object
  Sprite* load(std::string_view name);
};

#endif

// EOF
