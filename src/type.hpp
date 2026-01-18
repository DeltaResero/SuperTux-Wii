// src/type.hpp
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

#ifndef SUPERTUX_TYPE_H
#define SUPERTUX_TYPE_H

#include <string>
#include <vector>
#include <SDL.h>
#include "scene.hpp"

/* 'Base' type for game objects */

struct base_type
{
  float x;
  float y;
  float xm;
  float ym;
  float width;
  float height;
};

/* Base class for all dynamic game object. */
class GameObject
{
public:
  GameObject() {};
  virtual ~GameObject() {};
  virtual void action(float frame_ratio) = 0;
  virtual void draw() = 0;
  virtual std::string type() = 0;
  /* Draw ignoring the scroll_x value. FIXME: Hack? Should be discussed. @tobgle*/
  void draw_on_screen(float x = -1, float y = -1)
  {
    base_type btmp = base;
    if(x != -1 || y != -1)
    {
      btmp = base;
      if(x != -1)
        base.x = x;
      if(y != -1)
        base.y = y;
    }
    float tmp = scroll_x;
    scroll_x = 0; draw();
    scroll_x = tmp;
    base = btmp;
  };

  void move_to(float x, float y) { base.x = x; base.y = y; };

  base_type base;
  base_type old_base;
};

struct string_list_type
{
  int num_items;
  int active_item;
  char **item;
};

using StringList = std::vector<std::string>;

#endif /*SUPERTUX_TYPE_H*/

// EOF
