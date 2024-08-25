//  $Id: music_manager.cpp 827 2004-04-29 00:15:11Z grumbel $
//
//  SuperTux
//  Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
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

#include <cassert>
#include <stdexcept>
#include "music_manager.h"
#include "musicref.h"
#include "sound.h"
#include "setup.h"

/**
 * Constructs a MusicManager.
 * Initializes the current_music to nullptr and sets music_enabled to true.
 */
MusicManager::MusicManager()
  : current_music(nullptr), music_enabled(true)
{
}

/**
 * Destructor for MusicManager.
 * Stops any currently playing music if an audio device is present.
 */
MusicManager::~MusicManager()
{
  if (audio_device)
    Mix_HaltMusic();  // Stop any currently playing music on destruction
}

/**
 * Loads the music file and returns a reference to the music resource.
 * @param file The name of the music file to load.
 * @return A reference to the loaded music resource, or a null reference if failed.
 * @throws std::runtime_error if the music file does not exist or cannot be loaded.
 */
MusicRef MusicManager::load_music(const std::string& file)
{
  if (!audio_device)
    return MusicRef(nullptr);  // Return a null reference if no audio device is available

  if (!exists_music(file))
    throw std::runtime_error("Couldn't load music file: " + file);

  auto i = musics.find(file);
  assert(i != musics.end());  // Ensure the music file exists in the map
  return MusicRef(& (i->second));
}

/**
 * Checks if the music file exists and loads it if not already loaded.
 * @param file The name of the music file to check.
 * @return True if the music file exists or was loaded successfully, false otherwise.
 */
bool MusicManager::exists_music(const std::string& file)
{
  auto i = musics.find(file);
  if (i != musics.end())
    return true;

  Mix_Music* song = Mix_LoadMUS(file.c_str());
  if (!song)
    return false;  // Return false if the music file couldn't be loaded

  auto result = musics.emplace(file, MusicResource());
  MusicResource& resource = result.first->second;
  resource.manager = this;
  resource.music = song;

  return true;
}

/**
 * Frees the specified music resource.
 * @param music Pointer to the music resource to free.
 * Frees the memory associated with the music resource and removes it from the internal map.
 */
void MusicManager::free_music(MusicResource* music)
{
  Mix_FreeMusic(music->music);  // Free the music resource using SDL_mixer

  for (auto i = musics.begin(); i != musics.end(); ++i)
  {
    if (&i->second == music)
    {
      musics.erase(i);
      break;
    }
  }
}

/**
 * Plays the specified music resource.
 * @param musicref Reference to the music resource to play.
 * @param loops Number of times to loop the music (-1 for infinite loops).
 * If the specified music is already playing, the method does nothing.
 */
void MusicManager::play_music(const MusicRef& musicref, int loops)
{
  if (!audio_device)
    return;

  if (musicref.music == nullptr || current_music == musicref.music)
    return;

  if (current_music)
    current_music->refcount--;

  current_music = musicref.music;
  current_music->refcount++;

  if (music_enabled)
    Mix_PlayMusic(current_music->music, loops);
}

/**
 * Halts the currently playing music.
 * If the reference count of the music reaches zero, the music resource is freed.
 */
void MusicManager::halt_music()
{
  if (!audio_device)
    return;

  Mix_HaltMusic();  // Stop the current music

  if (current_music) {
    current_music->refcount--;
    if (current_music->refcount == 0)
      free_music(current_music);  // Free the music if no longer in use
    current_music = nullptr;  // Reset the current music to null
  }
}

/**
 * Enables or disables music playback.
 * @param enable True to enable music, false to disable.
 * If music is disabled, any currently playing music is halted.
 */
void MusicManager::enable_music(bool enable)
{
  if (!audio_device)
    return;

  if (enable == music_enabled)
    return;

  music_enabled = enable;
  if (!music_enabled) {
    Mix_HaltMusic();  // Stop music if disabling
  } else {
    Mix_PlayMusic(current_music->music, -1);  // Resume or start music if enabling
  }
}

/**
 * Destructor for MusicResource.
 * Frees the associated music resource if the destructor is called,
 * although this is intentionally avoided due to known SDL_mixer bugs.
 */
MusicManager::MusicResource::~MusicResource()
{
  // Mix_FreeMusic(music);
  // The destructor intentionally does not call Mix_FreeMusic due to known SDL_mixer bugs.
}

// EOF
