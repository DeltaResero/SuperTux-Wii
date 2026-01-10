// src/musicref.h
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

#ifndef SUPERTUX_MUSICREF_H
#define SUPERTUX_MUSICREF_H

#include "music_manager.hpp"

// Class to handle references to music resources
class MusicRef
{
public:
  MusicRef();
  MusicRef(const MusicRef& other);
  ~MusicRef();

  MusicRef& operator=(const MusicRef& other);

private:
  friend class MusicManager;
  MusicRef(MusicManager::MusicResource* music);  // Private constructor used by MusicManager

  MusicManager::MusicResource* music;  // Pointer to the actual music resource
};

#endif /*SUPERTUX_MUSICREF_H*/

// EOF
