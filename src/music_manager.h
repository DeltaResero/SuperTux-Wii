//  $Id: music_manager.h 827 2004-04-29 00:15:11Z grumbel $
//
//  SuperTux -  A Jump'n Run
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

/** This class manages a list of music resources and is responsible for playing
 * the music.
 */
class MusicManager
{
public:
  MusicManager();
  ~MusicManager();

  /**
   * Loads the music file and returns a reference to the music resource.
   * @param file The name of the music file to load.
   * @return A reference to the loaded music resource, or a null reference if failed.
   * @throws std::runtime_error if the music file does not exist or cannot be loaded.
   */
  MusicRef load_music(const std::string& file);

  /**
   * Checks if the music file exists and loads it if not already loaded.
   * @param filename The name of the music file to check.
   * @return True if the music file exists or was loaded successfully, false otherwise.
   */
  bool exists_music(const std::string& filename);

  /**
   * Plays the specified music resource.
   * @param music Reference to the music resource to play.
   * @param loops Number of times to loop the music (-1 for infinite loops).
   */
  void play_music(const MusicRef& music, int loops = -1);

  /**
   * Halts the currently playing music.
   * If the reference count of the music reaches zero, the music resource is freed.
   */
  void halt_music();

  /**
   * Enables or disables music playback.
   * @param enable True to enable music, false to disable.
   * If music is disabled, any currently playing music is halted.
   */
  void enable_music(bool enable);

private:
  friend class MusicRef;

  class MusicResource
  {
  public:
    ~MusicResource();

    MusicManager* manager;
    Mix_Music* music;
    int refcount;
  };

  /**
   * Frees the specified music resource.
   * @param music Pointer to the music resource to free.
   * Frees the memory associated with the music resource and removes it from the internal map.
   */
  void free_music(MusicResource* music);

  std::map<std::string, MusicResource> musics;
  MusicResource* current_music;
  bool music_enabled;
};

#endif // HEADER_MUSIC_MANAGER_H

// EOF
