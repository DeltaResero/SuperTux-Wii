//  type.cpp
//
//  SuperTux
//  Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
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

#include "SDL_image.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include "setup.h"
#include "globals.h"
#include "screen.h"
#include "type.h"
#include "scene.h"

/**
 * Initializes a string list.
 * Sets up the initial state of the list with no items and no active item.
 * @param pstring_list Pointer to the string list to initialize.
 */
void string_list_init(string_list_type* pstring_list)
{
  pstring_list->num_items = 0;
  pstring_list->active_item = -1;
  pstring_list->item = nullptr;
}

/**
 * Returns the active string in the list.
 * If no item is active, returns an empty string.
 * @param pstring_list Pointer to the string list.
 * @return const char* The active string, or an empty string if none.
 */
const char* string_list_active(string_list_type* pstring_list)
{
  if (pstring_list == nullptr || pstring_list->active_item == -1)
    return "";

  return pstring_list->item[pstring_list->active_item];
}

/**
 * Adds a string to the list.
 * Allocates memory for the new string and adds it to the list. If no item is active, the new item becomes active.
 * @param pstring_list Pointer to the string list.
 * @param str The string to add.
 */
void string_list_add_item(string_list_type* pstring_list, const char* str)
{
  size_t max_length = 256;  // Define a maximum length for safety
  size_t str_len = strnlen(str, max_length);

  // Allocate memory for the new string, ensuring null termination
  char* pnew_string = new char[str_len + 1];
  std::strncpy(pnew_string, str, str_len);
  pnew_string[str_len] = '\0';  // Ensure null termination

  // Increase the number of items
  ++pstring_list->num_items;

  // Reallocate the item array to hold the new string
  pstring_list->item = (char**) std::realloc(pstring_list->item, sizeof(char*) * pstring_list->num_items);

  // Add the new string to the end of the list
  pstring_list->item[pstring_list->num_items - 1] = pnew_string;

  // If no item is active, set the new item as active
  if (pstring_list->active_item == -1)
    pstring_list->active_item = 0;
}

/**
 * Copies one string list to another.
 * Frees any existing strings in the destination list and copies all strings from the source list.
 * @param pstring_list Destination string list.
 * @param pstring_list_orig Source string list.
 */
void string_list_copy(string_list_type* pstring_list, string_list_type pstring_list_orig)
{
  // Free the current list
  string_list_free(pstring_list);

  // Copy each item from the original list
  for (int i = 0; i < pstring_list_orig.num_items; ++i)
  {
    string_list_add_item(pstring_list, pstring_list_orig.item[i]);
  }
}

/**
 * Finds a string in the list.
 * Searches for the given string and returns its index. Returns -1 if not found.
 * @param pstring_list Pointer to the string list.
 * @param str The string to search for.
 * @return int The index of the string, or -1 if not found.
 */
int string_list_find(string_list_type* pstring_list, const char* str)
{
  for (int i = 0; i < pstring_list->num_items; ++i)
  {
    if (std::strcmp(pstring_list->item[i], str) == 0)
    {
      return i;
    }
  }
  return -1;
}

/**
 * Sorts the strings in the list alphabetically.
 * Uses std::sort to sort the strings in ascending order.
 * @param pstring_list Pointer to the string list.
 */
void string_list_sort(string_list_type* pstring_list)
{
  std::sort(pstring_list->item, pstring_list->item + pstring_list->num_items, [](const char* a, const char* b) {
    return std::strcmp(a, b) < 0;
  });
}

/**
 * Frees the memory allocated for the string list.
 * This function deallocates all memory associated with the list and resets its state.
 * @param pstring_list Pointer to the string list.
 */
void string_list_free(string_list_type* pstring_list)
{
  if (pstring_list != nullptr)
  {
    // Free each string
    for (int i = 0; i < pstring_list->num_items; ++i)
    {
      delete[] pstring_list->item[i];
    }

    // Free the item array
    std::free(pstring_list->item);

    // Reset the list
    pstring_list->item = nullptr;
    pstring_list->num_items = 0;
    pstring_list->active_item = -1;
  }
}

// EOF
