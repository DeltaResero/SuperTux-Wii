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

void SpatialGrid::get_overlapping_cells(float x, float y, float w, float h,
                                         std::vector<CellKey>& out_cells) const
{
  out_cells.clear();

  CellKey min_cell = get_cell(x, y);
  CellKey max_cell = get_cell(x + w, y + h);

  for (int cy = min_cell.y; cy <= max_cell.y; ++cy)
  {
    for (int cx = min_cell.x; cx <= max_cell.x; ++cx)
    {
      out_cells.push_back(CellKey{cx, cy});
    }
  }
}

void SpatialGrid::add_badguy(BadGuy* badguy)
{
  if (!badguy) return;

  // Add to global list for unbounded queries
  all_badguys.push_back(badguy);

  // Add to grid cells for bounded queries
  std::vector<CellKey> cells;
  get_overlapping_cells(badguy->base.x, badguy->base.y,
                        badguy->base.width, badguy->base.height, cells);

  for (const auto& cell_key : cells)
  {
    grid[cell_key].badguys.push_back(badguy);
  }
}

void SpatialGrid::add_bullet(Bullet* bullet)
{
  if (!bullet) return;

  all_bullets.push_back(bullet);

  std::vector<CellKey> cells;
  get_overlapping_cells(bullet->base.x, bullet->base.y,
                        bullet->base.width, bullet->base.height, cells);

  for (const auto& cell_key : cells)
  {
    grid[cell_key].bullets.push_back(bullet);
  }
}

void SpatialGrid::add_upgrade(Upgrade* upgrade)
{
  if (!upgrade) return;

  all_upgrades.push_back(upgrade);

  std::vector<CellKey> cells;
  get_overlapping_cells(upgrade->base.x, upgrade->base.y,
                        upgrade->base.width, upgrade->base.height, cells);

  for (const auto& cell_key : cells)
  {
    grid[cell_key].upgrades.push_back(upgrade);
  }
}

std::vector<BadGuy*> SpatialGrid::query_badguys(float x, float y, float w, float h) const
{
  std::vector<BadGuy*> result;
  result.reserve(20); // Typical screen has ~10-20 visible badguys

  std::vector<CellKey> cells;
  get_overlapping_cells(x, y, w, h, cells);

  // Collect all badguys from overlapping cells
  // Note: Same badguy may appear multiple times if it spans cells
  for (const auto& cell_key : cells)
  {
    auto it = grid.find(cell_key);
    if (it != grid.end())
    {
      const auto& cell_badguys = it->second.badguys;
      result.insert(result.end(), cell_badguys.begin(), cell_badguys.end());
    }
  }

  return result;
}

std::vector<Upgrade*> SpatialGrid::query_upgrades(float x, float y, float w, float h) const
{
  std::vector<Upgrade*> result;
  result.reserve(10);

  std::vector<CellKey> cells;
  get_overlapping_cells(x, y, w, h, cells);

  for (const auto& cell_key : cells)
  {
    auto it = grid.find(cell_key);
    if (it != grid.end())
    {
      const auto& cell_upgrades = it->second.upgrades;
      result.insert(result.end(), cell_upgrades.begin(), cell_upgrades.end());
    }
  }

  return result;
}

// EOF
