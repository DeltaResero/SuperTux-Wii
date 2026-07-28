// src/lispreader.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux - lispreader.h
// Copyright (C) 1998-2000 Mark Probst
// Copyright (C) 2002 Ingo Ruhnke <grumbel@gmx.de>
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef __LISPREADER_H__
#define __LISPREADER_H__

#include <stdio.h>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

// Stream types for handling file and string streams
inline constexpr int LISP_STREAM_FILE       = 1;
inline constexpr int LISP_STREAM_STRING     = 2;

// Lisp object types
inline constexpr int LISP_TYPE_INTERNAL      = -3;
inline constexpr int LISP_TYPE_PARSE_ERROR   = -2;
inline constexpr int LISP_TYPE_EOF           = -1;
inline constexpr int LISP_TYPE_NIL           = 0;
inline constexpr int LISP_TYPE_SYMBOL        = 1;
inline constexpr int LISP_TYPE_INTEGER       = 2;
inline constexpr int LISP_TYPE_STRING        = 3;
inline constexpr int LISP_TYPE_REAL          = 4;
inline constexpr int LISP_TYPE_CONS          = 5;
inline constexpr int LISP_TYPE_BOOLEAN       = 7;

// Structure defining Lisp stream types
typedef struct
{
  int type;

  union
  {
    FILE *file;
    struct
    {
      const char *buf;
      size_t pos;
      size_t len;
    } string;
  } v;
}
lisp_stream_t;

// Structure defining a Lisp object
typedef struct _lisp_object_t lisp_object_t;
struct _lisp_object_t
{
  int type;

  union
  {
    struct
    {
      struct _lisp_object_t *car;
      struct _lisp_object_t *cdr;
    } cons;

    char *string;
    int integer;
    float real;
  } v;
};

// Stream initialization functions
lisp_stream_t* lisp_stream_init_file(lisp_stream_t *stream, FILE *file);
lisp_stream_t* lisp_stream_init_string(lisp_stream_t *stream, const char *buf);

// Lisp object manipulation functions
lisp_object_t* lisp_read(lisp_stream_t *in);
lisp_object_t* lisp_read_from_file(std::string_view filename);
void lisp_free(lisp_object_t *obj);
lisp_object_t* lisp_read_from_string(const char *buf);
void lisp_reset_pool();

// Accessor functions for Lisp object values
int lisp_type(lisp_object_t *obj);
int lisp_integer(lisp_object_t *obj);
float lisp_real(lisp_object_t *obj);
char* lisp_symbol(lisp_object_t *obj);
char* lisp_string(lisp_object_t *obj);
int lisp_boolean(lisp_object_t *obj);
lisp_object_t* lisp_car(lisp_object_t *obj);
lisp_object_t* lisp_cdr(lisp_object_t *obj);

// Utility function to find the value associated with a symbol in a list (ideal for small lists)
lisp_object_t* lisp_find_value(lisp_object_t* list, const char* key);

// Lisp object creation functions
lisp_object_t* lisp_make_integer(int value);
lisp_object_t* lisp_make_real(float value);
lisp_object_t* lisp_make_symbol(const char *value);
lisp_object_t* lisp_make_string(const char *value);
lisp_object_t* lisp_make_cons(lisp_object_t *car, lisp_object_t *cdr);
lisp_object_t* lisp_make_boolean(int value);

// Macros for checking and accessing Lisp object types
#define lisp_nil()           ((lisp_object_t*)0)
#define lisp_nil_p(obj)      (obj == 0)
#define lisp_integer_p(obj)  (lisp_type((obj)) == LISP_TYPE_INTEGER)
#define lisp_real_p(obj)     (lisp_type((obj)) == LISP_TYPE_REAL)
#define lisp_symbol_p(obj)   (lisp_type((obj)) == LISP_TYPE_SYMBOL)
#define lisp_string_p(obj)   (lisp_type((obj)) == LISP_TYPE_STRING)
#define lisp_cons_p(obj)     (lisp_type((obj)) == LISP_TYPE_CONS)
#define lisp_boolean_p(obj)  (lisp_type((obj)) == LISP_TYPE_BOOLEAN)

// LispReader class for reading Lisp objects
class LispReader
{
private:
  // Map for O(1) property lookups.
  // Keys are interned C-strings, so pointer comparison is sufficient and fast.
  std::unordered_map<const char*, lisp_object_t*> property_map;

  lisp_object_t* search_for(const char* name);

  template <typename T, typename Predicate, typename Getter>
  bool read_vector_impl(const char* name, std::vector<T>* vec, Predicate pred, Getter get);
public:
  explicit LispReader(lisp_object_t* l);

  bool read_int_vector(const char* name, std::vector<int>* vec);
  bool read_string_vector(const char* name, std::vector<std::string>* vec);
  bool read_string(const char* name, std::string* str);
  bool read_int(const char* name, int* i);
  bool read_float(const char* name, float* f);
  bool read_bool(const char* name, bool* b);
  bool read_lisp(const char* name, lisp_object_t** b);
};

#endif // __LISPREADER_H__

// EOF
