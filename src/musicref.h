//  musicref.h
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

#ifndef SUPERTUX_MUSICREF_H
#define SUPERTUX_MUSICREF_H

#include "music_manager.h"

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
