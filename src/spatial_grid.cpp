// src/spatial_grid.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "spatial_grid.hpp"
#include "badguy.hpp"
#include <cmath>

SpatialGrid::SpatialGrid(int cell_size_)
  : cell_size(cell_size_)
{
  // Reserve space to avoid reallocation during gameplay
  m_temp_cells.reserve(9); // Typically 3x3 max for normal sized objects

  // Reserve cache space
  m_query_cache_badguys.reserve(32);
}

void SpatialGrid::clear()
{
  // Empty the per-cell badguy lists but keep the cells (and their vectors'
  // capacity) alive. Destroying the map every frame would force each occupied
  // cell to reallocate its vector from scratch on every rebuild, which is
  // exactly the malloc churn this engine is designed to avoid on Wii.
  // The map only grows to the set of cells ever occupied in the level, so the
  // retained memory is small and bounded.
  for (auto& entry : grid)
  {
    entry.second.badguys.clear();
  }
}

SpatialGrid::CellKey SpatialGrid::get_cell(float x, float y) const
{
  return CellKey{
    static_cast<int>(std::floor(x / cell_size)),
    static_cast<int>(std::floor(y / cell_size))
  };
}

const std::vector<SpatialGrid::CellKey>& SpatialGrid::get_overlapping_cells(
    float x, float y, float w, float h) const
{
  m_temp_cells.clear();

  CellKey min_cell = get_cell(x, y);
  CellKey max_cell = get_cell(x + w, y + h);

  for (int cy = min_cell.y; cy <= max_cell.y; ++cy)
  {
    for (int cx = min_cell.x; cx <= max_cell.x; ++cx)
    {
      m_temp_cells.push_back(CellKey{cx, cy});
    }
  }

  return m_temp_cells;
}

void SpatialGrid::add_badguy(BadGuy* badguy)
{
  if (!badguy) return;

  // Add to grid cells for bounded queries
  const auto& cells = get_overlapping_cells(badguy->base.x, badguy->base.y,
                                            badguy->base.width, badguy->base.height);

  for (const auto& cell_key : cells)
  {
    grid[cell_key].badguys.push_back(badguy);
  }
}

const std::vector<BadGuy*>& SpatialGrid::query_badguys(float x, float y, float w, float h) const
{
  m_query_cache_badguys.clear();

  const auto& cells = get_overlapping_cells(x, y, w, h);

  // Collect all badguys from overlapping cells.
  // An entity spanning multiple cells is registered once per cell, so
  // deduplicate while preserving first-occurrence order. Duplicates would
  // make callers process the same collision pair more than once per frame,
  // which breaks toggle-style responses (e.g. direction flips).
  // The result set is small, so a linear membership check is cheapest here.
  for (const auto& cell_key : cells)
  {
    auto it = grid.find(cell_key);
    if (it == grid.end())
    {
      continue;
    }

    for (BadGuy* badguy : it->second.badguys)
    {
      bool already_present = false;
      for (BadGuy* existing : m_query_cache_badguys)
      {
        if (existing == badguy)
        {
          already_present = true;
          break;
        }
      }
      if (!already_present)
      {
        m_query_cache_badguys.push_back(badguy);
      }
    }
  }

  return m_query_cache_badguys;
}

// EOF
