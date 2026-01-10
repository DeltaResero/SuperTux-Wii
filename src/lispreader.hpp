// src/lispreader.h
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux - lispreader.h
// Copyright (C) 1998-2000 Mark Probst
// Copyright (C) 2002 Ingo Ruhnke <grumbel@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef __LISPREADER_H__
#define __LISPREADER_H__

#include <stdio.h>
#include <string>
#include <vector>
#include <unordered_map>

// Stream types for handling file, string, and custom streams
#define LISP_STREAM_FILE       1
#define LISP_STREAM_STRING     2
#define LISP_STREAM_ANY        3

// Lisp object types
#define LISP_TYPE_INTERNAL      -3
#define LISP_TYPE_PARSE_ERROR   -2
#define LISP_TYPE_EOF           -1
#define LISP_TYPE_NIL           0
#define LISP_TYPE_SYMBOL        1
#define LISP_TYPE_INTEGER       2
#define LISP_TYPE_STRING        3
#define LISP_TYPE_REAL          4
#define LISP_TYPE_CONS          5
#define LISP_TYPE_PATTERN_CONS  6
#define LISP_TYPE_BOOLEAN       7
#define LISP_TYPE_PATTERN_VAR   8

// Pattern matching types
#define LISP_PATTERN_ANY        1
#define LISP_PATTERN_SYMBOL     2
#define LISP_PATTERN_STRING     3
#define LISP_PATTERN_INTEGER    4
#define LISP_PATTERN_REAL       5
#define LISP_PATTERN_BOOLEAN    6
#define LISP_PATTERN_LIST       7
#define LISP_PATTERN_OR         8

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

    struct
    {
      void *data;
      int (*next_char) (void *data);
      void (*unget_char) (char c, void *data);
    } any;
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

    struct
    {
      int type;
      int index;
      struct _lisp_object_t *sub;
    } pattern;
  } v;
};

// Stream initialization functions
lisp_stream_t* lisp_stream_init_file(lisp_stream_t *stream, FILE *file);
lisp_stream_t* lisp_stream_init_string(lisp_stream_t *stream, const char *buf);
lisp_stream_t* lisp_stream_init_any(lisp_stream_t *stream, void *data,
                                    int (*next_char) (void *data),
                                    void (*unget_char) (char c, void *data));

// Lisp object manipulation functions
lisp_object_t* lisp_read(lisp_stream_t *in);
lisp_object_t* lisp_read_from_file(const std::string& filename);
void lisp_free(lisp_object_t *obj);
lisp_object_t* lisp_read_from_string(const char *buf);
void lisp_reset_pool();

// Pattern matching functions
int lisp_compile_pattern(lisp_object_t **obj, int *num_subs);
int lisp_match_pattern(lisp_object_t *pattern, lisp_object_t *obj, lisp_object_t **vars, int num_subs);
int lisp_match_string(const char *pattern_string, lisp_object_t *obj, lisp_object_t **vars);

// Accessor functions for Lisp object values
int lisp_type(lisp_object_t *obj);
int lisp_integer(lisp_object_t *obj);
float lisp_real(lisp_object_t *obj);
char* lisp_symbol(lisp_object_t *obj);
char* lisp_string(lisp_object_t *obj);
int lisp_boolean(lisp_object_t *obj);
lisp_object_t* lisp_car(lisp_object_t *obj);
lisp_object_t* lisp_cdr(lisp_object_t *obj);

// Utility function for accessing parts of a cons list based on a string of 'a' and 'd'
lisp_object_t* lisp_cxr(lisp_object_t *obj, const char *x);
// Utility function to find the value associated with a symbol in a list (ideal for small lists)
lisp_object_t* lisp_find_value(lisp_object_t* list, const char* key);

// Lisp object creation functions
lisp_object_t* lisp_make_integer(int value);
lisp_object_t* lisp_make_real(float value);
lisp_object_t* lisp_make_symbol(const char *value);
lisp_object_t* lisp_make_string(const char *value);
lisp_object_t* lisp_make_cons(lisp_object_t *car, lisp_object_t *cdr);
lisp_object_t* lisp_make_boolean(int value);

// List-related functions
int lisp_list_length(lisp_object_t *obj);
lisp_object_t* lisp_list_nth_cdr(lisp_object_t *obj, int index);
lisp_object_t* lisp_list_nth(lisp_object_t *obj, int index);

// Dumps a Lisp object to a file
void lisp_dump(lisp_object_t *obj, FILE *out);

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
  lisp_object_t* lst;
  // Map for O(1) property lookups.
  // Keys are interned C-strings, so pointer comparison is sufficient and fast.
  std::unordered_map<const char*, lisp_object_t*> property_map;

  lisp_object_t* search_for(const char* name);

  template <typename T, typename Predicate, typename Getter>
  bool read_vector_impl(const char* name, std::vector<T>* vec, Predicate pred, Getter get);
public:
  LispReader(lisp_object_t* l);

  bool read_int_vector(const char* name, std::vector<int>* vec);
  bool read_char_vector(const char* name, std::vector<char>* vec);
  bool read_string_vector(const char* name, std::vector<std::string>* vec);
  bool read_string(const char* name, std::string* str);
  bool read_int(const char* name, int* i);
  bool read_float(const char* name, float* f);
  bool read_bool(const char* name, bool* b);
  bool read_lisp(const char* name, lisp_object_t** b);
};

// LispWriter class for writing Lisp objects
class LispWriter
{
private:
  std::vector<lisp_object_t*> lisp_objs;

  void append(lisp_object_t* obj);
  lisp_object_t* make_list3(lisp_object_t*, lisp_object_t*, lisp_object_t*);
  lisp_object_t* make_list2(lisp_object_t*, lisp_object_t*);
public:
  LispWriter(const char* name);

  void write_float(const char* name, float f);
  void write_int(const char* name, int i);
  void write_boolean(const char* name, bool b);
  void write_string(const char* name, const char* str);
  void write_symbol(const char* name, const char* symname);
  void write_lisp_obj(const char* name, lisp_object_t* lst);

  lisp_object_t* create_lisp();
};

#endif // __LISPREADER_H__

// EOF
