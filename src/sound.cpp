// src/sound.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2004 Duong-Khang NGUYEN <neoneurone@users.sf.net>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <SDL2/SDL_mixer.h>
#include "defines.hpp"
#include "globals.hpp"
#include "sound.hpp"
#include "setup.hpp"

/* Global variables */
bool use_sound = true;    /* handle sound on/off menu and command-line option */
bool use_music = true;    /* handle music on/off menu and command-line option */
bool audio_device = true; /* != 0: available and initialized */


/* Memory usage for SDL_mixer channel allocations is roughly 256 bytes per channel
 * - 4 channels: ~1 KB || 8 channels: ~2 KB || 16 channels: ~4 KB || 32 channels: ~8 KB */
const int TOTAL_CHANNELS = 32;   // Fixed number of channels for sound effects

const char * soundfilenames[NUM_SOUNDS] =
{
  "/sounds/jump.wav",
  "/sounds/bigjump.wav",
  "/sounds/skid.wav",
  "/sounds/distro.wav",
  "/sounds/herring.wav",
  "/sounds/brick.wav",
  "/sounds/hurt.wav",
  "/sounds/squish.wav",
  "/sounds/fall.wav",
  "/sounds/ricochet.wav",
  "/sounds/upgrade.wav",
  "/sounds/excellent.wav",
  "/sounds/coffee.wav",
  "/sounds/shoot.wav",
  "/sounds/lifeup.wav",
  "/sounds/stomp.wav",
  "/sounds/kick.wav",
  "/sounds/explode.wav",
  "/sounds/warp.wav"
};

Mix_Chunk * sounds[NUM_SOUNDS];

/**
 * Open the audio device and allocate a fixed number of channels.
 * @param frequency Frequency to be set for audio.
 * @param format Format of the audio (e.g., AUDIO_S16SYS).
 * @param channels Number of channels (1 for mono, 2 for stereo).
 * @param chunksize Size of each audio chunk.
 * @return 0 if success, negative values for errors.
 */
int open_audio(int frequency, Uint16 format, int channels, int chunksize)
{
  if (Mix_OpenAudio(frequency, format, channels, chunksize) < 0)
  {
    return -1;
  }

  // Allocate channels for mixing
  if (Mix_AllocateChannels(TOTAL_CHANNELS) != TOTAL_CHANNELS)
  {
    return -2;
  }

  /* Reserve some channels and register panning effects */
  if (Mix_ReserveChannels(SOUND_RESERVED_CHANNELS) != SOUND_RESERVED_CHANNELS)
  {
    return -3;
  }

  /* Prepare the spanning effects */
  Mix_SetPanning(SOUND_LEFT_SPEAKER, 230, 24);
  Mix_SetPanning(SOUND_RIGHT_SPEAKER, 24, 230);

  return 0;
}

/**
 * Close the audio device.
 */
void close_audio(void)
{
  if (audio_device)
  {
    Mix_UnregisterAllEffects(SOUND_LEFT_SPEAKER);
    Mix_UnregisterAllEffects(SOUND_RIGHT_SPEAKER);
    Mix_CloseAudio();
  }
}

/**
 * Load a sound file into a Mix_Chunk object.
 * @param file File path to the sound file.
 * @return Pointer to the loaded sound chunk, or nullptr if loading failed.
 */
Mix_Chunk* load_sound(const std::string& file)
{
  if (!audio_device)
  {
    return nullptr;
  }

  Mix_Chunk* snd = Mix_LoadWAV(file.c_str());

  if (snd == nullptr)
  {
    st_abort("Can't load", file);
  }

  return snd;
}

/**
 * Play a sound on the specified speaker and check if all channels are full (debug mode).
 * @param snd Sound chunk to be played.
 * @param whichSpeaker Enum specifying left, right, or center speaker.
 */
void play_sound(Mix_Chunk* snd, enum Sound_Speaker whichSpeaker)
{
  /* This won't call the function if the user has disabled sound
   * either via menu or via command-line option
   */
  if (!audio_device || !use_sound)
  {
    return;
  }

#ifdef DEBUG
  // Check if all channels are full
  if (Mix_Playing(-1) >= TOTAL_CHANNELS)
  {
    printf("Warning: Exceeded available channels! Sound may not play.\n");
  }
#endif

  Mix_PlayChannel(whichSpeaker, snd, 0);

  /* Prepare for panning effects for next call */
  switch (whichSpeaker)
  {
    case SOUND_LEFT_SPEAKER:
      Mix_SetPanning(SOUND_LEFT_SPEAKER, 230, 24);
      break;

    case SOUND_RIGHT_SPEAKER:
      Mix_SetPanning(SOUND_RIGHT_SPEAKER, 24, 230);
      break;

    default:  // Keep the compiler happy
      break;
  }
}

/**
 * Free a sound chunk from memory.
 * @param chunk Pointer to the sound chunk to be freed.
 */
void free_chunk(Mix_Chunk* chunk)
{
  Mix_FreeChunk(chunk);
}

// EOF
