// src/spatial_grid.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_SPATIAL_GRID_H
#define SUPERTUX_SPATIAL_GRID_H

#include <vector>
#include <unordered_map>
#include "defines.hpp"

class BadGuy;
class Bullet;
class Upgrade;

/**
 * Spatial hash grid for efficient collision detection.
 * Divides the world into cells and tracks which entities are in each cell.
 */
class SpatialGrid
{
public:
  explicit SpatialGrid(int cell_size = 128);
  ~SpatialGrid() = default;

  // Add entities to the grid (called each frame)
  void add_badguy(BadGuy* badguy);
  void add_bullet(Bullet* bullet);
  void add_upgrade(Upgrade* upgrade);

  // Clear all entities (called at frame start)
  void clear();

  // Query entities in cells overlapping the given rectangle
  // Used for normal bounded collision checks
  std::vector<BadGuy*> query_badguys(float x, float y, float w, float h) const;
  std::vector<Upgrade*> query_upgrades(float x, float y, float w, float h) const;

  // Query ALL entities regardless of position
  // Used for Mr. Iceblock off-screen collisions
  const std::vector<BadGuy*>& get_all_badguys() const { return all_badguys; }
  const std::vector<Bullet*>& get_all_bullets() const { return all_bullets; }

private:
  struct CellKey {
    int x, y;

    bool operator==(const CellKey& other) const {
      return x == other.x && y == other.y;
    }
  };

  struct CellKeyHash {
    size_t operator()(const CellKey& key) const {
      // Simple but effective hash for 2D grid coordinates
      return (static_cast<size_t>(key.x) * 73856093) ^
             (static_cast<size_t>(key.y) * 19349663);
    }
  };

  struct Cell {
    std::vector<BadGuy*> badguys;
    std::vector<Bullet*> bullets;
    std::vector<Upgrade*> upgrades;
  };

  int cell_size;
  std::unordered_map<CellKey, Cell, CellKeyHash> grid;

  // Store ALL entities for unbounded queries (Mr. Iceblock case)
  std::vector<BadGuy*> all_badguys;
  std::vector<Bullet*> all_bullets;
  std::vector<Upgrade*> all_upgrades;

  // Reusable scratch buffer for cell queries
  mutable std::vector<CellKey> m_temp_cells;

  // Helper to convert world coordinates to cell coordinates
  CellKey get_cell(float x, float y) const;

  // Get all cells that overlap the given rectangle
  const std::vector<CellKey>& get_overlapping_cells(float x, float y, float w, float h) const;
};

#endif // SUPERTUX_SPATIAL_GRID_H

// EOF
