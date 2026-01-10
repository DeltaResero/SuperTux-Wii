// src/gameobjs.h
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_GAMEOBJS_H
#define SUPERTUX_GAMEOBJS_H

#include "type.hpp"
#include "texture.hpp"
#include "timer.hpp"
#include "scene.hpp"

/* Bounciness of distros: */
#define NO_BOUNCE 0
#define BOUNCE 1

class RenderBatcher;

class BouncyDistro: public GameObject
{
  public:
    bool removable;

    void init(float x, float y);
    void action(float frame_ratio);
    void draw() override {}
    std::string type()
    {
      return "BouncyDistro";
    };
};

extern Surface* img_distro[4];

#define BOUNCY_BRICK_MAX_OFFSET 8
#define BOUNCY_BRICK_SPEED 0.9

class Tile;

class BrokenBrick: public GameObject
{
  public:
    bool removable;
    Timer timer;
    Tile* tile;

    // Cached random values to avoid repeated rand() calls in draw
    int random_offset_x;
    int random_offset_y;

    void init(Tile* tile, float x, float y, float xm, float ym);
    void action(float frame_ratio);
    void draw() override {}
    std::string type()
    {
      return "BrokenBrick";
    };
};

class BouncyBrick: public GameObject
{
  public:
    bool removable;
    float offset;
    float offset_m;
    int shape;

    void init(float x, float y);
    void action(float frame_ratio);
    void draw() override;
    void draw(RenderBatcher* batcher);
    std::string type()
    {
      return "BouncyBrick";
    };
};

class FloatingScore: public GameObject
{
  public:
    bool removable;
    int value;
    Timer timer;

    void init(float x, float y, int s);
    void action(float frame_ratio);
    void draw() override {}
    std::string type()
    {
      return "FloatingScore";
    };
};

#endif /*SUPERTUX_GAMEOBJS_H*/

// EOF
