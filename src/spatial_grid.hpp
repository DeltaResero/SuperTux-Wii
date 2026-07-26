// src/spatial_grid.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2025-2026 DeltaResero
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

/**
 * Spatial hash grid for efficient collision detection.
 * Divides the world into cells and tracks which badguys are in each cell.
 *
 * Only badguys are stored. Bullets and upgrades are always the side that
 * asks the question rather than the side being searched for, so putting
 * them in the grid would cost work every frame that nothing reads back.
 */
class SpatialGrid
{
public:
  explicit SpatialGrid(int cell_size = 128);
  ~SpatialGrid() = default;

  // Add badguys to the grid (called each frame)
  void add_badguy(BadGuy* badguy);

  // Clear all badguys (called at frame start)
  void clear();

  // Query badguys in cells overlapping the given rectangle
  const std::vector<BadGuy*>& query_badguys(float x, float y, float w, float h) const;

private:
  struct CellKey {
    int x;
    int y;

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
  };

  int cell_size;
  std::unordered_map<CellKey, Cell, CellKeyHash> grid;

  // Reusable scratch buffer for cell queries
  mutable std::vector<CellKey> m_temp_cells;

  // Query cache to prevent heap allocation churn
  mutable std::vector<BadGuy*> m_query_cache_badguys;

  // Helper to convert world coordinates to cell coordinates
  CellKey get_cell(float x, float y) const;

  // Get all cells that overlap the given rectangle
  const std::vector<CellKey>& get_overlapping_cells(float x, float y, float w, float h) const;
};

#endif // SUPERTUX_SPATIAL_GRID_H

// EOF
