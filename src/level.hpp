// src/level.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2004 Ricardo Cruz <rick2@aeiou.pt>
// Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
// Copyright (C) 2004 Duong-Khang NGUYEN <neoneurone@users.sourceforge.net>
// Copyright (C) 2004 Matthias Braun <matze@braunis.de>
// Copyright (C) 2004 Ryan Flegel <xxdigitalhellxx@hotmail.com>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_LEVEL_H
#define SUPERTUX_LEVEL_H

#include <string>
#include <string_view>
#include <vector>
#include "lispreader.hpp"
#include "texture.hpp"
#include "badguy.hpp"
#include "musicref.hpp"

// Tilemaps
inline constexpr int TM_BG = 0;
inline constexpr int TM_IA = 1;
inline constexpr int TM_FG = 2;

class LevelSubset
{
public:
  std::string name;
  std::string title;
  std::string description;
  Surface* image;
  int levels;

  LevelSubset();
  ~LevelSubset();

  void create(std::string_view subset_name);
  void parse(lisp_object_t* data);
  void load(std::string_view subset);
  void save();
};

struct OriginalTileInfo
{
  int x, y;
  unsigned int tile;
};

struct ResetPoint
{
  int x, y;
};

class Level
{
public:
  std::string name;
  std::string author;
  std::string song_title;
  std::string bkgd_image;
  std::string particle_system;
  int width;
  int start_pos_x, start_pos_y;
  int time_left;
  float gravity;
  bool back_scrolling;
  float hor_autoscroll_speed;
  int bkgd_speed;
  Color bkgd_top;
  Color bkgd_bottom;
  Surface* img_bkgd;

  std::vector<unsigned int> bg_tiles;
  std::vector<unsigned int> ia_tiles;
  std::vector<unsigned int> fg_tiles;
  std::vector<OriginalTileInfo> original_tiles;
  std::vector<ResetPoint> reset_points;
  std::vector<BadGuyData> badguy_data;

  Level();
  Level(std::string_view subset, int level);
  Level(std::string_view filename);
  ~Level();

  void init_defaults();
  int load(std::string_view subset, int level);
  int load(std::string_view filename);
  void reload_bricks_and_coins();
  void save(const std::string& subset, int level);
  void cleanup();
  void load_gfx();
  void load_image(Surface** ptexture, std::string theme, const char* file, bool use_alpha);
  void change_size(int new_width);
  void change(float x, float y, int tm, unsigned int c);
  void load_song();
  void free_song();
  MusicRef get_level_music() const;
  MusicRef get_level_music_fast() const;
  unsigned int gettileid(float x, float y) const;
  unsigned int get_tile_at(int x, int y) const;
  void draw_bg() const;

  // A lightweight "peek" function to read just the title from a level file.
  static std::string get_level_title_fast(std::string_view level_filename);

private:
  MusicRef level_song;
  MusicRef level_song_fast;

  // Helper functions for loading
  void parseProperties(LispReader& reader);
  void parseTilemaps(LispReader& reader, int version);
  void parseObjects(LispReader& reader);
};

#endif /*SUPERTUX_LEVEL_H*/

// EOF
