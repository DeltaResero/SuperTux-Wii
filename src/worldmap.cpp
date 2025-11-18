//  worldmap.cpp
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

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include "globals.h"
#include "texture.h"
#include "screen.h"
#include "lispreader.h"
#include "gameloop.h"
#include "setup.h"
#include "worldmap.h"
#include "resources.h"
#include "level.h"
#include "timer.h"
#include "player.h"
#include "utils.h"

#define DISPLAY_MAP_MESSAGE_TIME 2800

namespace WorldMapNS
{

// Initialize the static instance pointer
WorldMap* WorldMap::current_ = nullptr;

/**
 * Reverses the given direction.
 * @param direction The direction to reverse.
 * @return The reversed direction.
 */
Direction reverse_dir(Direction direction)
{
  switch (direction)
  {
    case D_WEST:
    {
      return D_EAST;
    }
    case D_EAST:
    {
      return D_WEST;
    }
    case D_NORTH:
    {
      return D_SOUTH;
    }
    case D_SOUTH:
    {
      return D_NORTH;
    }
    case D_NONE:
    {
      return D_NONE;
    }
  }
  return D_NONE;
}

/**
 * Converts a Direction to a string representation.
 * @param direction The direction to convert.
 * @return The string representation of the direction.
 */
std::string direction_to_string(Direction direction)
{
  switch (direction)
  {
    case D_WEST:
    {
      return "west";
    }
    case D_EAST:
    {
      return "east";
    }
    case D_NORTH:
    {
      return "north";
    }
    case D_SOUTH:
    {
      return "south";
    }
    default:
    {
      return "none";
    }
  }
}

/**
 * Converts a string representation of a direction to Direction.
 * @param directory The string representation of the direction.
 * @return The corresponding Direction.
 */
Direction string_to_direction(const std::string& directory)
{
  if (directory == "west")
  {
    return D_WEST;
  }
  else if (directory == "east")
  {
    return D_EAST;
  }
  else if (directory == "north")
  {
    return D_NORTH;
  }
  else if (directory == "south")
  {
    return D_SOUTH;
  }
  else
  {
    return D_NONE;
  }
}

/**
 * Initializes the TileManager, reading tile data from a file.
 */
TileManager::TileManager()
{
  std::string stwt_filename = datadir + "/images/worldmap/antarctica.stwt";
  lisp_object_t* root_obj = lisp_read_from_file(stwt_filename);

  if (!root_obj)
  {
    st_abort("Couldn't load file", stwt_filename);
  }

  if (strcmp(lisp_symbol(lisp_car(root_obj)), "supertux-worldmap-tiles") == 0)
  {
    lisp_object_t* cur = lisp_cdr(root_obj);
    while (!lisp_nil_p(cur))
    {
      lisp_object_t* element = lisp_car(cur);
      if (strcmp(lisp_symbol(lisp_car(element)), "tile") == 0)
      {
        int id = 0;
        std::string filename = "<invalid>";

        Tile* tile = new Tile;
        tile->north = true;
        tile->east = true;
        tile->south = true;
        tile->west = true;
        tile->stop = true;
        tile->auto_walk = false;

        LispReader reader(lisp_cdr(element));
        reader.read_int("id", &id);
        reader.read_bool("north", &tile->north);
        reader.read_bool("south", &tile->south);
        reader.read_bool("west", &tile->west);
        reader.read_bool("east", &tile->east);
        reader.read_bool("stop", &tile->stop);
        reader.read_bool("auto-walk", &tile->auto_walk);
        reader.read_string("image", &filename);

        std::string temp;
        reader.read_string("one-way", &temp);
        tile->one_way = BOTH_WAYS;
        if (!temp.empty())
        {
          if (temp == "north-south")
          {
            tile->one_way = NORTH_SOUTH_WAY;
          }
          else if (temp == "south-north")
          {
            tile->one_way = SOUTH_NORTH_WAY;
          }
          else if (temp == "east-west")
          {
            tile->one_way = EAST_WEST_WAY;
          }
          else if (temp == "west-east")
          {
            tile->one_way = WEST_EAST_WAY;
          }
        }

        tile->sprite = new Surface(datadir + "/images/worldmap/" + filename, true);
        if (id >= int(tiles.size()))
        {
          tiles.resize(id + 1);
        }
        tiles[id] = tile;
      }
      else
      {
        puts("Unhandled symbol");
      }

      cur = lisp_cdr(cur);
    }
  }
  else
  {
    assert(0);
  }
}

/**
 * Destructor for TileManager, frees all allocated tiles.
 */
TileManager::~TileManager()
{
  for (std::vector<Tile*>::iterator i = tiles.begin(); i != tiles.end(); ++i)
  {
    delete *i;
  }
}

/**
 * Retrieves a tile by its index.
 * @param i The index of the tile.
 * @return The tile at the given index.
 */
Tile* TileManager::get(int i)
{
  assert(i >= 0 && i < int(tiles.size()));
  return tiles[i];
}

/**
 * Constructor for Tux, initializes Tux with world map and sprites.
 * @param worldmap_ The WorldMap to associate with Tux.
 */
Tux::Tux(WorldMap* worldmap_) : worldmap(worldmap_)
{
  largetux_sprite = new Surface(datadir + "/images/worldmap/tux.png", true);
  firetux_sprite = new Surface(datadir + "/images/worldmap/firetux.png", true);
  smalltux_sprite = new Surface(datadir + "/images/worldmap/smalltux.png", true);

  offset = 0;
  moving = false;
  tile_pos.x = worldmap->get_start_x();
  tile_pos.y = worldmap->get_start_y();
  direction = D_NONE;
  input_direction = D_NONE;
}

/**
 * Destructor for Tux, deletes all loaded sprites.
 */
Tux::~Tux()
{
  deleteSprites();
}

/**
 * Loads all Tux sprites.
 */
void Tux::loadSprites()
{
  largetux_sprite = new Surface(datadir + "/images/worldmap/tux.png", true);
  firetux_sprite = new Surface(datadir + "/images/worldmap/firetux.png", true);
  smalltux_sprite = new Surface(datadir + "/images/worldmap/smalltux.png", true);
}

/**
 * Deletes all loaded Tux sprites.
 */
void Tux::deleteSprites()
{
  if (smalltux_sprite)
  {
    delete smalltux_sprite;
  }
  if (firetux_sprite)
  {
    delete firetux_sprite;
  }
  if (largetux_sprite)
  {
    delete largetux_sprite;
  }
  smalltux_sprite = 0;
  firetux_sprite = 0;
  largetux_sprite = 0;
}

/**
 * Draws Tux at the given offset.
 * @param offset The point used to offset the drawing.
 */
void Tux::draw(const Point& offset)
{
  Point pos = get_pos();
  switch (player_status.bonus)
  {
    case PlayerStatus::GROWUP_BONUS:
    {
      largetux_sprite->draw(pos.x + offset.x, pos.y + offset.y - 10);
      break;
    }
    case PlayerStatus::FLOWER_BONUS:
    {
      firetux_sprite->draw(pos.x + offset.x, pos.y + offset.y - 10);
      break;
    }
    case PlayerStatus::NO_BONUS:
    {
      smalltux_sprite->draw(pos.x + offset.x, pos.y + offset.y - 10);
      break;
    }
  }
}

/**
 * Returns the current position of Tux in world map coordinates.
 * @return The position of Tux as a Point object.
 */
Point Tux::get_pos()
{
  float x = tile_pos.x * 32;
  float y = tile_pos.y * 32;

  switch (direction)
  {
    case D_WEST:
    {
      x -= offset - 32;
      break;
    }
    case D_EAST:
    {
      x += offset - 32;
      break;
    }
    case D_NORTH:
    {
      y -= offset - 32;
      break;
    }
    case D_SOUTH:
    {
      y += offset - 32;
      break;
    }
    case D_NONE:
    {
      break;
    }
  }

  return Point((int)x, (int)y);
}

/**
 * Stops Tux's movement.
 */
void Tux::stop()
{
  offset = 0;
  direction = D_NONE;
  moving = false;
}

/**
 * Updates Tux's movement and tile position based on input and world state.
 * @param delta The time delta used to update movement.
 */
void Tux::update(float delta)
{
  if (!moving)
  {
    if (input_direction != D_NONE)
    {
      WorldMapNS::WorldMap::Level* level = worldmap->at_level();

      // We got a new direction, so let's start walking when possible
      Point next_tile;
      if ((!level || level->solved || level->name.empty()) && worldmap->path_ok(input_direction, tile_pos, &next_tile))
      {
        tile_pos = next_tile;
        moving = true;
        direction = input_direction;
        back_direction = reverse_dir(direction);
      }
      else if (input_direction == back_direction)
      {
        moving = true;
        direction = input_direction;
        tile_pos = worldmap->get_next_tile(tile_pos, direction);
        back_direction = reverse_dir(direction);
      }
    }
  }
  else
  {
    // Let Tux walk a few pixels (20 pixels/sec)
    offset += 20.0f * delta;

    if (offset > 32)
    {
      // We reached the next tile, so we check what to do now
      offset -= 32;

      WorldMap::Level* level = worldmap->at_level();
      if (level && level->name.empty() && !level->display_map_message.empty() && level->passive_message)
      {
        // direction and the apply_action_ are opposites, since they "see"
        // directions in a different way
        if ((direction == D_NORTH && level->apply_action_south) ||
            (direction == D_SOUTH && level->apply_action_north) ||
            (direction == D_WEST && level->apply_action_east) ||
            (direction == D_EAST && level->apply_action_west))
        {
          worldmap->passive_message = level->display_map_message;
          worldmap->passive_message_timer.start(DISPLAY_MAP_MESSAGE_TIME);
        }
      }

      Tile* cur_tile = worldmap->at(tile_pos);
      if (cur_tile->stop || (level && (!level->name.empty() || level->teleport_dest_x != -1)))
      {
        stop();
      }
      else
      {
        if (worldmap->at(tile_pos)->auto_walk)
        {
          // Turn to a new direction
          Tile* tile = worldmap->at(tile_pos);
          Direction dir = D_NONE;

          if (tile->north && back_direction != D_NORTH)
          {
            dir = D_NORTH;
          }
          else if (tile->south && back_direction != D_SOUTH)
          {
            dir = D_SOUTH;
          }
          else if (tile->east && back_direction != D_EAST)
          {
            dir = D_EAST;
          }
          else if (tile->west && back_direction != D_WEST)
          {
            dir = D_WEST;
          }

          if (dir != D_NONE)
          {
            direction = dir;
            back_direction = reverse_dir(direction);
          }
          else
          {
            // Should never be reached if tiledata is good
            stop();
            return;
          }
        }

        // Walk automatically to the next tile
        Point next_tile;
        if (worldmap->path_ok(direction, tile_pos, &next_tile))
        {
          tile_pos = next_tile;
        }
        else
        {
          stop();
        }
      }
    }
  }
}

//---------------------------------------------------------------------------
/**
 * Tile constructor, initializes an empty Tile object.
 */
Tile::Tile()
{
}

/**
 * Tile destructor, deletes the tile's sprite.
 */
Tile::~Tile()
{
  delete sprite;
}

//---------------------------------------------------------------------------
/**
 * WorldMap constructor, initializes the world map and loads initial resources.
 */
WorldMap::WorldMap()
{
  current_ = this;
  tile_manager = new TileManager();

  width = (int)(20);
  height = (int)(15);

  start_x = int(4);
  start_y = int(5);

  passive_message_timer.init(true);

  loadSprites();

  map_file = datadir + "/levels/worldmaps/world1.stwm";

  input_direction = D_NONE;
  enter_level = false;

  name = "<no file>";
  music = "salcon.mod";
}

/**
 * WorldMap destructor, cleans up allocated objects and resources.
 */
WorldMap::~WorldMap()
{
  if (current_ == this) current_ = nullptr;
  delete tux;
  delete tile_manager;

  deleteSprites();
  lisp_reset_pool(); // Free all memory used by the worldmap data
}

/**
 * Loads the sprites for the world map.
 */
void WorldMap::loadSprites()
{
  leveldot_green = new Surface(datadir + "/images/worldmap/leveldot_green.png", true);
  leveldot_red = new Surface(datadir + "/images/worldmap/leveldot_red.png", true);
  leveldot_teleporter = new Surface(datadir + "/images/worldmap/teleporter.png", true);
}

/**
 * Deletes the loaded sprites for the world map.
 */
void WorldMap::deleteSprites()
{
  if (leveldot_green)
  {
    delete leveldot_green;
  }
  if (leveldot_red)
  {
    delete leveldot_red;
  }
  if (leveldot_teleporter)
  {
    delete leveldot_teleporter;
  }
  leveldot_green = 0;
  leveldot_red = 0;
  leveldot_teleporter = 0;
}

/**
 * Sets the world map file to be loaded.
 * @param mapfile The name of the map file.
 */
void WorldMap::set_map_file(std::string mapfile)
{
  map_file = datadir + "/levels/worldmaps/" + mapfile;
}

/**
 * Loads the world map from the specified file.
 */
void WorldMap::load_map()
{
  lisp_object_t* root_obj = lisp_read_from_file(map_file);
  if (!root_obj)
  {
    st_abort("Couldn't load file", map_file);
  }

  if (strcmp(lisp_symbol(lisp_car(root_obj)), "supertux-worldmap") == 0)
  {
    lisp_object_t* cur = lisp_cdr(root_obj);

    while (!lisp_nil_p(cur))
    {
      lisp_object_t* element = lisp_car(cur);

      if (strcmp(lisp_symbol(lisp_car(element)), "tilemap") == 0)
      {
        LispReader reader(lisp_cdr(element));
        reader.read_int("width", &width);
        reader.read_int("height", &height);
        reader.read_int_vector("data", &tilemap);
      }
      else if (strcmp(lisp_symbol(lisp_car(element)), "properties") == 0)
      {
        LispReader reader(lisp_cdr(element));
        reader.read_string("name", &name);
        reader.read_string("music", &music);
        reader.read_int("start_pos_x", &start_x);
        reader.read_int("start_pos_y", &start_y);
      }
      else if (strcmp(lisp_symbol(lisp_car(element)), "levels") == 0)
      {
        lisp_object_t* cur_level = lisp_cdr(element);

        while (!lisp_nil_p(cur_level))
        {
          lisp_object_t* level_element = lisp_car(cur_level);

          if (strcmp(lisp_symbol(lisp_car(level_element)), "level") == 0)
          {
            Level level;
            LispReader reader(lisp_cdr(level_element));
            level.solved = false;

            level.north = true;
            level.east = true;
            level.south = true;
            level.west = true;

            reader.read_string("extro-filename", &level.extro_filename);
            reader.read_string("name", &level.name);
            reader.read_int("x", &level.x);
            reader.read_int("y", &level.y);
            reader.read_string("map-message", &level.display_map_message);
            level.auto_path = true;
            reader.read_bool("auto-path", &level.auto_path);
            level.passive_message = true;
            reader.read_bool("passive-message", &level.passive_message);

            level.invisible_teleporter = false;
            level.teleport_dest_x = level.teleport_dest_y = -1;
            reader.read_int("dest_x", &level.teleport_dest_x);
            reader.read_int("dest_y", &level.teleport_dest_y);
            reader.read_string("teleport-message", &level.teleport_message);
            reader.read_bool("invisible-teleporter", &level.invisible_teleporter);

            level.apply_action_north = level.apply_action_south =
                  level.apply_action_east = level.apply_action_west = true;
            reader.read_bool("apply-action-up", &level.apply_action_north);
            reader.read_bool("apply-action-down", &level.apply_action_south);
            reader.read_bool("apply-action-left", &level.apply_action_west);
            reader.read_bool("apply-action-right", &level.apply_action_east);

            reader.read_string("title", &level.title); // read level's title directly

            levels.push_back(level);
          }

          cur_level = lisp_cdr(cur_level);
        }
      }

      cur = lisp_cdr(cur);
    }
  }

  tux = new Tux(this);
}

/**
 * Retrieves and sets the level's title from its file.
 * @param level The level whose title will be retrieved.
 */
void WorldMap::get_level_title(Levels::pointer level)
{
  /** Get level's title */
  level->title = "<no title>";

  // Construct the full path to the level file.
  std::string level_path = datadir + "/levels/" + level->name;

  // Use the fast title reader instead of parsing the whole file.
  level->title = ::Level::get_level_title_fast(level_path);
}
/**
 * Handles pressing the Escape key to show or hide the menu.
 */
void WorldMap::on_escape_press()
{
  // Show or hide the menu
  if (!Menu::current())
  {
    Menu::set_current(worldmap_menu);
  }
  else
  {
    Menu::set_current(0);
  }
}

/**
 * Handles keyboard-specific input events.
 * @param event The SDL_Event for the key press.
 */
void WorldMap::handleKeyboardInput(const SDL_Event& event)
{
  if (event.type == SDL_KEYDOWN)
  {
    switch (event.key.keysym.sym)
    {
      case SDLK_ESCAPE:
      {
        on_escape_press();
        break;
      }
      case SDLK_SPACE: // added for wii mote
      case SDLK_LCTRL:
      case SDLK_RETURN:
      {
        enter_level = true;
        break;
      }
      default:
      {
        break;
      }
    }
  }
}

#ifdef TSCONTROL
/**
 * Handles mouse-specific input events.
 * @param event The SDL_Event for the mouse action.
 */
void WorldMap::handleMouseInput(const SDL_Event& event)
{
  if (event.type == SDL_MOUSEBUTTONDOWN)
  {
    if (event.motion.y < screen->h / 4)
    {
      input_direction = D_NORTH;
    }
    else if (event.motion.y > 3 * screen->h / 4)
    {
      input_direction = D_SOUTH;
    }
    else if (event.motion.x < screen->w / 4)
    {
      input_direction = D_WEST;
    }
    else if (event.motion.x > 3 * screen->w / 4)
    {
      input_direction = D_EAST;
    }
    else
    {
      enter_level = true;
    }
  }
}
#endif

/**
 * Handles joystick-specific input events.
 * @param event The SDL_Event for the joystick action.
 */
void WorldMap::handleJoystickInput(const SDL_Event& event)
{
  switch (event.type)
  {
    case SDL_JOYAXISMOTION:
    {
      if (event.jaxis.axis == joystick_keymap.x_axis)
      {
        if (event.jaxis.value < -joystick_keymap.dead_zone)
        {
          input_direction = D_WEST;
        }
        else if (event.jaxis.value > joystick_keymap.dead_zone)
        {
          input_direction = D_EAST;
        }
      }
      else if (event.jaxis.axis == joystick_keymap.y_axis)
      {
        if (event.jaxis.value > joystick_keymap.dead_zone)
        {
          input_direction = D_SOUTH;
        }
        else if (event.jaxis.value < -joystick_keymap.dead_zone)
        {
          input_direction = D_NORTH;
        }
      }
      break;
    }
    case SDL_JOYHATMOTION:
    {
      if (event.jhat.value == SDL_HAT_UP)
      {
        input_direction = D_NORTH;
      }
      else if (event.jhat.value == SDL_HAT_DOWN)
      {
        input_direction = D_SOUTH;
      }
      else if (event.jhat.value == SDL_HAT_LEFT)
      {
        input_direction = D_WEST;
      }
      else if (event.jhat.value == SDL_HAT_RIGHT)
      {
        input_direction = D_EAST;
      }
      break;
    }
    case SDL_JOYBUTTONDOWN:
    {
      // A button (0) or 2 button (3) will enter level
      if (event.jbutton.button == 0 || event.jbutton.button == 3)
      {
        enter_level = true;
      }
      else if (event.jbutton.button == 6) // mote home
      {
        on_escape_press();
      }
      break;
    }
  }
}

/**
 * Processes player input by polling for events and dispatching to handlers.
 */
void WorldMap::get_input()
{
  enter_level = false;
  input_direction = D_NONE;

  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    if (Menu::current())
    {
      Menu::current()->event(event);
    }
    else
    {
      switch (event.type)
      {
        case SDL_QUIT:
        {
          st_abort("Received window close", "");
          break;
        }
        case SDL_KEYDOWN:
        {
          handleKeyboardInput(event);
          break;
        }
#ifdef TSCONTROL
        case SDL_MOUSEBUTTONDOWN:
        {
          handleMouseInput(event);
          break;
        }
#endif
        case SDL_JOYAXISMOTION:
        case SDL_JOYHATMOTION:
        case SDL_JOYBUTTONDOWN:
        {
          handleJoystickInput(event);
          break;
        }
        default:
        {
          break;
        }
      }
    }
  }

  if (!Menu::current())
  {
    Uint8* keystate = SDL_GetKeyState(NULL);

    if (keystate[SDLK_LEFT])
    {
      input_direction = D_WEST;
    }
    else if (keystate[SDLK_RIGHT])
    {
      input_direction = D_EAST;
    }
    else if (keystate[SDLK_UP])
    {
      input_direction = D_NORTH;
    }
    else if (keystate[SDLK_DOWN])
    {
      input_direction = D_SOUTH;
    }
  }
}

/**
 * Determines the next tile position based on the current position and direction.
 * @param pos The current position.
 * @param direction The direction of movement.
 * @return The next tile position.
 */
Point WorldMap::get_next_tile(Point pos, Direction direction)
{
  switch (direction)
  {
    case D_WEST:
    {
      pos.x -= 1;
      break;
    }
    case D_EAST:
    {
      pos.x += 1;
      break;
    }
    case D_NORTH:
    {
      pos.y -= 1;
      break;
    }
    case D_SOUTH:
    {
      pos.y += 1;
      break;
    }
    case D_NONE:
    {
      break;
    }
  }
  return pos;
}

/**
 * Checks if the path is valid and returns the new tile position if valid.
 * @param direction The direction to move.
 * @param old_pos The current position.
 * @param new_pos The new position to be calculated.
 * @return True if the path is valid, false otherwise.
 */
bool WorldMap::path_ok(Direction direction, Point old_pos, Point* new_pos)
{
  *new_pos = get_next_tile(old_pos, direction);

  if (!(new_pos->x >= 0 && new_pos->x < width && new_pos->y >= 0 && new_pos->y < height))
  { // New position is outside the tilemap
    return false;
  }
  else if (at(*new_pos)->one_way != BOTH_WAYS)
  {
    if ((at(*new_pos)->one_way == NORTH_SOUTH_WAY && direction != D_SOUTH) ||
        (at(*new_pos)->one_way == SOUTH_NORTH_WAY && direction != D_NORTH) ||
        (at(*new_pos)->one_way == EAST_WEST_WAY && direction != D_WEST) ||
        (at(*new_pos)->one_way == WEST_EAST_WAY && direction != D_EAST))
    {
      return false;
    }
    return true;
  }
  else
  { // Check if the tile allows us to go to new_pos
    switch (direction)
    {
      case D_WEST:
      {
        return (at(old_pos)->west && at(*new_pos)->east);
      }
      case D_EAST:
      {
        return (at(old_pos)->east && at(*new_pos)->west);
      }
      case D_NORTH:
      {
        return (at(old_pos)->north && at(*new_pos)->south);
      }
      case D_SOUTH:
      {
        return (at(old_pos)->south && at(*new_pos)->north);
      }
      case D_NONE:
      {
        assert(!"path_ok() can't work if direction is NONE");
      }
    }
    return false;
  }
}

/**
 * Handles the logic for what happens after a level is completed.
 * @param result The exit status from the GameSession.
 * @param coffee Whether Tux had the coffee power-up.
 * @param big Whether Tux was big.
 * @param level A pointer to the level that was just played.
 */
void WorldMap::handleLevelCompletion(GameSession::ExitStatus result, bool coffee, bool big, Level* level)
{
  switch (result)
  {
    case GameSession::ES_LEVEL_FINISHED:
    {
      bool old_level_state = level->solved;
      level->solved = true;

      if (coffee)
      {
        player_status.bonus = PlayerStatus::FLOWER_BONUS;
      }
      else if (big)
      {
        player_status.bonus = PlayerStatus::GROWUP_BONUS;
      }
      else
      {
        player_status.bonus = PlayerStatus::NO_BONUS;
      }

      if (old_level_state != level->solved && level->auto_path)
      { // Try to detect the next direction to which we should walk
        // FIXME: Mostly a hack
        Direction dir = D_NONE;
        Tile* tile = at(tux->get_tile_pos());

        if (tile->north && tux->back_direction != D_NORTH)
        {
          dir = D_NORTH;
        }
        else if (tile->south && tux->back_direction != D_SOUTH)
        {
          dir = D_SOUTH;
        }
        else if (tile->east && tux->back_direction != D_EAST)
        {
          dir = D_EAST;
        }
        else if (tile->west && tux->back_direction != D_WEST)
        {
          dir = D_WEST;
        }

        if (dir != D_NONE)
        {
          tux->set_direction(dir);
        }
#ifdef DEBUG
        std::cout << "Walk to dir: " << dir << std::endl;
#endif
      }

      if (!level->extro_filename.empty())
      {
        unloadsounds();
        MusicRef theme = music_manager->load_music(datadir + "/music/theme.mod");
        MusicRef credits = music_manager->load_music(datadir + "/music/credits.ogg");
        music_manager->play_music(theme);

        // Display final credits and go back to the main menu
        display_text_file(level->extro_filename, "/images/background/extro.jpg", SCROLL_SPEED_MESSAGE);
        music_manager->play_music(credits, 0);
        display_text_file("CREDITS", "/images/background/oiltux.jpg", SCROLL_SPEED_CREDITS);
        music_manager->play_music(theme);
        quit = true;
      }
      break;
    }
    case GameSession::ES_LEVEL_ABORT:
    {
      // Reseting the player_status might be a worthy
      // consideration, but I don't think we need it
      // 'cause only the bad players will use it to
      // 'cheat' a few items and that isn't necesarry a
      // bad thing (ie. better they continue that way,
      // then stop playing the game all together since it
      // is to hard)
      break;
    }
    case GameSession::ES_GAME_OVER:
    {
      /* draw an end screen
       * In the future, this should make a dialog a la SuperMario, asking if the
       * player wants to restart the world map with no score and from level 1
       */
      char str[80];
      drawgradient(Color(0, 255, 0), Color(255, 0, 255));

      blue_text->drawf("GAMEOVER", 0, 200, A_HMIDDLE, A_TOP, 1);

      snprintf(str, sizeof(str), "SCORE: %d", player_status.score);
      gold_text->drawf(str, 0, 224, A_HMIDDLE, A_TOP, 1);

      snprintf(str, sizeof(str), "COINS: %d", player_status.distros);
      gold_text->drawf(str, 0, 256, A_HMIDDLE, A_TOP, 1);

      flipscreen();

      SDL_Event event;
      wait_for_event(event, 2000, 5000, true);

      quit = true;
      player_status.reset();
      break;
    }
    case GameSession::ES_NONE:
    {
      break;
    }
  }
}

/**
 * Updates the world map and manages level interactions.
 * @param delta Time delta for frame update.
 */
void WorldMap::update(float delta)
{
  if (enter_level && !tux->is_moving())
  {
    Level* level = at_level();
    if (level && !level->name.empty())
    {
      if (level->x == tux->get_tile_pos().x && level->y == tux->get_tile_pos().y)
      {
#ifdef DEBUG
        std::cout << "Enter the current level: " << level->name << std::endl;
#endif
        deleteSprites();
        tux->deleteSprites();

        GameSession* session = new GameSession(datadir + "/levels/" + level->name, 1, ST_GL_LOAD_LEVEL_FILE);
        loadsounds();

        GameSession::ExitStatus result = session->run();
        bool coffee = session->get_world()->get_tux()->got_coffee;
        bool big = session->get_world()->get_tux()->size == BIG;
        delete session;
        session = 0;

        handleLevelCompletion(result, coffee, big, level);

        unloadsounds();
        loadSprites();
        tux->loadSprites();
        music_manager->play_music(song);
        Menu::set_current(0);
        if (!savegame_file.empty())
        {
          savegame(savegame_file);
        }
        return;
      }
    }
    else if (level && level->teleport_dest_x != -1 && level->teleport_dest_y != -1)
    {
      if (level->x == tux->get_tile_pos().x && level->y == tux->get_tile_pos().y)
      {
        loadsounds();  // FIXME: only doing it here because world bonus map warp sound
        play_sound(sounds[SND_TELEPORT], SOUND_CENTER_SPEAKER);
        tux->back_direction = D_NONE;
        tux->set_tile_pos(Point(level->teleport_dest_x, level->teleport_dest_y));
        SDL_Delay(800);  // Delay for visual effect & sound completion before unloading
        unloadsounds();  // FIXME: ideally should load/unload when loading world maps
      }
    }
    else
    {
      std::cout << "Nothing to enter at: " << tux->get_tile_pos().x << ", " << tux->get_tile_pos().y << std::endl;
    }
  }
  else
  {
    tux->update(delta);
    tux->set_direction(input_direction);
  }

  Menu* menu = Menu::current();
  if (menu)
  {
    menu->action();
  }
}

/**
 * Returns the Tile at the given position.
 * @param p The point to get the tile at.
 * @return The Tile at the given point.
 */
Tile* WorldMap::at(Point p)
{
  assert(p.x >= 0 && p.x < width && p.y >= 0 && p.y < height);
  return tile_manager->get(tilemap[width * p.y + p.x]);
}

/**
 * Returns the Level at the current position.
 * @return The Level object at the current Tux position or null if no level is present.
 */
WorldMap::Level* WorldMap::at_level()
{
  for (Levels::iterator i = levels.begin(); i != levels.end(); ++i)
  {
    if (i->x == tux->get_tile_pos().x && i->y == tux->get_tile_pos().y)
    {
      return &*i;
    }
  }

  return 0;
}

/**
 * Draws the world map at the specified offset.
 * @param offset The point used to offset drawing on the screen.
 */
void WorldMap::draw(const Point& offset)
{
  // Determine the range of tiles visible on the screen
  int x_start = -offset.x / 32;
  int y_start = -offset.y / 32;
  int x_end = x_start + (screen->w / 32) + 2;
  int y_end = y_start + (screen->h / 32) + 2;

  // Clamp the tile range to the map's actual boundaries
  if (x_start < 0) x_start = 0;
  if (y_start < 0) y_start = 0;
  if (x_end > width) x_end = width;
  if (y_end > height) y_end = height;


  // Only draw the visible tiles
  for (int y = y_start; y < y_end; ++y)
  {
    for (int x = x_start; x < x_end; ++x)
    {
      Tile* tile = at(Point(x, y));
      tile->sprite->draw(x * 32 + offset.x, y * 32 + offset.y);
    }
  }

  // Only draw visible level dots
  for (Levels::iterator i = levels.begin(); i != levels.end(); ++i)
  {
    // Only draw dots within the visible tile range
    if (i->x >= x_start && i->x < x_end && i->y >= y_start && i->y < y_end)
    {
      if (i->name.empty())
      {
        if ((i->teleport_dest_x != -1) && !i->invisible_teleporter)
        {
          leveldot_teleporter->draw(i->x * 32 + offset.x, i->y * 32 + offset.y);
        }
      }
      else if (i->solved)
      {
        leveldot_green->draw(i->x * 32 + offset.x, i->y * 32 + offset.y);
      }
      else
      {
        leveldot_red->draw(i->x * 32 + offset.x, i->y * 32 + offset.y);
      }
    }
  }

  tux->draw(offset);
  draw_status();
}

/**
 * Draws the player's current score, coins, and lives status.
 */
void WorldMap::draw_status()
{
  // Draw the shared HUD elements
  draw_player_hud();

  // Draw elements specific to the world map
  if (!tux->is_moving())
  {
    for (Levels::iterator i = levels.begin(); i != levels.end(); ++i)
    {
      if (i->x == tux->get_tile_pos().x && i->y == tux->get_tile_pos().y)
      {
        if (!i->name.empty())
        {
          white_text->draw_align(i->title.c_str(), screen->w / 2, screen->h - offset_y, A_HMIDDLE, A_BOTTOM);
        }
        else if (i->teleport_dest_x != -1)
        {
          if (!i->teleport_message.empty())
          {
            gold_text->draw_align(i->teleport_message.c_str(), screen->w / 2, screen->h - offset_y, A_HMIDDLE, A_BOTTOM);
          }
        }

        /* Display a message in the map, if any as been selected */
        if (!i->display_map_message.empty() && !i->passive_message)
        {
          gold_text->draw_align(i->display_map_message.c_str(), screen->w / 2, screen->h - 30, A_HMIDDLE, A_BOTTOM);
        }

        break;
      }
    }
  }

  /* Display a passive message in the map, if needed */
  if (passive_message_timer.check())
  {
    gold_text->draw_align(passive_message.c_str(), screen->w / 2, screen->h - 30, A_HMIDDLE, A_BOTTOM);
  }
}

/**
 * Handles all input processing for a single frame.
 */
void WorldMap::processInput()
{
  get_input();
}

/**
 * Handles all scene update logic for a single frame.
 * @param deltaTime The time elapsed since the last frame.
 */
void WorldMap::updateScene(float deltaTime)
{
  update(deltaTime);
}

/**
 * Handles all rendering for a single frame.
 */
void WorldMap::renderScene()
{
  Point tux_pos = tux->get_pos();
  offset.x = -tux_pos.x + screen->w / 2;
  offset.y = -tux_pos.y + screen->h / 2;

  if (offset.x > 0)
  {
    offset.x = 0;
  }
  if (offset.y > 0)
  {
    offset.y = 0;
  }

  if (offset.x < screen->w - width * 32)
  {
    offset.x = screen->w - width * 32;
  }
  if (offset.y < screen->h - height * 32)
  {
    offset.y = screen->h - height * 32;
  }

  draw(offset);

#ifndef TSCONTROL
  if (Menu::current())
  {
    Menu::current()->draw();
    mouse_cursor->draw();
  }
#else
  if (Menu::current())
  {
    Menu::current()->draw();
  }
  if (show_mouse)
  {
    mouse_cursor->draw();
  }
#endif
  flipscreen();
}

/**
 * Displays the world map and handles the main update loop.
 */
void WorldMap::display()
{
  Menu::set_current(0);
  quit = false;

  song = music_manager->load_music(datadir + "/music/" + music);
  music_manager->play_music(song);

  unsigned int last_update_time = Ticks::get();

  while (!quit)
  {
    unsigned int update_time = Ticks::get();
    float delta = ((float)(update_time - last_update_time)) / 100.0f;
    delta *= 1.3f;

    if (delta > 10.0f)
    {
      delta = 0.3f;
    }
    last_update_time = update_time;

    processInput();
    updateScene(delta);
    renderScene();
  }
}

/**
 * Saves the game state to the specified file.
 * @param filename The name of the file to save the game state.
 */
void WorldMap::savegame(const std::string& filename)
{
#ifdef DEBUG
  std::cout << "savegame: " << filename << std::endl;
#endif
  std::ofstream out(filename.c_str());

  int nb_solved_levels = 0;
  for (Levels::iterator i = levels.begin(); i != levels.end(); ++i)
  {
    if (i->solved)
    {
      ++nb_solved_levels;
    }
  }

  out << "(supertux-savegame\n"
      << "  (version 1)\n"
      << "  (title  \"" << name << " - " << nb_solved_levels << "/" << levels.size() << "\")\n"
      << "  (lives   " << player_status.lives << ")\n"
      << "  (score   " << player_status.score << ")\n"
      << "  (distros " << player_status.distros << ")\n"
      << "  (tux (x " << tux->get_tile_pos().x << ") (y " << tux->get_tile_pos().y << ")\n"
      << "       (back \"" << direction_to_string(tux->back_direction) << "\")\n"
      << "       (bonus \"" << bonus_to_string(player_status.bonus) <<  "\"))\n"
      << "  (levels\n";

  for (Levels::iterator i = levels.begin(); i != levels.end(); ++i)
  {
    if (i->solved && !i->name.empty())
    {
      out << "     (level (name \"" << i->name << "\")\n"
          << "            (solved #t))\n";
    }
  }

  out << "   )\n"
      << " )\n\n;; EOF ;;" << std::endl;
}

/**
 * Loads the game state from the specified file.
 * @param filename The name of the file to load the game state.
 */
void WorldMap::loadgame(const std::string& filename)
{
#ifdef DEBUG
  std::cout << "loadgame: " << filename << std::endl;
#endif
  savegame_file = filename;

  lisp_object_t* savegame = lisp_read_from_file(filename);
  if (!savegame)
  {
    // Reset player state for a new game
#ifdef DEBUG
    std::cout << "WorldMap:loadgame: File not found: " << filename << std::endl;
#endif
    player_status.reset();
    return;
  }

  lisp_object_t* cur = savegame;

  if (strcmp(lisp_symbol(lisp_car(cur)), "supertux-savegame") != 0)
  {
    lisp_free(savegame);
    return;
  }

  cur = lisp_cdr(cur);
  LispReader reader(cur);

  reader.read_int("lives", &player_status.lives);
  reader.read_int("score", &player_status.score);
  reader.read_int("distros", &player_status.distros);

  if (player_status.lives < 0)
  {
    player_status.lives = START_LIVES;
  }

  lisp_object_t* tux_cur = 0;
  if (reader.read_lisp("tux", &tux_cur))
  {
    Point p;
    std::string back_str = "none";
    std::string bonus_str = "none";

    LispReader tux_reader(tux_cur);
    tux_reader.read_int("x", &p.x);
    tux_reader.read_int("y", &p.y);
    tux_reader.read_string("back", &back_str);
    tux_reader.read_string("bonus", &bonus_str);

    player_status.bonus = string_to_bonus(bonus_str);
    tux->back_direction = string_to_direction(back_str);
    tux->set_tile_pos(p);
  }

  lisp_object_t* level_cur = 0;
  if (reader.read_lisp("levels", &level_cur))
  {
    while (level_cur)
    {
      lisp_object_t* sym  = lisp_car(lisp_car(level_cur));
      lisp_object_t* data = lisp_cdr(lisp_car(level_cur));

      if (strcmp(lisp_symbol(sym), "level") == 0)
      {
        std::string name;
        bool solved = false;

        LispReader level_reader(data);
        level_reader.read_string("name", &name);
        level_reader.read_bool("solved", &solved);

        for (Levels::iterator i = levels.begin(); i != levels.end(); ++i)
        {
          if (name == i->name)
          {
            i->solved = solved;
          }
        }
      }

      level_cur = lisp_cdr(level_cur);
    }
  }

  lisp_free(savegame);
}

/**
 * Loads a world map from the specified file.
 * @param filename The name of the file to load the world map.
 */
void WorldMap::loadmap(const std::string& filename)
{
  savegame_file = "";
  set_map_file(filename);
  load_map();
}

/**
 * A lightweight "peek" function that reads just enough of a worldmap file
 * to extract its title, avoiding a full parse. This is much faster than
 * loading the entire map just to display its name in a menu.
 * @param mapfile_path The full path to the .stwm file.
 * @return The title of the worldmap.
 */
std::string WorldMap::get_world_title_fast(const std::string& mapfile_path)
{
  return get_title_from_lisp_file(mapfile_path, "Invalid Worldmap", "Untitled Worldmap");
}

} // namespace WorldMapNS

// EOF
