//  particlesystem.h
//
//  SuperTux
//  Copyright (C) 2004 Matthias Braun <matze@braunis.de>
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef SUPERTUX_PARTICLESYSTEM_H
#define SUPERTUX_PARTICLESYSTEM_H

#include <vector>
#include <array>
#include "texture.h"

/**
 * This is the base class for particle systems. It defines the interface for
 * systems that are responsible for storing and managing a set of particles.
 * Each particle has an x- and y-coordinate, the number of the layer where it
 * should be drawn, and a texture.
 * The coordinate system used here is a virtual one. It would be a bad idea to
 * populate whole levels with particles, so we're using a virtual rectangle
 * that is tiled onto the level when drawing. This rectangle has the size
 * (virtual_width, virtual_height). We're using modulo on the particle
 * coordinates, so when a particle leaves one side, it'll reenter on the
 * opposite side.
 *
 * Classes that implement a specific particle system should subclass from this
 * class, initialize particles in the constructor, and move them in the simulate
 * function.
 */

struct Particle
{
    float x = 0.0f;
    float y = 0.0f;
    int layer = 0;
    Surface* texture = nullptr;
    // We can add an 'active' flag if we want dynamic particle effects later.
    // For snow/clouds that live forever, we don't need it.
};

// Specialized data for a snow particle
struct SnowParticle : public Particle
{
    float speed = 0.0f;
};

// Specialized data for a cloud particle
struct CloudParticle : public Particle
{
    float speed = 0.0f;
};

class ParticleSystem
{
public:
    ParticleSystem();
    virtual ~ParticleSystem() = default; // Use default destructor

    // The draw function can be shared if we templatize it or use std::function,
    // but for simplicity, we'll let subclasses handle their own drawing for now.
    virtual void draw(float scrollx, float scrolly, int layer) = 0;
    virtual void simulate(float elapsed_time) = 0;

protected:
    float virtual_width = 0.0f;
    float virtual_height = 0.0f;
};

class SnowParticleSystem : public ParticleSystem
{
public:
    SnowParticleSystem();
    ~SnowParticleSystem() override; // Use override for clarity

    void draw(float scrollx, float scrolly, int layer) override;
    void simulate(float elapsed_time) override;

private:
    // Store particles by value in a contiguous block of memory
    std::vector<SnowParticle> particles;
    // Use std::array for fixed-size texture arrays
    std::array<Surface*, 3> snowimages;
};

class CloudParticleSystem : public ParticleSystem
{
public:
    CloudParticleSystem();
    ~CloudParticleSystem() override;

    void draw(float scrollx, float scrolly, int layer) override;
    void simulate(float elapsed_time) override;

private:
    // Store particles by value.
    std::vector<CloudParticle> particles;
    Surface* cloudimage = nullptr;
};

#endif

// EOF
