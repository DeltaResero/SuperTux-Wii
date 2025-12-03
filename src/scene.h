// src/scene.h
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

#ifndef SUPERTUX_SCENE_H
#define SUPERTUX_SCENE_H

#include "texture.h"
#include "timer.h"

#define FRAME_RATE 10  // 100 Frames per second (10ms)

// Holds the player's status
struct PlayerStatus
{
  int score;
  int distros;
  int lives;
  enum BonusType { NO_BONUS, GROWUP_BONUS, FLOWER_BONUS };
  BonusType bonus;

  int score_multiplier;

  PlayerStatus();  // Constructor to initialize the player's status
  void reset();    // Resets the player's score, lives, and bonuses
};

// Utility functions to convert BonusType to and from strings
std::string bonus_to_string(PlayerStatus::BonusType b);
PlayerStatus::BonusType string_to_bonus(const std::string& str);

// Global variables for player status and frame management
extern PlayerStatus player_status;
extern float scroll_x;
extern unsigned int global_frame_counter;

#endif /*SUPERTUX_SCENE_H*/

// EOF
