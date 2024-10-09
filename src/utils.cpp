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

// Define the strlcpy function in utils.cpp
size_t strlcpy(char* dst, const char* src, size_t size)
{
  if (dst == nullptr || src == nullptr)
  {
    return 0; // Invalid pointers
  }

  size_t src_len = std::strlen(src);

  if (size > 0)
  {
    // Ensure we don't copy more than the destination size - 1 to leave space for null terminator
    size_t copy_len = (src_len >= size) ? size - 1 : src_len;

    // Perform the copy
    std::memcpy(dst, src, copy_len);

    // Always null-terminate
    dst[copy_len] = '\0';
  }

  // Return the total length of the source string (not truncated)
  return src_len;
}

// EOF
