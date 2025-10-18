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
  snowimages[0] = new Surface(datadir + "/images/shared/snow0.png", USE_ALPHA);
  snowimages[1] = new Surface(datadir + "/images/shared/snow1.png", USE_ALPHA);
  snowimages[2] = new Surface(datadir + "/images/shared/snow2.png", USE_ALPHA);

  virtual_width = screen->w * 2.0f;
  virtual_height = screen->h;

  // Pre-allocate memory for all particles at once to avoid using 'new' in a loop.
  // This is the core of the object pool pattern for performance.
  size_t snowflakecount = static_cast<size_t>(virtual_width / 10.0);
  particles.resize(snowflakecount);

  // Create some random snowflakes
  for (SnowParticle& particle : particles)
  {
    particle.x = static_cast<float>(rand() % static_cast<int>(virtual_width));
    particle.y = static_cast<float>(rand() % screen->h);
    particle.layer = rand() % 2;
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
  // The particles vector automatically cleans up its own memory,
  // as it now stores objects directly instead of pointers.
}

/**
 * Simulates the movement of snow particles over the elapsed time.
 * Particles are moved downwards based on their speed, and those that leave the screen
 * from the bottom are repositioned at the top.
 * @param elapsed_time The time passed since the last simulation update.
 */
void SnowParticleSystem::simulate(float elapsed_time)
{
  for (SnowParticle& particle : particles)
  {
    particle.y += particle.speed * elapsed_time;
    if (particle.y > virtual_height)
    {
      // Reset particle to the top with a new random x position
      particle.y = std::fmod(particle.y, virtual_height) - virtual_height;
      particle.x = static_cast<float>(rand() % static_cast<int>(virtual_width));
    }
  }
}

/**
 * Draws the snow particles on the screen.
 * This function remaps particle coordinates based on the scrolling offsets
 * and handles wrapping for seamless parallax effects.
 * @param scrollx Horizontal scroll offset.
 * @param scrolly Vertical scroll offset (unused for snow).
 * @param layer The layer on which the particles should be drawn.
 */
void SnowParticleSystem::draw(float scrollx, float scrolly, int layer)
{
  for (const SnowParticle& particle : particles)
  {
    if (particle.layer != layer)
    {
      continue;
    }

    // Remap x coordinate onto screen coordinates with parallax effect.
    // Different layers scroll at different speeds.
    float screen_x = std::fmod(particle.x - scrollx * (0.5f + particle.layer * 0.5f), virtual_width);
    if (screen_x < 0)
    {
      screen_x += virtual_width;
    }

    // Snow y-position is not affected by camera's vertical scroll.
    float screen_y = particle.y;

    // Draw the particle.
    particle.texture->draw(screen_x, screen_y);

    // If the particle is on the edge, draw its wrapped-around copy to ensure seamless tiling.
    if (screen_x + particle.texture->w > virtual_width) {
      particle.texture->draw(screen_x - virtual_width, screen_y);
    }
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
  cloudimage = new Surface(datadir + "/images/shared/cloud.png", USE_ALPHA);

  virtual_width = 2000.0f;
  virtual_height = screen->h;

  // Pre-allocate memory for all particles at once.
  particles.resize(15);

  // Create some random clouds
  for (CloudParticle& particle : particles)
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
 * Particles are moved horizontally based on their speed and wrap around the screen.
 * @param elapsed_time The time passed since the last simulation update.
 */
void CloudParticleSystem::simulate(float elapsed_time)
{
  for (CloudParticle& particle : particles)
  {
    particle.x += particle.speed * elapsed_time;
    // If a particle moves completely off the left side of the screen,
    // wrap it around to the right side to create a continuous flow.
    if (particle.x < -particle.texture->w)
    {
      particle.x += virtual_width + particle.texture->w;
    }
  }
}

/**
 * Draws the cloud particles on the screen.
 * This function remaps particle coordinates based on the scrolling offsets
 * and handles wrapping for a seamless parallax effect.
 * @param scrollx Horizontal scroll offset.
 * @param scrolly Vertical scroll offset (unused for clouds).
 * @param layer The layer on which the particles should be drawn.
 */
void CloudParticleSystem::draw(float scrollx, float scrolly, int layer)
{
  // Clouds are always on layer 0.
  if(layer != 0) return;

  for (const CloudParticle& particle : particles)
  {
    // Remap x coordinate onto screen coordinates with a parallax effect (slower scrolling).
    float screen_x = std::fmod(particle.x - scrollx * 0.5f, virtual_width);
    if(screen_x < 0)
    {
      screen_x += virtual_width;
    }

    // Cloud y-position is not affected by camera's vertical scroll.
    float screen_y = particle.y;

    particle.texture->draw(screen_x, screen_y);

    // If the particle is on the edge, draw its wrapped-around copy.
    if (screen_x + particle.texture->w > virtual_width) {
      particle.texture->draw(screen_x - virtual_width, screen_y);
    }
  }
}

// EOF
