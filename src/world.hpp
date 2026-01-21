// src/world.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_WORLD_H
#define SUPERTUX_WORLD_H

#include <vector>
#include <string>
#include <string_view>
#include <bitset>
#include <SDL2/SDL.h>
#include "type.hpp"
#include "scene.hpp"
#include "special.hpp"
#include "badguy.hpp"
#include "particlesystem.hpp"
#include "gameobjs.hpp"
#include "object_pool.hpp"
#include "spatial_grid.hpp"
#include "defines.hpp" // Added for pool size constants

class Level;
class RenderBatcher;
class Player;

/** The World class holds a level and all the game objects (badguys,
    bouncy distros, etc) that are needed to run a game. */
class World
{
private:
  typedef std::vector<BadGuy*> BadGuys;
  std::vector<BadGuy*> bad_guys_to_add;
  Level* level;
  Player tux;

  Timer scrolling_timer;

  int distro_counter;
  bool counting_distros;
  int currentmusic;

  static World* current_;
  RenderBatcher* m_renderBatcher;

  float m_elapsed_time;

  void common_setup();
  void resolvePlayerPhysics(Player* player);

  // Helper function to draw a layer of tiles (bg, ia, or fg)
  void draw_tile_layer(RenderBatcher* batcher, const unsigned int* tile_data, bool is_interactive_layer = false);

  // Helper function to clean up all objects marked for removal
  void cleanup_dead_objects();

  template<typename T, typename Func>
  void draw_pooled_objects(ObjectPool<T>& pool, Func draw_lambda)
  {
    const auto& pool_data = pool.get_pool_data();
    for (size_t index : pool.get_active_indices())
    {
      draw_lambda(pool_data[index]);
    }
  }

public:
  BadGuys bad_guys;
  BadGuys normal_colliders;
  BadGuys special_colliders;

  ObjectPool<BouncyDistro> bouncy_distros{POOL_MAX_BOUNCY_DISTROS};
  ObjectPool<BrokenBrick> broken_bricks{POOL_MAX_BROKEN_BRICKS};
  ObjectPool<FloatingScore> floating_scores{POOL_MAX_FLOATING_SCORES};
  ObjectPool<Bullet> bullets{POOL_MAX_BULLETS};
  ObjectPool<Upgrade> upgrades{POOL_MAX_UPGRADES};

  // BouncyBrick is infrequent and can remain a simple vector for now
  std::vector<BouncyBrick*> bouncy_bricks;

  typedef std::vector<ParticleSystem*> ParticleSystems;
  ParticleSystems particle_systems;

private:
  // Spatial grid is declared last as it's constructed after pools
  SpatialGrid* m_spatial_grid{nullptr};

  // Bitset for on-screen bouncy bricks.
  // The draw loop iterates 34 columns (buffer) * SCREEN_HEIGHT_TILES rows.
  // 34 * 20 (approx max height) = 680 bits = 85 bytes (fits in L1 cache).
  static constexpr int DRAW_BUFFER_WIDTH = 34;
  std::bitset<DRAW_BUFFER_WIDTH * SCREEN_HEIGHT_TILES> m_screen_occupancy;

public:
  static World* current() { return current_; }
  static void set_current(World* w) { current_ = w; }

  World(std::string_view filename);
  World(std::string_view subset, int level_nr);

  World() {};
  ~World();

  void activate_world();
  void deactivate_world();

  Level*  get_level() { return level; }
  Player* get_tux() { return &tux; }

  void set_defaults();

  void draw();
  void action(float frame_ratio);
  void scrolling(float frame_ratio);   // camera scrolling

  void play_music(int musictype);

  // Added const
  int get_music_type() const;


  /** Checks for all possible collisions. And calls the
      collision_handlers, which the collision_objects provide for this
      case (or not). */
  void collision_handler();

  void activate_particle_systems();
  void activate_bad_guys();

  void add_score(float x, float y, int s);
  void add_bouncy_distro(float x, float y);
  void add_broken_brick(Tile* tile, float x, float y);
  void add_broken_brick_piece(Tile* tile, float x, float y, float xm, float ym);
  void add_bouncy_brick(float x, float y);

  BadGuy* add_bad_guy(float x, float y, BadGuyKind kind, bool stay_on_platform = false);

  void add_upgrade(float x, float y, Direction dir, UpgradeKind kind);
  void add_bullet(float x, float y, float xm, Direction dir);

  void set_badguy_collision_state(BadGuy* bg, bool is_special);

  /** Try to grab the coin at the given coordinates */
  void trygrabdistro(float x, float y, int bounciness);

  /** Try to break the brick at the given coordinates */
  void trybreakbrick(float x, float y, bool small, Direction col_side);

  /** Try to get the content out of a bonus box, thus emptying it */
  void tryemptybox(float x, float y, Direction col_side);

  /** Try to bumb a badguy that might we walking above Tux, thus shaking
      the tile which the badguy is walking on an killing him this way */
  void trybumpbadguy(float x, float y);

  /** Apply bonuses active in the player status, used to reactivate
      bonuses from former levels */
  void apply_bonuses();
};

#endif /*SUPERTUX_WORLD_H*/

// EOF
