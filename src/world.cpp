//  world.cpp
//
//  SuperTux
//  Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
//  Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//  02111-1307, USA.

#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include "globals.h"
#include "scene.h"
#include "screen.h"
#include "defines.h"
#include "world.h"
#include "level.h"
#include "tile.h"
#include "resources.h"

Surface* img_distro[4];

World* World::current_ = 0;

World::World(const std::string& filename)
{
  // FIXME: Move this to action and draw and everywhere else where the
  // world calls child functions
  current_ = this;

  level = new Level(filename);
  tux.init();

  set_defaults();

  get_level()->load_gfx();
  activate_bad_guys();
  activate_particle_systems();
  get_level()->load_song();

  apply_bonuses();

  scrolling_timer.init(true);
}

World::World(const std::string& subset, int level_nr)
{
  // FIXME: Move this to action and draw and everywhere else where the
  // world calls child functions
  current_ = this;

  level = new Level(subset, level_nr);
  tux.init();

  set_defaults();

  get_level()->load_gfx();
  activate_world();
  get_level()->load_song();

  apply_bonuses();

  scrolling_timer.init(true);
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
      tux.base.height = 64;
      tux.base.y -= 32;
      break;
    }
}

World::~World()
{
  deactivate_world();

  delete level;
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

  for (std::vector<BouncyDistro*>::iterator i = bouncy_distros.begin();
       i != bouncy_distros.end(); ++i)
    delete *i;
  bouncy_distros.clear();

  for (std::vector<BrokenBrick*>::iterator i = broken_bricks.begin();
       i != broken_bricks.end(); ++i)
    delete *i;
  broken_bricks.clear();

  for (std::vector<BouncyBrick*>::iterator i = bouncy_bricks.begin();
       i != bouncy_bricks.end(); ++i)
    delete *i;
  bouncy_bricks.clear();

  for (std::vector<FloatingScore*>::iterator i = floating_scores.begin();
       i != floating_scores.end(); ++i)
    delete *i;
  floating_scores.clear();

  upgrades.clear();
}

void World::set_defaults()
{
  // Set defaults:
  scroll_x = 0;

  player_status.score_multiplier = 1;

  counting_distros = false;
  distro_counter = 0;

  /* set current song/music */
  currentmusic = LEVEL_MUSIC;
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

void World::draw()
{
  int y,x;

  /* Draw the real background */
  if(level->img_bkgd)
  {
    int s = (int)((float)scroll_x * ((float)level->bkgd_speed/100.0f)) % screen->w;
    level->img_bkgd->draw_part(s, 0,0,0,level->img_bkgd->w - s, level->img_bkgd->h);
    level->img_bkgd->draw_part(0, 0,screen->w - s ,0,s,level->img_bkgd->h);
  }
  else
  {
    drawgradient(level->bkgd_top, level->bkgd_bottom);
  }

  /* Draw particle systems (background) */
  std::vector<ParticleSystem*>::iterator p;
  for(p = particle_systems.begin(); p != particle_systems.end(); ++p)
  {
    (*p)->draw(scroll_x, 0, 0);
  }

  int tile_scroll_x = static_cast<int>(scroll_x) >> 5; // Divide by 32
  float sub_tile_scroll_x = fmodf(scroll_x, 32.0f);
  int current_width = level->width;

  /* Draw background: */
  for (y = 0; y < 15; ++y)
  {
    for (x = 0; x < 21; ++x)
    {
      int tile_x = x + tile_scroll_x;
      if (tile_x < current_width)
      {
        Tile::draw(32 * x - sub_tile_scroll_x, y * 32,
                   level->bg_tiles[y * current_width + tile_x]);
      }
    }
  }

  /* Draw interactive tiles: */
  for (y = 0; y < 15; ++y)
  {
    for (x = 0; x < 21; ++x)
    {
      int tile_x = x + tile_scroll_x;
      if (tile_x < current_width)
      {
        Tile::draw(32 * x - sub_tile_scroll_x, y * 32,
                   level->ia_tiles[y * current_width + tile_x]);
      }
    }
  }

  /* (Bouncy bricks): */
  for (unsigned int i = 0; i < bouncy_bricks.size(); ++i)
  {
    bouncy_bricks[i]->draw();
  }

  for (auto* badguy : bad_guys)
  {
    badguy->draw();
  }

  tux.draw();

  for (unsigned int i = 0; i < bullets.size(); ++i)
  {
    bullets[i].draw();
  }

  for (unsigned int i = 0; i < floating_scores.size(); ++i)
  {
    floating_scores[i]->draw();
  }

  for (unsigned int i = 0; i < upgrades.size(); ++i)
  {
    upgrades[i].draw();
  }

  for (unsigned int i = 0; i < bouncy_distros.size(); ++i)
  {
    bouncy_distros[i]->draw();
  }

  for (unsigned int i = 0; i < broken_bricks.size(); ++i)
  {
    broken_bricks[i]->draw();
  }

  /* Draw foreground: */
  for (y = 0; y < 15; ++y)
  {
    for (x = 0; x < 21; ++x)
    {
      int tile_x = x + tile_scroll_x;
      if (tile_x < current_width)
      {
        Tile::draw(32 * x - sub_tile_scroll_x, y * 32,
                   level->fg_tiles[y * current_width + tile_x]);
      }
    }
  }

  /* Draw particle systems (foreground) */
  for(p = particle_systems.begin(); p != particle_systems.end(); ++p)
  {
    (*p)->draw(scroll_x, 0, 1);
  }
}

void World::action(float elapsed_time)
{
  // Update all game logic
  tux.action(elapsed_time);
  tux.check_bounds(level->back_scrolling, (bool)level->hor_autoscroll_speed);
  scrolling(elapsed_time);

  // Update all game objects using simple forward loops.
  // No removal happens in this phase.
  for (auto* distro : bouncy_distros)
  {
    distro->action(elapsed_time);
  }

  for (auto* brick : broken_bricks)
  {
    brick->action(elapsed_time);
  }

  for (auto* brick : bouncy_bricks)
  {
    brick->action(elapsed_time);
  }

  for (auto* score : floating_scores)
  {
    score->action(elapsed_time);
  }

  for (auto& bullet : bullets)
  {
    bullet.action(elapsed_time);
  }

  for (auto& upgrade : upgrades)
  {
    upgrade.action(elapsed_time);
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

  // Clean up all removable objects
  // All cleanup now happens at the very end of the frame, using safe backward iteration.
  for (int i = bouncy_distros.size() - 1; i >= 0; --i)
  {
    if (bouncy_distros[i]->removable)
    {
      delete bouncy_distros[i];
      bouncy_distros.erase(bouncy_distros.begin() + i);
    }
  }

  for (int i = broken_bricks.size() - 1; i >= 0; --i)
  {
    if (broken_bricks[i]->removable)
    {
      delete broken_bricks[i];
      broken_bricks.erase(broken_bricks.begin() + i);
    }
  }

  for (int i = bouncy_bricks.size() - 1; i >= 0; --i)
  {
    if (bouncy_bricks[i]->removable)
    {
      delete bouncy_bricks[i];
      bouncy_bricks.erase(bouncy_bricks.begin() + i);
    }
  }

  for (int i = floating_scores.size() - 1; i >= 0; --i)
  {
    if (floating_scores[i]->removable)
    {
      delete floating_scores[i];
      floating_scores.erase(floating_scores.begin() + i);
    }
  }

  for (int i = bullets.size() - 1; i >= 0; --i)
  {
    if (bullets[i].removable)
    {
      bullets.erase(bullets.begin() + i);
    }
  }

  for (int i = upgrades.size() - 1; i >= 0; --i)
  {
    if (upgrades[i].removable)
    {
      upgrades.erase(upgrades.begin() + i);
    }
  }

  // Use the same safe backward loop for bad_guys.
  for (int i = bad_guys.size() - 1; i >= 0; --i)
  {
    if (bad_guys[i]->is_removable())
    {
      delete bad_guys[i];
      bad_guys.erase(bad_guys.begin() + i);
    }
  }

  // Also clean up the collision lists.
  // We don't need to delete the objects here, just remove the pointers.
  for (int i = normal_colliders.size() - 1; i >= 0; --i)
  {
    if (normal_colliders[i]->is_removable())
    {
      normal_colliders.erase(normal_colliders.begin() + i);
    }
  }

  for (int i = special_colliders.size() - 1; i >= 0; --i)
  {
    if (special_colliders[i]->is_removable())
    {
      special_colliders.erase(special_colliders.begin() + i);
    }
  }
}

// the space that it takes for the screen to start scrolling, regarding
// screen bounds (in pixels)
#define X_SPACE (400-16)
// the time it takes to move the camera (in ms)
#define CHANGE_DIR_SCROLL_SPEED 2000

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
  if (scroll_x > level->width * 32 - screen->w)
  {
    scroll_x = level->width * 32 - screen->w;
  }
}

void World::collision_handler()
{
  /* --- COLLISION CULLING SETUP --- */
  // To improve performance, we define an "active" area based on the screen's
  // current position. Objects outside this area will be culled/skipped from
  // most collision checks. A 64-pixel buffer is added to each side for safety,
  // preventing fast-moving objects from tunneling through each other between frames.
  const float screen_x_start = scroll_x - 64.0f;
  const float screen_x_end   = scroll_x + screen->w + 64.0f;


  // --- BULLET vs BADGUY ---
  // Only check active bullets against visible enemies.
  for (auto& bullet : bullets)
  {
    // If a bullet has already been marked for removal, skip it.
    if (bullet.removable)
    {
        continue;
    }

    // Skip collision checks for this bullet if it is outside the active area.
    if (bullet.base.x + bullet.base.width < screen_x_start || bullet.base.x > screen_x_end)
    {
      continue;
    }

    // Check against normal colliders
    for (auto* badguy : normal_colliders)
    {
      // Tux can't kill a dying enemy
      if (badguy->dying != DYING_NOT)
      {
        continue;
      }

      // Also skip collision checks for any enemy outside the active area.
      if (badguy->base.x + badguy->base.width < screen_x_start || badguy->base.x > screen_x_end)
      {
        continue;
      }

      if (rectcollision(bullet.base, badguy->base))
      {
        badguy->collision(0, CO_BULLET);
        bullet.collision(CO_BADGUY);
        break; // A bullet can only hit one enemy.
      }
    }

    // If the bullet hit a normal_collider, it is now inactive. Skip special_colliders.
    if (bullet.removable)
    {
        continue;
    }

    // Check against special colliders
    for (auto* badguy : special_colliders)
    {
        // Tux can't kill a dying enemy
        if (badguy->dying != DYING_NOT)
        {
            continue;
        }

        if (rectcollision(bullet.base, badguy->base))
        {
            badguy->collision(0, CO_BULLET);
            bullet.collision(CO_BADGUY);
            break; // A bullet can only hit one enemy.
        }
    }
  }


  /* --- BADGUY vs BADGUY --- */

  // Pass 1: Special vs. Normal (preserves the "off-screen kill" mechanic)
  // Check every special collider against every normal collider.
  for (auto* special : special_colliders)
  {
    if (special->dying != DYING_NOT)
    {
      continue;
    }

    for (auto* normal : normal_colliders)
    {
      if (normal->dying != DYING_NOT)
      {
        continue;
      }

      // Broad-phase AABB check on X-axis.
      // If the objects are not even close on the X-axis, they can't be colliding.
      // This avoids the more expensive rectcollision() call for most pairs.
      if (special->base.x > normal->base.x + normal->base.width ||
          special->base.x + special->base.width < normal->base.x)
      {
        continue;
      }

      if (rectcollision(special->base, normal->base))
      {
        normal->collision(special, CO_BADGUY);
        special->collision(normal, CO_BADGUY);
      }
    }
  }

  // Pass 2: Special vs. Special
  // Check special colliders against each other.
  for (size_t i = 0; i < special_colliders.size(); ++i)
  {
    BadGuy* special1 = special_colliders[i];
    // Skip any enemy that is already dying.
    if (special1->dying != DYING_NOT)
    {
      continue;
    }

    for (size_t j = i + 1; j < special_colliders.size(); ++j)
    {
      BadGuy* special2 = special_colliders[j];
      // Skip self-check and dying enemies
      if (special2->dying != DYING_NOT)
      {
        continue;
      }

      // Broad-phase AABB check on X-axis.
      if (special1->base.x > special2->base.x + special2->base.width ||
          special1->base.x + special1->base.width < special2->base.x)
      {
        continue;
      }

      if (rectcollision(special1->base, special2->base))
      {
        special2->collision(special1, CO_BADGUY);
        special1->collision(special2, CO_BADGUY);
      }
    }
  }

  // Pass 3: Normal vs. Normal
  // Highly optimized loop that only checks on-screen enemies.
  for (size_t i = 0; i < normal_colliders.size(); ++i)
  {
    if (normal_colliders[i]->dying != DYING_NOT)
    {
      continue;
    }
    // Optimization: Cull off-screen enemies immediately.
    if (normal_colliders[i]->base.x + normal_colliders[i]->base.width < screen_x_start ||
        normal_colliders[i]->base.x > screen_x_end)
    {
      continue;
    }

    for (size_t j = i + 1; j < normal_colliders.size(); ++j)
    {
      if (normal_colliders[j]->dying != DYING_NOT)
      {
        continue;
      }

      // Optimization: Cull the second enemy too.
      if (normal_colliders[j]->base.x + normal_colliders[j]->base.width < screen_x_start ||
          normal_colliders[j]->base.x > screen_x_end)
      {
        continue;
      }

      if (rectcollision(normal_colliders[i]->base, normal_colliders[j]->base))
      {
        normal_colliders[j]->collision(normal_colliders[i], CO_BADGUY);
        normal_colliders[i]->collision(normal_colliders[j], CO_BADGUY);
      }
    }
  }

  // --- PLAYER COLLISIONS ---
  // Stop all further collision checking if Tux/player is dying.
  if(tux.dying != DYING_NOT)
  {
    return;
  }

  // Check against all enemies (both normal and special)
  for (auto* badguy : bad_guys)
  {
    // Ignore dying enemies
    if (badguy->dying != DYING_NOT)
    {
      continue;
    }

    // Simple culling for player collision
    if (badguy->base.x + badguy->base.width < tux.base.x - 64.0f ||
        badguy->base.x > tux.base.x + tux.base.width + 64.0f)
    {
      continue;
    }

    if (rectcollision_offset(badguy->base, tux.base, 0, 0))
    {
      if (tux.previous_base.y < tux.base.y && tux.previous_base.y + tux.previous_base.height <
        badguy->base.y + badguy->base.height/2 && !tux.invincible_timer.started())
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


  /* --- CO_UPGRADE & CO_PLAYER check --- */
  // Upgrades can only be collected by the player, so we only need to check for them near the player
  for (unsigned int i = 0; i < upgrades.size(); ++i)
  {
    // Skip any upgrade that is not within a 64-pixel buffer around Tux
    if (upgrades[i].base.x + upgrades[i].base.width < tux.base.x - 64.0f ||
        upgrades[i].base.x > tux.base.x + tux.base.width + 64.0f)
    {
      continue;
    }

    if (rectcollision(upgrades[i].base, tux.base))
    {
      upgrades[i].collision(&tux, CO_PLAYER, COLLISION_NORMAL);
    }
  }
}

void World::add_score(float x, float y, int s)
{
  player_status.score += s;

  FloatingScore* new_floating_score = new FloatingScore();
  new_floating_score->init(x-scroll_x, y, s);
  floating_scores.push_back(new_floating_score);
}

void World::add_bouncy_distro(float x, float y)
{
  BouncyDistro* new_bouncy_distro = new BouncyDistro();
  new_bouncy_distro->init(x,y);
  bouncy_distros.push_back(new_bouncy_distro);
}

void World::add_broken_brick(Tile* tile, float x, float y)
{
  add_broken_brick_piece(tile, x, y, -1, -4);
  add_broken_brick_piece(tile, x, y + 16, -1.5, -3);

  add_broken_brick_piece(tile, x + 16, y, 1, -4);
  add_broken_brick_piece(tile, x + 16, y + 16, 1.5, -3);
}

void World::add_broken_brick_piece(Tile* tile, float x, float y, float xm, float ym)
{
  BrokenBrick* new_broken_brick = new BrokenBrick();
  new_broken_brick->init(tile, x, y, xm, ym);
  broken_bricks.push_back(new_broken_brick);
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
  Upgrade new_upgrade;
  new_upgrade.init(x,y,dir,kind);
  upgrades.push_back(new_upgrade);
}

void World::add_bullet(float x, float y, float xm, Direction dir)
{
  if(bullets.size() > MAX_BULLETS-1)
    return;

  Bullet new_bullet;
  new_bullet.init(x,y,xm,dir);
  bullets.push_back(new_bullet);

  play_sound(sounds[SND_SHOOT], SOUND_CENTER_SPEAKER);
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
  currentmusic = musictype;
  switch(currentmusic) {
    case HURRYUP_MUSIC:
      music_manager->play_music(get_level()->get_level_music_fast());
      break;
    case LEVEL_MUSIC:
      music_manager->play_music(get_level()->get_level_music());
      break;
    case HERRING_MUSIC:
      music_manager->play_music(herring_song);
      break;
    default:
      music_manager->halt_music();
      break;
  }
}

int World::get_music_type()
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
          add_bouncy_distro(((int)(x + 1) / 32) * 32,
                                  (int)(y / 32) * 32);

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
          add_broken_brick(tile,
                                 ((int)(x + 1) / 32) * 32,
                                 (int)(y / 32) * 32);

          /* Get some score: */
          play_sound(sounds[SND_BRICK], SOUND_CENTER_SPEAKER);
          player_status.score = player_status.score + SCORE_BRICK;
        }
    }
  else if(tile->fullbox)
    tryemptybox(x, y, col_side);
}

/* Empty a box: */
void World::tryemptybox(float x, float y, Direction col_side)
{
  Tile* tile = gettile(x,y);
  if (!tile->fullbox)
    return;

  // according to the collision side, set the upgrade direction
  if(col_side == LEFT)
    col_side = RIGHT;
  else
    col_side = LEFT;

  int posx = ((int)(x+1) / 32) * 32;
  int posy = (int)(y/32) * 32 - 32;
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
          add_bouncy_distro(((int)(x + 1) / 32) * 32,
                                  (int)(y / 32) * 32);
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
    if (badguy->base.x >= x - 32 && badguy->base.x <= x + 32 &&
        badguy->base.y >= y - 16 && badguy->base.y <= y + 16)
    {
      badguy->collision(&tux, CO_PLAYER, COLLISION_BUMP);
    }
  }

  // Check against special colliders as well
  for (auto* badguy : special_colliders)
  {
    if (badguy->base.x >= x - 32 && badguy->base.x <= x + 32 &&
        badguy->base.y >= y - 16 && badguy->base.y <= y + 16)
    {
      badguy->collision(&tux, CO_PLAYER, COLLISION_BUMP);
    }
  }

  // Upgrades:
  for (unsigned int i = 0; i < upgrades.size(); i++)
  {
    if (upgrades[i].base.height == 32 &&
        upgrades[i].base.x >= x - 32 && upgrades[i].base.x <= x + 32 &&
        upgrades[i].base.y >= y - 16 && upgrades[i].base.y <= y + 16)
    {
      upgrades[i].collision(&tux, CO_PLAYER, COLLISION_BUMP);
    }
  }
}

// EOF
