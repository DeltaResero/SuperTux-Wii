// src/worldmap.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_WORLDMAP_H
#define SUPERTUX_WORLDMAP_H

#include <memory>
#include <vector>
#include <string>
#include <string_view>

#include "musicref.hpp"
#include "gameloop.hpp"

// Forward declare SDL_Event to avoid including SDL.h in the header
// Corrected from 'struct' to 'union' to match the actual SDL definition.
union SDL_Event;

class RenderBatcher;

namespace WorldMapNS
{

struct Point
{
  Point() : x(0), y(0) {}

  Point(const Point& pos)
    : x(pos.x), y(pos.y) {}

  Point& operator=(const Point& pos)
  {
    x = pos.x;
    y = pos.y;
    return *this;
  }

  Point(int x_, int y_)
    : x(x_), y(y_) {}

  int x;
  int y;
};

// For one way tiles
enum
{
  BOTH_WAYS,
  NORTH_SOUTH_WAY,
  SOUTH_NORTH_WAY,
  EAST_WEST_WAY,
  WEST_EAST_WAY
};

class Tile
{
public:
  Tile();
  ~Tile();

  std::unique_ptr<Surface> sprite;

  // Directions in which Tux is allowed to walk from this tile
  bool north;
  bool east;
  bool south;
  bool west;

  /** One way tile */
  int one_way;

  /** Stop on this tile or walk over it? */
  bool stop;

  /** When set automatically turn directions when walked over such a
      tile (ie. walk smoothly a curve) */
  bool auto_walk;
};

class TileManager
{
private:
  typedef std::vector<std::unique_ptr<Tile>> Tiles;
  Tiles tiles;

public:
  TileManager();
  ~TileManager();

  Tile* get(int i);
};

enum Direction { D_NONE, D_WEST, D_EAST, D_NORTH, D_SOUTH };

std::string direction_to_string(Direction d);
Direction   string_to_direction(std::string_view d);
Direction reverse_dir(Direction d);

class WorldMap;

class Tux
{
public:
  Direction back_direction;
private:
  WorldMap* worldmap;
  std::unique_ptr<Surface> largetux_sprite;
  std::unique_ptr<Surface> firetux_sprite;
  std::unique_ptr<Surface> smalltux_sprite;

  Direction input_direction;
  Direction direction;
  Point tile_pos;
  /** Length by which tux is away from its current tile, length is in
      input_direction direction */
  float offset;
  bool  moving;

  void stop();
public:
  explicit Tux(WorldMap* worldmap_);
  ~Tux();

  void loadSprites();
  void deleteSprites();

  void draw(const Point& offset_, RenderBatcher* batcher);
  void update(float delta);

  void set_direction(Direction d)
  {
    input_direction = d;
  }

  bool is_moving() const
  {
    return moving;
  }

  // Added const
  Point get_pos() const;

  Point get_tile_pos() const
  {
    return tile_pos;
  }
  void  set_tile_pos(Point p)
  {
    tile_pos = p;
  }
};

/** */
class WorldMap
{
public:
  static WorldMap* current() { return current_; }
  void quit_map() { quit = true; }
public:
  struct Level
  {
    /** Tile position on the map. A level whose entry gives no coordinates
        keeps -1 and is simply never stood on, rather than matching a tile
        by accident. */
    int x = -1;
    int y = -1;
    std::string name;
    std::string title;
    bool solved = false;

    /** Filename of the extro text to show once the level is
        successfully completed */
    std::string extro_filename;

    /** Message to show in the Map during a certain time */
    std::string display_map_message;
    bool passive_message = true;

    /** Teleporters */
    int teleport_dest_x = -1;
    int teleport_dest_y = -1;
    std::string teleport_message;
    bool invisible_teleporter = false;

    /** If false, disables the auto walking after finishing a level */
    bool auto_path = true;

    /** Only applies actions (ie. map messages) when going to that direction */
    bool apply_action_north = true;
    bool apply_action_east = true;
    bool apply_action_south = true;
    bool apply_action_west = true;

    // Directions which are walkable from this level
    bool north = true;
    bool east = true;
    bool south = true;
    bool west = true;
  };

  /** Variables to deal with the passive map messages */
  Timer passive_message_timer;
  std::string passive_message;

  /** A lightweight function to get a worldmap's title without a full load */
  static std::string get_world_title_fast(std::string_view mapfile_path);

  WorldMap();
  ~WorldMap();

  void loadSprites();
  void deleteSprites();

  void set_map_file(std::string_view mapfile);

  /** Busy loop */
  void display();

  void load_map();

  void get_input();

  /** Update Tux position */
  void update(float delta);

  /** Draw one frame */
  void draw(const Point& offset_);

  static Point get_next_tile(Point pos, Direction direction);
  Tile* at(Point pos);
  WorldMap::Level* at_level();

  /** Check if it is possible to walk from \a pos into \a direction,
      if possible, write the new position to \a new_pos */
  bool path_ok(Direction direction, Point pos, Point* new_pos);

  void savegame(std::string_view filename);
  void loadgame(std::string_view filename);
  void loadmap(std::string_view filename);

  const std::string& get_world_title() const
  {
    return name;
  }

  const int& get_start_x() const
  {
    return start_x;
  }

  const int& get_start_y() const
  {
    return start_y;
  }

  /** This functions should be call by contrib menu to set
     all levels as played, since their state is not saved. */
  void set_levels_as_solved()
  {
    for(Levels::iterator i = levels.begin(); i != levels.end(); ++i)
      i->solved = true;
  }

private:
  // Moved typedef to before its first use.
  typedef std::vector<Level> Levels;

  // Refactored main loop components
  void processInput();
  void updateScene(float deltaTime);
  void renderScene();

  // Refactored input handlers
  void handleKeyboardInput(const SDL_Event& event);
  void handleJoystickInput(const SDL_Event& event);

  // Refactored update logic
  void handleLevelCompletion(GameSession::ExitStatus result, bool coffee, bool big, Level* level);

  void draw_status();
  static void on_escape_press();

  // Tells the player a savegame could not be written
  void report_save_failure(std::string_view filename);

  // Smart tile substitution for snow tiles
  // Added const
  int get_display_tile_id(int x, int y) const;

  std::unique_ptr<Tux> tux;
  bool quit;

  std::unique_ptr<Surface> level_sprite;
  std::unique_ptr<Surface> leveldot_green;
  std::unique_ptr<Surface> leveldot_red;
  std::unique_ptr<Surface> leveldot_teleporter;

  std::string name;
  std::string music;

  std::vector<int> tilemap;
  std::vector<int> display_tilemap; // Cached visual tile IDs
  int width;
  int height;

  int start_x;
  int start_y;

  std::unique_ptr<TileManager> tile_manager;

  Levels levels;

  MusicRef song;

  Direction input_direction;
  bool enter_level;

  Point offset;
  std::string savegame_file;
  std::string map_file;

  std::unique_ptr<RenderBatcher> m_renderBatcher;

private:
  static WorldMap* current_;
};

} // namespace WorldMapNS

#endif

// EOF
