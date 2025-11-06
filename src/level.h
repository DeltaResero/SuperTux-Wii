//  level.h
//
//  SuperTux
//  Copyright (C) 2004 SuperTux Development Team, see AUTHORS for details
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

#ifndef SUPERTUX_LEVEL_H
#define SUPERTUX_LEVEL_H

#include <string>
#include <vector>
#include "lispreader.h"
#include "texture.h"
#include "badguy.h"
#include "musicref.h"

// Tilemaps
#define TM_BG 0
#define TM_IA 1
#define TM_FG 2

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

  void create(const std::string& subset_name);
  void parse(lisp_object_t* data);
  void load(const std::string& subset);
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
  Level(const std::string& subset, int level);
  Level(const std::string& filename);
  ~Level();

  void init_defaults();
  int load(const std::string& subset, int level);
  int load(const std::string& filename);
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
  void draw_bg();

private:
  MusicRef level_song;
  MusicRef level_song_fast;
};

#endif /*SUPERTUX_LEVEL_H*/

// EOF
