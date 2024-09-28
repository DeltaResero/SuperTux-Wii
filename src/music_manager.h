//  music_manager.h
//
//  SuperTux
//  Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
//  Copyright (C) 2004 Duong-Khang NGUYEN <neoneurone@users.sf.net>
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

#ifndef HEADER_MUSIC_MANAGER_H
#define HEADER_MUSIC_MANAGER_H

#include <SDL_mixer.h>
#include <string>
#include <map>

class MusicRef;

// Class to manage and play music resources
class MusicManager
{
public:
  MusicManager();
  ~MusicManager();

  MusicRef load_music(const std::string& file);  // Load a music file
  bool exists_music(const std::string& filename);  // Check if music file exists
  void play_music(const MusicRef& music, int loops = -1);  // Play the loaded music
  void halt_music();  // Stop currently playing music
  void enable_music(bool enable);  // Enable or disable music playback

private:
  friend class MusicRef;

  class MusicResource
  {
  public:
    ~MusicResource();

    MusicManager* manager;  // Manager handling this resource
    Mix_Music* music;       // SDL music resource
    int refcount;           // Reference count for the music
  };

  void free_music(MusicResource* music);  // Free a music resource

  std::map<std::string, MusicResource> musics;  // Map to hold music resources
  MusicResource* current_music;  // Currently playing music
  bool music_enabled;            // Flag to enable or disable music
};

#endif // HEADER_MUSIC_MANAGER_H

// EOF
