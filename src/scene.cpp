// src/scene.cpp
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

#include <stdlib.h>
#include "scene.h"
#include "defines.h"

// Global player status
PlayerStatus player_status;

/**
 * Constructor for PlayerStatus.
 * Initializes score, lives, and bonus to default values.
 */
PlayerStatus::PlayerStatus()
  : score(0),
    distros(0),
    lives(START_LIVES),
    bonus(NO_BONUS),
    score_multiplier(1)
{
}

/**
 * Resets the player's status, including score, lives, and bonus.
 */
void PlayerStatus::reset()
{
  score = 0;
  distros = 0;
  lives = START_LIVES;
  bonus = NO_BONUS;
  score_multiplier = 1;
}

/**
 * Converts a PlayerStatus::BonusType to a human-readable string.
 * @param b The bonus type to convert.
 * @return The corresponding string ("none", "growup", or "iceflower").
 */
std::string bonus_to_string(PlayerStatus::BonusType b)
{
  switch (b)
  {
    case PlayerStatus::NO_BONUS:
      return "none";
    case PlayerStatus::GROWUP_BONUS:
      return "growup";
    case PlayerStatus::FLOWER_BONUS:
      return "iceflower";
    default:
      return "none";
  }
}

/**
 * Converts a string to a PlayerStatus::BonusType.
 * @param str The string to convert ("none", "growup", or "iceflower").
 * @return The corresponding BonusType, or NO_BONUS if invalid.
 */
PlayerStatus::BonusType string_to_bonus(const std::string& str)
{
  if (str == "none")
  {
    return PlayerStatus::NO_BONUS;
  }
  else if (str == "growup")
  {
    return PlayerStatus::GROWUP_BONUS;
  }
  else if (str == "iceflower")
  {
    return PlayerStatus::FLOWER_BONUS;
  }
  else
  {
    return PlayerStatus::NO_BONUS;
  }
}

// FIXME: Move this into a view class
float scroll_x = 0.0f;

// Global frame counter
unsigned int global_frame_counter = 0;

// EOF
