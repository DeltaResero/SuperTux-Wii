// src/object_pool.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_OBJECT_POOL_H
#define SUPERTUX_OBJECT_POOL_H

#include <vector>
#include <cstddef>

/**
 * High-performance, cache-friendly, contiguous memory object pool.
 *
 * Design principles for Wii optimization:
 * 1. Single Allocation: A std::vector<T> allocates all objects in one
 *    contiguous block of memory, eliminating heap fragmentation and costly
 *    bursts of 'new' calls.
 * 2. Index-Based Management: Uses vectors of indices (size_t) for the free
 *    and active lists, avoiding pointer indirection and ensuring perfect
 *    cache locality during iteration.
 */
template<typename T>
class ObjectPool
{
private:
  std::vector<T> m_pool;             // The single, contiguous allocation for all objects.
  std::vector<size_t> m_freeIndices; // A stack of indices pointing to available objects in m_pool.
  std::vector<size_t> m_activeIndices; // A tightly packed list of indices for in-use objects.

public:
  explicit ObjectPool(size_t size)
  {
    m_pool.resize(size); // Perform the single, large allocation.
    m_freeIndices.reserve(size);
    m_activeIndices.reserve(size);

    // Populate the free list with all indices initially.
    for (size_t i = 0; i < size; ++i)
    {
      m_freeIndices.push_back(i);
    }
  }

  ~ObjectPool() = default; // The vectors will handle their own memory.

  // Disable copying
  ObjectPool(const ObjectPool&) = delete;
  ObjectPool& operator=(const ObjectPool&) = delete;

  T* acquire()
  {
    if (m_freeIndices.empty())
    {
      return nullptr; // Pool is exhausted.
    }

    // Get an index from the free list.
    size_t index = m_freeIndices.back();
    m_freeIndices.pop_back();

    // Add it to the active list.
    m_activeIndices.push_back(index);

    // Get the object, reset its state, and return a pointer to it.
    T* obj = &m_pool[index];
    obj->removable = false;
    return obj;
  }

  void updateAndCleanup(float elapsed_time)
  {
    for (size_t i = 0; i < m_activeIndices.size(); )
    {
      size_t index = m_activeIndices[i];
      T& obj = m_pool[index];
      obj.action(elapsed_time);

      if (obj.removable)
      {
        // Return the index to the free list.
        m_freeIndices.push_back(index);

        // Remove the index from the active list using swap-and-pop.
        m_activeIndices[i] = m_activeIndices.back();
        m_activeIndices.pop_back();
        // Do not increment 'i', as we need to process the swapped-in element.
      }
      else
      {
        ++i;
      }
    }
  }

  void clear()
  {
    m_activeIndices.clear();
    m_freeIndices.clear();
    m_freeIndices.reserve(m_pool.size());
    for (size_t i = 0; i < m_pool.size(); ++i)
    {
      m_freeIndices.push_back(i);
    }
  }

  // --- Accessors for high-performance iteration ---

  // Provides direct, cache-friendly access to the object arena.
  const std::vector<T>& get_pool_data() const
  {
    return m_pool;
  }

  // Provides a mutable pointer to an object at a given index.
  // This is necessary for external modification (e.g., collision handling).
  T* get_object_at(size_t index)
  {
      return &m_pool[index];
  }

  // Provides the list of active indices to iterate over.
  const std::vector<size_t>& get_active_indices() const
  {
    return m_activeIndices;
  }
};

#endif // SUPERTUX_OBJECT_POOL_H

// EOF
