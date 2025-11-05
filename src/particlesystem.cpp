//  particlesystem.cpp
//
//  SuperTux
//  Copyright (C) 2004 Matthias Braun <matze@braunis.de>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License;
//  either version 2 of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "particlesystem.h"

#include <iostream>
#include <cstdlib>
#include <cmath>
#include "globals.h"
#include "world.h"
#include "level.h"
#include "scene.h"

/**
 * Constructs a ParticleSystem object.
 * Initializes the virtual width and height of the particle system
 * based on the screen dimensions.
 */
ParticleSystem::ParticleSystem()
{
  virtual_width = screen->w;
  virtual_height = screen->h;
}

/**
 * Constructs a SnowParticleSystem object.
 * Initializes snowflake particles with random positions, layers, and speeds.
 * The snowflake textures are loaded from image files.
 */
SnowParticleSystem::SnowParticleSystem()
{
  // Initialize snowflake textures
  snowimages[0] = new Surface(datadir + "/images/shared/snow0.png", true);
  snowimages[1] = new Surface(datadir + "/images/shared/snow1.png", true);
  snowimages[2] = new Surface(datadir + "/images/shared/snow2.png", true);

  virtual_width = screen->w * 2.0f;
  virtual_height = screen->h;

  // Pre-allocate memory for all particles at once
  size_t snowflakecount = static_cast<size_t>(virtual_width / 10.0);
  particles.resize(snowflakecount);

  // Create some random snowflakes
  for (size_t i = 0; i < particles.size(); ++i)
  {
    auto& particle = particles[i]; // Work directly with the object in the vector
    particle.x = static_cast<float>(rand() % static_cast<int>(virtual_width));
    particle.y = static_cast<float>(rand() % screen->h);
    particle.layer = i % 2;
    int snowsize = rand() % 3;
    particle.texture = snowimages[snowsize];

    do
    {
      particle.speed = snowsize / 60.0f + (static_cast<float>(rand() % 10) / 300.0f);
    }
    while (particle.speed < 0.01f);

    particle.speed *= World::current()->get_level()->gravity;
  }
}

/**
 * Destroys the SnowParticleSystem object.
 * Cleans up any dynamically allocated memory associated with snowflake textures.
 */
SnowParticleSystem::~SnowParticleSystem()
{
  // Delete snowflake textures
  for (Surface* image : snowimages)
  {
    delete image;
  }
  // The particles vector automatically cleans up its own memory.
}

/**
 * Simulates the movement of snow particles over the elapsed time.
 * Particles are moved downwards based on their speed, and those that leave the screen
 * from the bottom are repositioned at the top.
 * @param elapsed_time The time passed since the last simulation update.
 */
void SnowParticleSystem::simulate(float elapsed_time)
{
  for (auto& particle : particles)
  {
    particle.y += particle.speed * elapsed_time;
    if (particle.y > screen->h)
    {
      particle.y = std::fmod(particle.y, virtual_height);
      particle.x = static_cast<float>(rand() % static_cast<int>(virtual_width));
    }
  }
}

/**
 * Draws the snow particles on the screen.
 * @param scrollx Horizontal scroll offset.
 * @param scrolly Vertical scroll offset.
 * @param layer The layer on which the particles should be drawn.
 */
void SnowParticleSystem::draw(float scrollx, float scrolly, int layer)
{
  for (const auto& particle : particles)
  {
    if (particle.layer != layer)
    {
      continue;
    }

    // Remap x,y coordinates onto screen coordinates
    float x = std::fmod(particle.x - scrollx, virtual_width);
    if (x < 0)
    {
      x += virtual_width;
    }

    float y = std::fmod(particle.y - scrolly, virtual_height);
    if (y < 0)
    {
      y += virtual_height;
    }

    float xmax = std::fmod(x + particle.texture->w, virtual_width);
    float ymax = std::fmod(y + particle.texture->h, virtual_height);

    // Particle on screen
    if (x >= screen->w && xmax >= screen->w)
    {
      continue;
    }
    if (y >= screen->h && ymax >= screen->h)
    {
      continue;
    }

    if (x > screen->w)
    {
      x -= virtual_width;
    }
    if (y > screen->h)
    {
      y -= virtual_height;
    }

    particle.texture->draw(x, y);
  }
}


/**
 * Constructs a CloudParticleSystem object.
 * Initializes cloud particles with random positions and speeds.
 * The cloud texture is loaded from an image file.
 */
CloudParticleSystem::CloudParticleSystem()
{
  // Initialize cloud texture
  cloudimage = new Surface(datadir + "/images/shared/cloud.png", true);

  virtual_width = 2000.0f;
  virtual_height = screen->h;

  // Pre-allocate memory for all particles at once.
  particles.resize(15);

  // Create some random clouds
  for (auto& particle : particles)
  {
    particle.x = static_cast<float>(rand() % static_cast<int>(virtual_width));
    particle.y = static_cast<float>(rand() % static_cast<int>(virtual_height));
    particle.layer = 0;
    particle.texture = cloudimage;
    particle.speed = -static_cast<float>(250 + rand() % 200) / 1000.0f;
  }
}

/**
 * Destroys the CloudParticleSystem object.
 * Cleans up any dynamically allocated memory associated with the cloud texture.
 */
CloudParticleSystem::~CloudParticleSystem()
{
  // Delete cloud texture
  delete cloudimage;
  // The particles vector automatically cleans up its own memory.
}

/**
 * Simulates the movement of cloud particles over the elapsed time.
 * Particles are moved horizontally based on their speed.
 * @param elapsed_time The time passed since the last simulation update.
 */
void CloudParticleSystem::simulate(float elapsed_time)
{
  for (auto& particle : particles)
  {
    particle.x += particle.speed * elapsed_time;
  }
}

/**
 * Draws the cloud particles on the screen.
 * BEHAVIOR FIX: This entire function is a direct copy of the original base class
 * drawing logic. This removes the incorrect parallax effect and restores the
 * vanilla visual behavior.
 * @param scrollx Horizontal scroll offset.
 * @param scrolly Vertical scroll offset.
 * @param layer The layer on which the particles should be drawn.
 */
void CloudParticleSystem::draw(float scrollx, float scrolly, int layer)
{
  for (const auto& particle : particles)
  {
    if (particle.layer != layer)
    {
      continue;
    }

    // Remap x,y coordinates onto screen coordinates
    float x = std::fmod(particle.x - scrollx, virtual_width);
    if (x < 0)
    {
      x += virtual_width;
    }

    float y = std::fmod(particle.y - scrolly, virtual_height);
    if (y < 0)
    {
      y += virtual_height;
    }

    float xmax = std::fmod(x + particle.texture->w, virtual_width);
    float ymax = std::fmod(y + particle.texture->h, virtual_height);

    // Particle on screen
    if (x >= screen->w && xmax >= screen->w)
    {
      continue;
    }
    if (y >= screen->h && ymax >= screen->h)
    {
      continue;
    }

    if (x > screen->w)
    {
      x -= virtual_width;
    }
    if (y > screen->h)
    {
      y -= virtual_height;
    }

    particle.texture->draw(x, y);
  }
}

// EOF
