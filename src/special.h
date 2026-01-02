// src/special.h
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2003 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_SPECIAL_H
#define SUPERTUX_SPECIAL_H

#include <SDL2/SDL.h>
#include "type.h"
#include "texture.h"
#include "collision.h"
#include "player.h"
#include "physic.h"

// Upgrade types
enum UpgradeKind {
  UPGRADE_GROWUP,
  UPGRADE_ICEFLOWER,
  UPGRADE_HERRING,
  UPGRADE_1UP
};

void load_special_gfx();

class RenderBatcher;

class Upgrade : public GameObject
{
public:
  bool removable;
  UpgradeKind kind;
  Direction dir;
  Physic physic;

  void init(float x, float y, Direction dir, UpgradeKind kind);
  void action(float frame_ratio);
  void updatePhysics(float deltaTime);
  void draw() override {}
  void collision(void* p_c_object, int c_object, CollisionType type);
  std::string type() { return "Upgrade"; };

  ~Upgrade() {};
};

class Bullet : public GameObject
{
 public:
  bool removable;
  int life_count;
  // base_type is already part of GameObject, no need to redeclare
  base_type old_base;

  void init(float x, float y, float xm, Direction dir);
  void action(float frame_ratio);
  void updatePhysics(float deltaTime);
  void draw() override {}
  void collision(int c_object);
  std::string type() { return "Bullet"; };
};

#endif /*SUPERTUX_SPECIAL_H*/

// EOF
