//  defines.h
//
//  SuperTux
//  Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
//  Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//  02111-1307, USA.

#if !defined(SUPERTUX_DEFINES_H)
#define SUPERTUX_DEFINES_H

/* Version */
#ifndef VERSION
  #define VERSION "0.1.4-wii-d.2"
#endif

/* FPS */
#define FPS (1000 / 25)  // Target: 25 frames/sec (~40 ms frame delay)

/* Directions */
enum Direction
{
  LEFT = 0,
  RIGHT = 1
};

#define UP 0       // Up direction state
#define DOWN 1     // Down direction state

/* Dying types */
enum DyingType
{
  DYING_NOT = 0,         // Not dying
  DYING_SQUISHED = 1,    // Squished to death
  DYING_FALLING = 2      // Fell to death
};

/* Sizes */
#define SMALL 0
#define BIG 1

/* Speed limits */
#define MAX_WALK_XM 2.3   // Max horizontal speed while walking
#define MAX_RUN_XM 3.2    // Max horizontal speed while running
#define MAX_YM 20.0       // Max vertical speed (falling)

#define MAX_JUMP_TIME 375 // Max jump duration (ms)
#define MAX_LIVES 99      // Max lives

#define WALK_SPEED 1.0    // Speed when walking
#define RUN_SPEED 1.5     // Speed when running
#define JUMP_SPEED 1.2    // Speed when jumping

/* Gameplay limits */
#define START_LIVES 4        // Initial lives
#define MAX_BULLETS 2        // Max bullets player can have
#define YM_FOR_JUMP 6.0      // Min vertical speed needed for a jump
#define WALK_ACCELERATION_X 0.03 // Horizontal acceleration while walking
#define RUN_ACCELERATION_X 0.04  // Horizontal acceleration while running
#define KILL_BOUNCE_YM 8.0   // Vertical speed after enemy kill bounce

/* Skid settings */
#define SKID_XM 2.0          // Horizontal skid speed
#define SKID_TIME 200        // Skid duration (ms)

/* World sizes */
#define OFFSCREEN_DISTANCE 256 // Distance to offscreen limit
#define LEVEL_WIDTH 375        // Width of a level

/* Timing */
#define KICKING_TIME 200      // Kicking duration (ms)

/* Scroll speeds */
#define SCROLL_SPEED_CREDITS 1.2   // Scroll speed for credits
#define SCROLL_SPEED_MESSAGE 1.0   // Scroll speed for messages

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
