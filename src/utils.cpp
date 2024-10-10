// utils.cpp
//
//  SuperTux
//  Copyright (C) 2024 DeltaResero
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

#include "utils.h"
#include <cstring> // For memcpy, memchr

size_t strlcpy(char* dst, const char* src, size_t size)
{
  if (dst == nullptr || src == nullptr)
  {
    return 0; // Invalid pointers
  }

  // Use memchr to find the null-terminator within the given size limit
  const char* null_terminator = static_cast<const char*>(std::memchr(src, '\0', size));
  size_t src_len = null_terminator ? (null_terminator - src) : size - 1;  // If no null-terminator, copy up to size - 1

  if (size > 0)
  {
    // Copy the appropriate amount, leaving room for the null-terminator
    size_t copy_len = (src_len >= size) ? size - 1 : src_len;

    // Perform the copy
    std::memcpy(dst, src, copy_len);

    // Always null-terminate
    dst[copy_len] = '\0';
  }

  // Return the total length of the source string (or the truncated size if no null-terminator was found)
  return src_len;
}

// EOF
