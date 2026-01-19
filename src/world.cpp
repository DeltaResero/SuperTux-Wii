// src/world.cpp
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

#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <string_view>
#include "globals.hpp"
#include "scene.hpp"
#include "screen.hpp"
#include "defines.hpp"
#include "world.hpp"
#include "level.hpp"
#include "tile.hpp"
#include "resources.hpp"
#include "render_batcher.hpp"
#include "text.hpp"
#include "sound.hpp"
#include "player.hpp"
#include "collision.hpp"
#include "texture.hpp"

// Extern sprite declarations needed for monolithic draw loops
extern Sprite* img_bullet;
extern Sprite* img_star;
extern Sprite* img_growup;
extern Sprite* img_iceflower;
extern Sprite* img_1up;
extern Surface* img_distro[4];

// Extern declaration for bumpbrick if it is not in a header
extern void bumpbrick(float x, float y);

namespace {
  // the space that it takes for the screen to start scrolling, regarding
  // screen bounds (in pixels)
  constexpr int X_SPACE = 400 - 16;
  // the time it takes to move the camera (in ms)
  constexpr int CHANGE_DIR_SCROLL_SPEED = 2000;
}

World* World::current_ = 0;

void World::common_setup()
{
  tux.init();
  set_defaults();
  get_level()->load_gfx();
  activate_bad_guys();
  activate_particle_systems();
  get_level()->load_song();
  apply_bonuses();
  scrolling_timer.init(true);
  m_renderBatcher = new RenderBatcher();
  m_spatial_grid = new SpatialGrid(128);
}

World::World(std::string_view filename)
  // Initialize all object pools with their fixed sizes.
  // This pre-allocates all memory needed for these objects upfront.
  : bouncy_distros(32),
    broken_bricks(64),
    floating_scores(32),
    bullets(8),
    upgrades(16),
    m_spatial_grid(nullptr)
{
  // FIXME: Move this to action and draw and everywhere else where the
  // world calls child functions
  current_ = this;
  level = new Level(filename);
  common_setup();
}

World::World(std::string_view subset, int level_nr)
  // Initialize all object pools with their fixed sizes.
  : bouncy_distros(32),
    broken_bricks(64),
    floating_scores(32),
    bullets(8),
    upgrades(16),
    m_spatial_grid(nullptr)
{
  // FIXME: Move this to action and draw and everywhere else where the
  // world calls child functions
  current_ = this;
  level = new Level(subset, level_nr);
  common_setup();
}

void World::apply_bonuses()
{
  // Apply bonuses from former levels
  switch (player_status.bonus)
  {
    case PlayerStatus::NO_BONUS:
      break;

    case PlayerStatus::FLOWER_BONUS:
      tux.got_coffee = true;
      // fall through

    case PlayerStatus::GROWUP_BONUS:
      // FIXME: Move this to Player class
      tux.size = BIG;
      tux.base.height = TILE_SIZE * 2;
      tux.base.y -= TILE_SIZE;
      break;
  }
}

World::~World()
{
  deactivate_world();
  delete level;
  delete m_renderBatcher;
  delete m_spatial_grid;
}

void World::activate_world()
{
  set_defaults();
  activate_bad_guys();
  activate_particle_systems();
}

void World::deactivate_world()
{
  for (auto* badguy : bad_guys)
  {
    delete badguy;
  }
  bad_guys.clear();
  normal_colliders.clear();
  special_colliders.clear();

  for (ParticleSystems::iterator i = particle_systems.begin();
          i != particle_systems.end(); ++i)
    delete *i;
  particle_systems.clear();

  // Reset all object pools, returning all objects to the free list.
  bouncy_distros.clear();
  broken_bricks.clear();
  floating_scores.clear();
  bullets.clear();
  upgrades.clear();

  for (std::vector<BouncyBrick*>::iterator i = bouncy_bricks.begin();
       i != bouncy_bricks.end(); ++i)
    delete *i;
  bouncy_bricks.clear();
}

void World::set_defaults()
{
  // Set defaults:
  scroll_x = 0;
  player_status.score_multiplier = 1;
  counting_distros = false;
  distro_counter = 0;
  /* set current song/music */
  currentmusic = NO_MUSIC;
}

void World::activate_bad_guys()
{
  for (std::vector<BadGuyData>::iterator i = level->badguy_data.begin();
       i != level->badguy_data.end();
       ++i)
  {
#ifdef DEBUG
     printf("add bad guy %d\n", i->kind);
#endif
     add_bad_guy(i->x, i->y, i->kind, i->stay_on_platform);
  }
}

void World::activate_particle_systems()
{
  if (level->particle_system == "clouds")
  {
    particle_systems.push_back(new CloudParticleSystem);
  }
  else if (level->particle_system == "snow")
  {
    particle_systems.push_back(new SnowParticleSystem);
  }
  else if (level->particle_system != "")
  {
    st_abort("unknown particle system specified in level", "");
  }
}

/**
 * Helper function to draw a layer of tiles.
 * This consolidates the repetitive loop logic used for drawing the background,
 * interactive, and foreground tile layers.
 */
void World::draw_tile_layer(RenderBatcher* batcher, const unsigned int* tile_data, bool is_interactive_layer)
{
  const int current_width = level->width;
  // Calculate the first tile index and subtract 12 to create a wide buffer for large objects.
  const int first_tile_x = static_cast<int>(floorf(scroll_x / (float)TILE_SIZE)) - 12;

  for (int y = 0; y < SCREEN_HEIGHT_TILES; ++y)
  {
    // Loop 34 times to cover the screen plus the wide buffer.
    for (int x = 0; x < 34; ++x)
    {
      int map_tile_x = first_tile_x + x;
      if (map_tile_x >= 0 && map_tile_x < current_width)
      {
        bool should_draw_tile = true;

        // Special check for the interactive layer to avoid drawing a static
        // tile where a bouncy brick is currently active.
        if (is_interactive_layer)
        {
          for (const auto* brick : bouncy_bricks)
          {
            if (static_cast<int>(brick->base.x) / TILE_SIZE == map_tile_x &&
                static_cast<int>(brick->base.y) / TILE_SIZE == y)
            {
              should_draw_tile = false;
              break;
            }
          }
        }

        if (should_draw_tile)
        {
          float tile_world_x = map_tile_x * (float)TILE_SIZE;
          unsigned int tile_id = tile_data[y * current_width + map_tile_x];
          Tile::draw(batcher, tile_world_x - scroll_x, y * (float)TILE_SIZE, tile_id);
        }
      }
    }
  }
}

void World::draw()
{
  /* Draw the real background */
  level->draw_bg();

  /* Draw particle systems (background) */
  for(auto* p : particle_systems)
  {
    p->draw(scroll_x, 0, 0);
  }

  // Single unified rendering loop - works for both SDL and OpenGL!
  RenderBatcher* batcher = use_gl ? m_renderBatcher : nullptr;

  /* Draw background, interactive, and foreground tiles using the helper */
  draw_tile_layer(batcher, level->bg_tiles.data());
  draw_tile_layer(batcher, level->ia_tiles.data(), true);

#ifndef NOOPENGL
  if (use_gl)
  {
    m_renderBatcher->flush();
  }
#endif

  for (unsigned int i = 0; i < bouncy_bricks.size(); ++i)
  {
    bouncy_bricks[i]->draw(batcher);
  }

  for (auto* badguy : bad_guys)
  {
    badguy->draw(batcher);
  }

  tux.draw(batcher);

  // Draw Bullets
  draw_pooled_objects(bullets, [&](const Bullet& bullet) {
    if (bullet.base.x >= scroll_x - bullet.base.width && bullet.base.x <= scroll_x + screen->w)
    {
      if (batcher)
      {
        img_bullet->draw(*batcher, bullet.base.x, bullet.base.y);
      }
      else
      {
        img_bullet->draw(bullet.base.x, bullet.base.y);
      }
    }
  });

  // Draw Upgrades
  draw_pooled_objects(upgrades, [&](const Upgrade& upgrade) {
    Sprite* sprite_to_draw = nullptr;
    if (upgrade.kind == UPGRADE_GROWUP)
    {
      sprite_to_draw = img_growup;
    }
    else if (upgrade.kind == UPGRADE_ICEFLOWER)
    {
      sprite_to_draw = img_iceflower;
    }
    else if (upgrade.kind == UPGRADE_HERRING)
    {
      sprite_to_draw = img_star;
    }
    else if (upgrade.kind == UPGRADE_1UP)
    {
      sprite_to_draw = img_1up;
    }

    if (sprite_to_draw)
    {
      if (upgrade.base.height < TILE_SIZE)
      {
        if (batcher)
        {
          sprite_to_draw->draw_part(*batcher, 0, 0, upgrade.base.x, upgrade.base.y + TILE_SIZE - upgrade.base.height, TILE_SIZE, upgrade.base.height);
        }
        else
        {
          sprite_to_draw->draw_part(0, 0, upgrade.base.x - scroll_x, upgrade.base.y + TILE_SIZE - upgrade.base.height, TILE_SIZE, upgrade.base.height);
        }
      }
      else
      {
        if (batcher)
        {
          sprite_to_draw->draw(*batcher, upgrade.base.x, upgrade.base.y);
        }
        else
        {
          sprite_to_draw->draw(upgrade.base.x, upgrade.base.y);
        }
      }
    }
  });

  // Draw Bouncy Distros
  draw_pooled_objects(bouncy_distros, [&](const BouncyDistro& distro) {
    if (batcher)
    {
      batcher->add(img_distro[0], distro.base.x, distro.base.y, 0, 0);
    }
    else
    {
      img_distro[0]->draw(distro.base.x - scroll_x, distro.base.y);
    }
  });

  // Draw Broken Bricks
  draw_pooled_objects(broken_bricks, [&](const BrokenBrick& brick) {
    if (!brick.tile->images.empty())
    {
      if (batcher)
      {
        batcher->add_part(brick.tile->images[0], brick.random_offset_x, brick.random_offset_y,
                          brick.base.x, brick.base.y, 16, 16, 0, 0);
      }
      else
      {
        brick.tile->images[0]->draw_part(brick.random_offset_x, brick.random_offset_y,
                                          brick.base.x - scroll_x, brick.base.y, 16, 16);
      }
    }
  });

#ifndef NOOPENGL
  if (use_gl)
  {
    m_renderBatcher->flush();
  }
#endif

  // Draw Floating Scores (Text-based, drawn AFTER flush)
  draw_pooled_objects(floating_scores, [&](const FloatingScore& score) {
    std::string score_str = std::to_string(score.value);
    int x_pos = static_cast<int>(score.base.x - scroll_x + 16 - score_str.length() * 8);
    gold_text->draw(score_str, x_pos, static_cast<int>(score.base.y), 1);
  });

  /* Draw foreground tiles: */
  draw_tile_layer(batcher, level->fg_tiles.data());

#ifndef NOOPENGL
  if (use_gl)
  {
    m_renderBatcher->flush();
  }
#endif

  /* Draw particle systems (foreground) */
  for(auto* p : particle_systems)
  {
    p->draw(scroll_x, 0, 1);
  }

#ifndef NOOPENGL
  if (use_gl)
  {
    SurfaceOpenGL::reset_state();
  }
#endif
}

void World::resolvePlayerPhysics(Player* player)
{
    if (player->dying != DYING_NOT) return;

    bool jumped_in_solid = false;

    // If collision stopped horizontal movement, kill horizontal velocity
    if (player->post_physics_base.x != player->base.x)
    {
      player->physic.set_velocity_x(0);
    }

    // Exception for when Tux is stuck under a tile while unducking
    if (!player->duck && player->on_ground() && player->old_base.x == player->base.x && player->old_base.y == player->base.y && collision_object_map(player->base))
    {
      player->base.x += m_elapsed_time * WALK_SPEED * (player->dir ? 1 : -1);
      player->previous_base = player->old_base = player->base;
    }

    // Handle gravity and landing logic
    if (!player->on_ground())
    {
      // If we are in the air, gravity should be active.
      player->physic.enable_gravity(true);
      if (player->under_solid())
      {
        // If we hit a ceiling, stop all upward velocity.
        player->physic.set_velocity_y(0);
        jumped_in_solid = true; // Flag that we hit a ceiling this frame.
      }
    }
    else
    {
      // If the player was previously falling (positive y-velocity in our coordinate system)
      // and is now on the ground, it means they have just landed.
      if (player->physic.get_velocity_y() > 0)
      {
        // Snap the player's FEET to the top of the tile grid to ensure they are perfectly aligned.
        // This prevents jittering and falling through thin platforms.
        player->base.y = (int)((player->base.y + player->base.height) / TILE_SIZE) * TILE_SIZE - player->base.height;
        player->physic.set_velocity_y(0);
      }

      // Since we are on the ground, gravity should be disabled to prevent accumulating downward velocity.
      player->physic.enable_gravity(false);
      player_status.score_multiplier = 1; // Reset score multiplier (for multi-hits)
    }

    // Handle interactions when bonking a block from below
    if (jumped_in_solid)
    {
      if (isbrick(player->base.x, player->base.y) || isfullbox(player->base.x, player->base.y))
      {
        trygrabdistro(player->base.x, player->base.y - TILE_SIZE, BOUNCE);
        trybumpbadguy(player->base.x, player->base.y - (TILE_SIZE * 2));
        trybreakbrick(player->base.x, player->base.y, player->size == SMALL, RIGHT);
        bumpbrick(player->base.x, player->base.y);
        tryemptybox(player->base.x, player->base.y, RIGHT);
      }
      if (isbrick(player->base.x + (TILE_SIZE - 1), player->base.y) || isfullbox(player->base.x + (TILE_SIZE - 1), player->base.y))
      {
        trygrabdistro(player->base.x + (TILE_SIZE - 1), player->base.y - TILE_SIZE, BOUNCE);
        trybumpbadguy(player->base.x + (TILE_SIZE - 1), player->base.y - (TILE_SIZE * 2));
        if (player->size == BIG) { trybreakbrick(player->base.x + (TILE_SIZE - 1), player->base.y, player->size == SMALL, LEFT); }
        bumpbrick(player->base.x + (TILE_SIZE - 1), player->base.y);
        tryemptybox(player->base.x + (TILE_SIZE - 1), player->base.y, LEFT);
      }
    }
    player->grabdistros();

    // Make sure Tux doesn't try to stick to solid roofs
    if (jumped_in_solid)
    {
      ++player->base.y;
      ++player->old_base.y;
      if (player->on_ground()) { player->jumping = false; }
    }
}

/**
 * Helper function to clean up all game objects that are marked for removal.
 * This includes BouncyBricks and all BadGuys from their various lists.
 * This uses the efficient swap-and-pop idiom to remove elements.
 */
void World::cleanup_dead_objects()
{
  // Clean up removable non-pooled objects
  for (size_t i = 0; i < bouncy_bricks.size(); )
  {
    if (bouncy_bricks[i]->removable)
    {
      delete bouncy_bricks[i];
      bouncy_bricks[i] = bouncy_bricks.back();
      bouncy_bricks.pop_back();
    }
    else
    {
      ++i;
    }
  }

  // Use the same safe backward loop for bad_guys.
  for (size_t i = 0; i < bad_guys.size(); )
  {
    if (bad_guys[i]->is_removable())
    {
      delete bad_guys[i];
      bad_guys[i] = bad_guys.back();
      bad_guys.pop_back();
    }
    else
    {
      ++i;
    }
  }

  // Also clean up the collision lists.
  // We don't need to delete the objects here, just remove the pointers.
  for (size_t i = 0; i < normal_colliders.size(); )
  {
    if (normal_colliders[i]->is_removable())
    {
      normal_colliders[i] = normal_colliders.back();
      normal_colliders.pop_back();
    }
    else
    {
      ++i;
    }
  }

  for (size_t i = 0; i < special_colliders.size(); )
  {
    if (special_colliders[i]->is_removable())
    {
      special_colliders[i] = special_colliders.back();
      special_colliders.pop_back();
    }
    else
    {
      ++i;
    }
  }
}

void World::action(float elapsed_time)
{
  m_elapsed_time = elapsed_time;

  // Update tile animations once per frame
  TileManager::instance()->update_animations(global_frame_counter);

  // Update all game logic
  tux.action(elapsed_time);
  tux.updatePhysics(elapsed_time);
  resolvePlayerPhysics(&tux);

  tux.check_bounds(level->back_scrolling, (bool)level->hor_autoscroll_speed);
  scrolling(elapsed_time);

  // Update all pooled objects with automatic cleanup
  bouncy_distros.updateAndCleanup(elapsed_time);
  broken_bricks.updateAndCleanup(elapsed_time);
  floating_scores.updateAndCleanup(elapsed_time);
  bullets.updateAndCleanup(elapsed_time);
  upgrades.updateAndCleanup(elapsed_time);

  // Update other game objects
  for (auto* brick : bouncy_bricks)
  {
    brick->action(elapsed_time);
  }

  for (auto* badguy : bad_guys)
  {
    badguy->action(elapsed_time);
  }

  for(auto* p : particle_systems)
  {
    p->simulate(elapsed_time);
  }

  // Handle all collisions
  collision_handler();
  // Clean up all objects marked for removal
  cleanup_dead_objects();
}

/**
 * This function smoothly scrolls the camera to keep the player in view.
 * It calculates a target position for the camera based on the original game's
 * rules and then smoothly moves the current scroll position towards that target
 * using a frame-rate independent exponential smoothing filter. This provides
 * a responsive feel without the jitter of the original implementation.
 */
void World::scrolling(float elapsed_time)
{
  if (level->hor_autoscroll_speed)
  {
    // Auto-scroll horizontally based on level configuration
    scroll_x += level->hor_autoscroll_speed * elapsed_time;
    return;
  }

  // Determine the camera's target position based on a central "dead zone".
  float target_scroll_x = scroll_x;
  const float tux_pos_x = (tux.base.x + (tux.base.width / 2));
  if (tux_pos_x > scroll_x + screen->w - X_SPACE)
  {
    target_scroll_x = tux_pos_x - (screen->w - X_SPACE);
  }
  else if (tux_pos_x < scroll_x + X_SPACE && (level->back_scrolling || debug_mode))
  {
    target_scroll_x = tux_pos_x - X_SPACE;
  }

  // Smoothly move the camera towards the target.
  const float camera_stiffness = 15.0f;
  const float blend_factor = 1.0f - expf(-camera_stiffness * elapsed_time);
  scroll_x += (target_scroll_x - scroll_x) * blend_factor;

  // Prevent scrolling before the start or after the level's end
  if (scroll_x < 0)
  {
    scroll_x = 0;
  }
  if (scroll_x > level->width * TILE_SIZE - screen->w)
  {
    scroll_x = level->width * TILE_SIZE - screen->w;
  }
}

void World::collision_handler()
{
  // Rebuild spatial grid each frame
  m_spatial_grid->clear();

  // Add all active entities to grid
  for (auto* badguy : bad_guys)
  {
    if (badguy->dying == DYING_NOT)
    {
      m_spatial_grid->add_badguy(badguy);
    }
  }

  for (size_t index : bullets.get_active_indices())
  {
    Bullet* bullet = bullets.get_object_at(index);
    if (bullet && !bullet->removable)
    {
      m_spatial_grid->add_bullet(bullet);
    }
  }

  for (size_t index : upgrades.get_active_indices())
  {
    Upgrade* upgrade = upgrades.get_object_at(index);
    if (upgrade)
    {
      m_spatial_grid->add_upgrade(upgrade);
    }
  }

  // Bullet vs BadGuy collisions
  for (size_t index : bullets.get_active_indices())
  {
    Bullet* bullet = bullets.get_object_at(index);
    if (bullet->removable) continue;

    // Query badguys near bullet (bounded query)
    auto nearby_badguys = m_spatial_grid->query_badguys(
      bullet->base.x - TILE_SIZE, bullet->base.y - TILE_SIZE,
      bullet->base.width + (TILE_SIZE * 2), bullet->base.height + (TILE_SIZE * 2)
    );

    for (auto* badguy : nearby_badguys)
    {
      if (badguy->dying != DYING_NOT) continue;

      if (rectcollision(bullet->base, badguy->base))
      {
        badguy->collision(0, CO_BULLET);
        bullet->collision(CO_BADGUY);
        break; // A bullet can only hit one enemy.
      }
    }
  }

  // Special colliders (Mr. Iceblock case)
  for (auto* special : special_colliders)
  {
    if (special->dying != DYING_NOT) continue;

    // Mr. Iceblock in KICK mode can hit enemies OFF SCREEN
    // Use unbounded query (all badguys) to preserve original behavior
    bool is_kicked_iceblock = (special->kind == BAD_MRICEBLOCK &&
                               special->mode == BadGuy::KICK);

    if (is_kicked_iceblock)
    {
      // Check against ALL badguys (original behavior)
      for (auto* normal : normal_colliders)
      {
        if (normal->dying != DYING_NOT) continue;

        if (rectcollision(special->base, normal->base))
        {
          normal->collision(special, CO_BADGUY);
          special->collision(normal, CO_BADGUY);
        }
      }
    }
    else
    {
      // Other special colliders use spatial query
      auto nearby = m_spatial_grid->query_badguys(
        special->base.x - TILE_SIZE, special->base.y - TILE_SIZE,
        special->base.width + (TILE_SIZE * 2), special->base.height + (TILE_SIZE * 2)
      );

      for (auto* normal : nearby)
      {
        if (normal->dying != DYING_NOT) continue;

        if (rectcollision(special->base, normal->base))
        {
          normal->collision(special, CO_BADGUY);
          special->collision(normal, CO_BADGUY);
        }
      }
    }
  }

  // Special vs Special collisions
  for (size_t i = 0; i < special_colliders.size(); ++i)
  {
    BadGuy* special1 = special_colliders[i];
    if (special1->dying != DYING_NOT) continue;

    for (size_t j = i + 1; j < special_colliders.size(); ++j)
    {
      BadGuy* special2 = special_colliders[j];
      if (special2->dying != DYING_NOT) continue;

      if (rectcollision(special1->base, special2->base))
      {
        special2->collision(special1, CO_BADGUY);
        special1->collision(special2, CO_BADGUY);
      }
    }
  }

  // Normal vs Normal (use spatial grid)
  const float screen_x_start = scroll_x - (float)(TILE_SIZE * 2);
  const float screen_x_end = scroll_x + screen->w + (float)(TILE_SIZE * 2);

  for (size_t i = 0; i < normal_colliders.size(); ++i)
  {
    if (normal_colliders[i]->dying != DYING_NOT) continue;
    if (normal_colliders[i]->base.x + normal_colliders[i]->base.width < screen_x_start ||
        normal_colliders[i]->base.x > screen_x_end) continue;

    auto nearby = m_spatial_grid->query_badguys(
      normal_colliders[i]->base.x - TILE_SIZE,
      normal_colliders[i]->base.y - TILE_SIZE,
      normal_colliders[i]->base.width + (TILE_SIZE * 2),
      normal_colliders[i]->base.height + (TILE_SIZE * 2)
    );

    for (auto* other : nearby)
    {
      if (other == normal_colliders[i]) continue;
      if (other->dying != DYING_NOT) continue;

      if (rectcollision(normal_colliders[i]->base, other->base))
      {
        other->collision(normal_colliders[i], CO_BADGUY);
        normal_colliders[i]->collision(other, CO_BADGUY);
      }
    }
  }

  // Player collisions
  if (tux.dying != DYING_NOT) return;

  for (auto* badguy : bad_guys)
  {
    if (badguy->dying != DYING_NOT) continue;

    if (badguy->base.x + badguy->base.width < tux.base.x - (float)(TILE_SIZE * 2) ||
        badguy->base.x > tux.base.x + tux.base.width + (float)(TILE_SIZE * 2))
      continue;

    if (rectcollision_offset(badguy->base, tux.base, 0, 0))
    {
      if (tux.previous_base.y < tux.base.y &&
          tux.previous_base.y + tux.previous_base.height < badguy->base.y + badguy->base.height/2 &&
          !tux.invincible_timer.started())
      {
        badguy->collision(&tux, CO_PLAYER, COLLISION_SQUISH);
      }
      else
      {
        tux.collision(badguy, CO_BADGUY);
        badguy->collision(&tux, CO_PLAYER, COLLISION_NORMAL);
      }
    }
  }

  // Upgrade collisions
  for (size_t index : upgrades.get_active_indices())
  {
    // Upgrades can only be collected by the player, so we only need to check for them near the player
    // Skip any upgrade that is not within a 64-pixel buffer around Tux
    Upgrade* upgrade = upgrades.get_object_at(index);

    if (upgrade->base.x + upgrade->base.width < tux.base.x - (float)(TILE_SIZE * 2) ||
        upgrade->base.x > tux.base.x + tux.base.width + (float)(TILE_SIZE * 2))
    {
      continue;
    }

    if (rectcollision(upgrade->base, tux.base))
    {
      upgrade->collision(&tux, CO_PLAYER, COLLISION_NORMAL);
    }
  }
}

void World::add_score(float x, float y, int s)
{
  player_status.score += s;

  FloatingScore* score = floating_scores.acquire();
  if (score)
  {
    score->init(x, y, s);
  }
}

void World::add_bouncy_distro(float x, float y)
{
  BouncyDistro* distro = bouncy_distros.acquire();
  if (distro)
  {
    distro->init(x, y);
  }
}

void World::add_broken_brick(Tile* tile, float x, float y)
{
  add_broken_brick_piece(tile, x, y, -1, -4);
  add_broken_brick_piece(tile, x, y + (TILE_SIZE / 2), -1.5, -3);

  add_broken_brick_piece(tile, x + (TILE_SIZE / 2), y, 1, -4);
  add_broken_brick_piece(tile, x + (TILE_SIZE / 2), y + (TILE_SIZE / 2), 1.5, -3);
}

void World::add_broken_brick_piece(Tile* tile, float x, float y, float xm, float ym)
{
  BrokenBrick* brick = broken_bricks.acquire();
  if (brick)
  {
    brick->init(tile, x, y, xm, ym);
  }
}

void World::add_bouncy_brick(float x, float y)
{
  BouncyBrick* new_bouncy_brick = new BouncyBrick();
  new_bouncy_brick->init(x,y);
  bouncy_bricks.push_back(new_bouncy_brick);
}

BadGuy*
World::add_bad_guy(float x, float y, BadGuyKind kind, bool stay_on_platform)
{
  BadGuy* badguy = new BadGuy(x,y,kind, stay_on_platform);
  bad_guys.push_back(badguy);
  normal_colliders.push_back(badguy); // All bad guys start as normal colliders.
  return badguy;
}

void World::add_upgrade(float x, float y, Direction dir, UpgradeKind kind)
{
  Upgrade* new_upgrade = upgrades.acquire();
  if (new_upgrade)
  {
    new_upgrade->init(x, y, dir, kind);
  }
}

void World::add_bullet(float x, float y, float xm, Direction dir)
{
  // Check the number of *active* bullets.
  if (bullets.get_active_indices().size() >= MAX_BULLETS)
  {
    return;
  }

  Bullet* new_bullet = bullets.acquire();
  if (new_bullet)
  {
    new_bullet->init(x, y, xm, dir);
    play_sound(sounds[SND_SHOOT], SOUND_CENTER_SPEAKER);
  }
}

// Moves a bad guy between the normal and special collision lists.
void World::set_badguy_collision_state(BadGuy* bg, bool is_special)
{
  if (is_special)
  {
    // Move from normal to special
    auto it = std::find(normal_colliders.begin(), normal_colliders.end(), bg);
    if (it != normal_colliders.end())
    {
      normal_colliders.erase(it);
      special_colliders.push_back(bg);
    }
  }
  else
  {
    // Move from special to normal
    auto it = std::find(special_colliders.begin(), special_colliders.end(), bg);
    if (it != special_colliders.end())
    {
      special_colliders.erase(it);
      normal_colliders.push_back(bg);
    }
  }
}

void World::play_music(int musictype)
{
  // If the level is ending, or we're already playing the requested music, then do nothing.
  if (currentmusic == musictype || currentmusic == LEVEL_END_MUSIC)
  {
    return;
  }

  currentmusic = musictype;

  switch(currentmusic)
  {
    case HURRYUP_MUSIC:
      music_manager->play_music(get_level()->get_level_music_fast());
      break;
    case LEVEL_MUSIC:
      music_manager->play_music(get_level()->get_level_music());
      break;
    case HERRING_MUSIC:
      music_manager->play_music(herring_song);
      break;
    case LEVEL_END_MUSIC:
      music_manager->play_music(level_end_song, 0);
      break;
    default:
      music_manager->halt_music();
      currentmusic = NO_MUSIC; // Reset state when halting
      break;
  }
}

int World::get_music_type() const
{
  return currentmusic;
}

/* Break a brick: */
void World::trybreakbrick(float x, float y, bool small, Direction col_side)
{
  Level* plevel = get_level();

  Tile* tile = gettile(x, y);
  if (tile->brick)
  {
    if (tile->data > 0)
    {
      /* Get a distro from it: */
      add_bouncy_distro(((int)(x + 1) / TILE_SIZE) * TILE_SIZE,
                         (int)(y / TILE_SIZE) * TILE_SIZE);

      // TODO: don't handle this in a global way but per-tile...
      if (!counting_distros)
      {
        counting_distros = true;
        distro_counter = 5;
      }
      else
      {
        distro_counter--;
      }

      if (distro_counter <= 0)
      {
        counting_distros = false;
        plevel->change(x, y, TM_IA, tile->next_tile);
      }

      play_sound(sounds[SND_DISTRO], SOUND_CENTER_SPEAKER);
      player_status.score = player_status.score + SCORE_DISTRO;
      player_status.distros++;
    }

    else if (!small)
    {
      /* Get rid of it: */
      plevel->change(x, y, TM_IA, tile->next_tile);

      /* Replace it with broken bits: */
      add_broken_brick(tile, ((int)(x + 1) / TILE_SIZE) * TILE_SIZE,
                              (int)(y / TILE_SIZE) * TILE_SIZE);

      /* Get some score: */
      play_sound(sounds[SND_BRICK], SOUND_CENTER_SPEAKER);
      player_status.score = player_status.score + SCORE_BRICK;
    }
  }

  else if(tile->fullbox)
  {
    tryemptybox(x, y, col_side);
  }
}

/* Empty a box: */
void World::tryemptybox(float x, float y, Direction col_side)
{
  Tile* tile = gettile(x,y);
  if (!tile->fullbox)
    return;

  // according to the collision side, set the upgrade direction
  if(col_side == LEFT)
  {
    col_side = RIGHT;
  }
  else
  {
    col_side = LEFT;
  }

  int posx = ((int)(x+1) / TILE_SIZE) * TILE_SIZE;
  int posy = (int)(y/TILE_SIZE) * TILE_SIZE - TILE_SIZE;
  switch(tile->data)
  {
    case 1: // Box with a distro!
      add_bouncy_distro(posx, posy);
      play_sound(sounds[SND_DISTRO], SOUND_CENTER_SPEAKER);
      player_status.score = player_status.score + SCORE_DISTRO;
      player_status.distros++;
      break;

    case 2: // Add an upgrade!
      if (tux.size == SMALL)     /* Tux is small, add mints! */
        add_upgrade(posx, posy, col_side, UPGRADE_GROWUP);
      else     /* Tux is big, add an iceflower: */
        add_upgrade(posx, posy, col_side, UPGRADE_ICEFLOWER);
      play_sound(sounds[SND_UPGRADE], SOUND_CENTER_SPEAKER);
      break;

    case 3: // Add a golden herring
      add_upgrade(posx, posy, col_side, UPGRADE_HERRING);
      break;

    case 4: // Add a 1up extra
      add_upgrade(posx, posy, col_side, UPGRADE_1UP);
      break;
    default:
      break;
  }

  /* Empty the box: */
  level->change(x, y, TM_IA, tile->next_tile);
}

/* Try to grab a distro: */
void World::trygrabdistro(float x, float y, int bounciness)
{
  Tile* tile = gettile(x, y);
  if (tile && tile->distro)
  {
    level->change(x, y, TM_IA, tile->next_tile);
    play_sound(sounds[SND_DISTRO], SOUND_CENTER_SPEAKER);

    if (bounciness == BOUNCE)
    {
      add_bouncy_distro(((int)(x + 1) / TILE_SIZE) * TILE_SIZE,
                         (int)(y / TILE_SIZE) * TILE_SIZE);
    }

    player_status.score = player_status.score + SCORE_DISTRO;
    player_status.distros++;
  }
}

/* Try to bump a bad guy from below: */
void World::trybumpbadguy(float x, float y)
{
  // Check against normal colliders
  for (auto* badguy : normal_colliders)
  {
    if (badguy->base.x >= x - TILE_SIZE && badguy->base.x <= x + TILE_SIZE &&
        badguy->base.y >= y - (TILE_SIZE / 2) && badguy->base.y <= y + (TILE_SIZE / 2))
    {
      badguy->collision(&tux, CO_PLAYER, COLLISION_BUMP);
    }
  }

  // Check against special colliders as well
  for (auto* badguy : special_colliders)
  {
    if (badguy->base.x >= x - TILE_SIZE && badguy->base.x <= x + TILE_SIZE &&
        badguy->base.y >= y - (TILE_SIZE / 2) && badguy->base.y <= y + (TILE_SIZE / 2))
    {
      badguy->collision(&tux, CO_PLAYER, COLLISION_BUMP);
    }
  }

  // Upgrades:
  for (size_t index : upgrades.get_active_indices())
  {
    Upgrade* upgrade = upgrades.get_object_at(index);
    if (upgrade->base.height == TILE_SIZE &&
        upgrade->base.x >= x - TILE_SIZE && upgrade->base.x <= x + TILE_SIZE &&
        upgrade->base.y >= y - (TILE_SIZE / 2) && upgrade->base.y <= y + (TILE_SIZE / 2))
    {
      upgrade->collision(&tux, CO_PLAYER, COLLISION_BUMP);
    }
  }
}

// EOF
