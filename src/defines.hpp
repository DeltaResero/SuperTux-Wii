// src/defines.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#if !defined(SUPERTUX_DEFINES_H)
#define SUPERTUX_DEFINES_H

/* Version */
#ifndef VERSION
  #define VERSION "0.1.4-wii-d.5"
#endif

/* --- Cross-compiler Cache Line Alignment --- */
/* Provides a portable way to align data structures to a 32-byte boundary, */
/* which is optimal for the Wii's L1 cache and beneficial on other platforms. */
#if defined(__GNUC__) || defined(__clang__)
  #define CACHE_LINE_ALIGN __attribute__((aligned(32)))
#elif defined(_MSC_VER)
  #define CACHE_LINE_ALIGN __declspec(align(32))
#else
  #define CACHE_LINE_ALIGN
#endif

/* Memory Optimizations */
#ifdef _WII_
  // Wii has limited RAM and predictable, short file paths.
  inline constexpr int PATH_BUFFER_SIZE = 128;
#else
  // Other platforms have more RAM and may have very long directory paths.
  inline constexpr int PATH_BUFFER_SIZE = 1024;
#endif

/* FPS */
inline constexpr int FPS = 1000 / 25;  // Target: 25 frames/sec (~40 ms frame delay)

/* Directions */
enum Direction
{
  LEFT = 0,
  RIGHT = 1
};

inline constexpr int UP = 0;       // Up direction state
inline constexpr int DOWN = 1;     // Down direction state

/* Dying types */
enum DyingType
{
  DYING_NOT = 0,         // Not dying
  DYING_SQUISHED = 1,    // Squished to death
  DYING_FALLING = 2      // Fell to death
};

/* Sizes */
inline constexpr int SMALL = 0;
inline constexpr int BIG = 1;

/* Speed limits */
inline constexpr float MAX_WALK_XM = 2.3f;   // Max horizontal speed while walking
inline constexpr float MAX_RUN_XM = 3.2f;    // Max horizontal speed while running
inline constexpr float MAX_YM = 20.0f;       // Max vertical speed (falling)

inline constexpr int MAX_JUMP_TIME = 375; // Max jump duration (ms)
inline constexpr int MAX_LIVES = 99;      // Max lives

inline constexpr float WALK_SPEED = 1.0f;    // Speed when walking
inline constexpr float RUN_SPEED = 1.5f;     // Speed when running
inline constexpr float JUMP_SPEED = 1.2f;    // Speed when jumping

/* Gameplay limits */
inline constexpr int START_LIVES = 4;        // Initial lives
inline constexpr int MAX_BULLETS = 2;        // Max bullets player can have
inline constexpr float YM_FOR_JUMP = 6.0f;      // Min vertical speed needed for a jump
inline constexpr float WALK_ACCELERATION_X = 0.03f; // Horizontal acceleration while walking
inline constexpr float RUN_ACCELERATION_X = 0.04f;  // Horizontal acceleration while running
inline constexpr float KILL_BOUNCE_YM = 8.0f;   // Vertical speed after enemy kill bounce

/* Skid settings */
inline constexpr float SKID_XM = 2.0f;          // Horizontal skid speed
inline constexpr int SKID_TIME = 200;        // Skid duration (ms)

/* World sizes */
inline constexpr int OFFSCREEN_DISTANCE = 256; // Distance to offscreen limit
inline constexpr int LEVEL_WIDTH = 375;        // Width of a level

/* Tile settings */
inline constexpr int TILE_SIZE = 32;           // Size of a tile (width and height)
inline constexpr int SCREEN_HEIGHT_TILES = 15; // Height of screen in tiles
inline constexpr int MIN_LEVEL_WIDTH = 21;     // Minimum width of a level in tiles

/* Timing */
inline constexpr int KICKING_TIME = 200;      // Kicking duration (ms)

/* Scroll speeds */
inline constexpr float SCROLL_SPEED_CREDITS = 1.55f;   // Scroll speed for credits
inline constexpr float SCROLL_SPEED_MESSAGE = 1.0f;   // Scroll speed for messages

/* Debugging */
#ifdef DEBUG
  #define DEBUG_MSG( msg ) { \
    printf("%s", msg); printf("\n"); \
  }
#else
  #define DEBUG_MSG( msg ) {}
#endif

#endif /* SUPERTUX_DEFINES_H */

// EOF
