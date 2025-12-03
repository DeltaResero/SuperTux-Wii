// src/particlesystem.h
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2004 Matthias Braun <matze@braunis.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_PARTICLESYSTEM_H
#define SUPERTUX_PARTICLESYSTEM_H

#include <vector>
#include <array>
#include "texture.h"

// Instead of complex inheritance, we use simple structs.
// This makes the data layout much more efficient for the CPU.
struct Particle
{
    float x = 0.0f;
    float y = 0.0f;
    int layer = 0;
    Surface* texture = nullptr;
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
    virtual ~ParticleSystem() = default;

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
    ~SnowParticleSystem() override;

    void draw(float scrollx, float scrolly, int layer) override;
    void simulate(float elapsed_time) override;

private:
    std::vector<SnowParticle> particles;
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
    // Object pool for clouds
    std::vector<CloudParticle> particles;
    Surface* cloudimage = nullptr;
};

#endif /*SUPERTUX_PARTICLESYSTEM_H*/

// EOF
