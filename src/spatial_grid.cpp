// src/spatial_grid.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "spatial_grid.hpp"
#include "badguy.hpp"
#include "special.hpp"
#include <cmath>

SpatialGrid::SpatialGrid(int cell_size_)
  : cell_size(cell_size_)
{
  // Reserve space to avoid reallocation during gameplay
  all_badguys.reserve(100);
  all_bullets.reserve(20);
  all_upgrades.reserve(30);
  m_temp_cells.reserve(9); // Typically 3x3 max for normal sized objects

  // Reserve cache space
  m_query_cache_badguys.reserve(32);
  m_query_cache_upgrades.reserve(16);
}

void SpatialGrid::clear()
{
  grid.clear();
  all_badguys.clear();
  all_bullets.clear();
  all_upgrades.clear();
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

  // Add to global list for unbounded queries
  all_badguys.push_back(badguy);

  // Add to grid cells for bounded queries
  const auto& cells = get_overlapping_cells(badguy->base.x, badguy->base.y,
                                            badguy->base.width, badguy->base.height);

  for (const auto& cell_key : cells)
  {
    grid[cell_key].badguys.push_back(badguy);
  }
}

void SpatialGrid::add_bullet(Bullet* bullet)
{
  if (!bullet) return;

  all_bullets.push_back(bullet);

  const auto& cells = get_overlapping_cells(bullet->base.x, bullet->base.y,
                                            bullet->base.width, bullet->base.height);

  for (const auto& cell_key : cells)
  {
    grid[cell_key].bullets.push_back(bullet);
  }
}

void SpatialGrid::add_upgrade(Upgrade* upgrade)
{
  if (!upgrade) return;

  all_upgrades.push_back(upgrade);

  const auto& cells = get_overlapping_cells(upgrade->base.x, upgrade->base.y,
                                            upgrade->base.width, upgrade->base.height);

  for (const auto& cell_key : cells)
  {
    grid[cell_key].upgrades.push_back(upgrade);
  }
}

const std::vector<BadGuy*>& SpatialGrid::query_badguys(float x, float y, float w, float h) const
{
  m_query_cache_badguys.clear();

  const auto& cells = get_overlapping_cells(x, y, w, h);

  // Collect all badguys from overlapping cells
  // Note: Same badguy may appear multiple times if it spans cells
  for (const auto& cell_key : cells)
  {
    auto it = grid.find(cell_key);
    if (it != grid.end())
    {
      const auto& cell_badguys = it->second.badguys;
      m_query_cache_badguys.insert(m_query_cache_badguys.end(),
                                   cell_badguys.begin(),
                                   cell_badguys.end());
    }
  }

  return m_query_cache_badguys;
}

const std::vector<Upgrade*>& SpatialGrid::query_upgrades(float x, float y, float w, float h) const
{
  m_query_cache_upgrades.clear();

  const auto& cells = get_overlapping_cells(x, y, w, h);

  for (const auto& cell_key : cells)
  {
    auto it = grid.find(cell_key);
    if (it != grid.end())
    {
      const auto& cell_upgrades = it->second.upgrades;
      m_query_cache_upgrades.insert(m_query_cache_upgrades.end(),
                                    cell_upgrades.begin(),
                                    cell_upgrades.end());
    }
  }

  return m_query_cache_upgrades;
}

// EOF
