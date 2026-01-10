// src/resources.h
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

#ifndef SUPERTUX_RESOURCES_H
#define SUPERTUX_RESOURCES_H

#include "musicref.hpp"

class SpriteManager;
class MusicManager;

extern Surface* img_waves[3];
extern Surface* img_water;
extern Surface* img_pole;
extern Surface* img_poletop;
extern Surface* img_flag[2];
extern Surface* img_cloud[2][4];
extern Surface* img_distro[4]; // Corrected Declaration

extern Surface* img_super_bkgd;

extern MusicRef herring_song;
extern MusicRef level_end_song;

extern SpriteManager* sprite_manager;
extern MusicManager* music_manager;

void loadshared();
void unloadshared();
void loadsounds();
void unloadsounds();

#endif /*SUPERTUX_RESOURCES_H*/

// EOF
