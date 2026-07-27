// src/special.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2003 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_SPECIAL_H
#define SUPERTUX_SPECIAL_H

#include <SDL2/SDL.h>
#include "type.hpp"
#include "texture.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "physic.hpp"

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
  bool on_ground; // Collision cache
  UpgradeKind kind;
  Direction dir;
  Physic physic;

  void init(float x, float y, Direction dir, UpgradeKind kind);
  void action(float frame_ratio) override;
  void updatePhysics(float deltaTime);
  void draw() override {}
  void collision(void* p_c_object, int c_object, CollisionType type_);
  std::string type() override { return "Upgrade"; };
};

class Bullet : public GameObject
{
 public:
  bool removable;
  int life_count;

  void init(float x, float y, float xm, Direction dir);
  void action(float frame_ratio) override;
  void updatePhysics(float deltaTime);
  void draw() override {}
  void collision(int c_object);
  std::string type() override { return "Bullet"; };
};

#endif /*SUPERTUX_SPECIAL_H*/

// EOF
