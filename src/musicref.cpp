// src/musicref.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2004 Matthias Braun <matze@braunis.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "musicref.hpp"

MusicRef::MusicRef()
  : music(0)
{
}

MusicRef::MusicRef(MusicManager::MusicResource* newmusic)
  : music(newmusic)
{
  if(music)
    music->refcount++;
}

MusicRef::~MusicRef()
{
  if(music) {
    music->refcount--;
    if(music->refcount == 0)
      music->manager->free_music(music);
  }
}

MusicRef::MusicRef(const MusicRef& other)
  : music(other.music)
{
  if(music)
    music->refcount++;
}

MusicRef&
MusicRef::operator =(const MusicRef& other)
{
  MusicManager::MusicResource* oldres = music;
  music = other.music;
  if(music)
    music->refcount++;
  if(oldres) {
    oldres->refcount--;
    if(oldres->refcount == 0)
      oldres->manager->free_music(oldres);
  }

  return *this;
}

// EOF
