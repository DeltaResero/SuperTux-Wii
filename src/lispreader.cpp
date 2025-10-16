/*
 * lispreader.c
 *
 * Copyright (C) 1998-2000 Mark Probst
 * Copyright (C) 2002 Ingo Ruhnke <grumbel@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <iostream>
#include <string>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <climits>
#include "setup.h"
#include "lispreader.h"

#define TOKEN_ERROR                   -1
#define TOKEN_EOF                     0
#define TOKEN_OPEN_PAREN              1
#define TOKEN_CLOSE_PAREN             2
#define TOKEN_SYMBOL                  3
#define TOKEN_STRING                  4
#define TOKEN_INTEGER                 5
#define TOKEN_REAL                    6
#define TOKEN_PATTERN_OPEN_PAREN      7
#define TOKEN_DOT                     8
#define TOKEN_TRUE                    9
#define TOKEN_FALSE                   10

#define MAX_TOKEN_LENGTH              1024

static char token_string[MAX_TOKEN_LENGTH + 1] = "";
static int token_length = 0;

static lisp_object_t end_marker = { LISP_TYPE_EOF, {{0, 0}} };
static lisp_object_t error_object = { LISP_TYPE_PARSE_ERROR , {{0, 0}} };
static lisp_object_t close_paren_marker = { LISP_TYPE_PARSE_ERROR , {{0, 0}} };
static lisp_object_t dot_marker = { LISP_TYPE_PARSE_ERROR , {{0, 0}} };

static int _match_pattern (lisp_object_t *pattern, lisp_object_t *obj, lisp_object_t **vars);

/**
 * Clears the current token.
 */
static void _token_clear(void)
{
  token_string[0] = '\0';
  token_length = 0;
}

/**
 * Appends a character to the current token.
 * @param c The character to append.
 */
static void _token_append(char c)
{
  assert(token_length < MAX_TOKEN_LENGTH);

  token_string[token_length++] = c;
  token_string[token_length] = '\0';
}

/**
 * Fetches the next character from the stream.
 * @param stream The Lisp stream to read from.
 * @return The next character or EOF.
 */
static int _next_char(lisp_stream_t *stream)
{
  switch (stream->type)
  {
    case LISP_STREAM_FILE:
      return fgetc(stream->v.file);

    case LISP_STREAM_STRING:
    {
      // Check buffer boundaries before accessing the string
      if (stream->v.string.pos >= stream->v.string.len)
      {
        return EOF;  // Return EOF if the position exceeds the buffer length
      }

      char c = stream->v.string.buf[stream->v.string.pos];
      if (c == 0)
      {
        return EOF;
      }

      ++stream->v.string.pos;
      return c;
    }

    case LISP_STREAM_ANY:
      return stream->v.any.next_char(stream->v.any.data);
  }

  assert(0);
  return EOF;
}

/**
 * Ungets the last character back to the stream.
 * @param c The character to unget.
 * @param stream The Lisp stream to modify.
 */
static void _unget_char(char c, lisp_stream_t *stream)
{
  switch (stream->type)
  {
    case LISP_STREAM_FILE:
      ungetc(c, stream->v.file);
      break;

    case LISP_STREAM_STRING:
      --stream->v.string.pos;
      break;

    case LISP_STREAM_ANY:
      stream->v.any.unget_char(c, stream->v.any.data);
      break;

    default:
      assert(0);
  }
}

/**
 * Scans the input stream for the next token.
 * @param stream The Lisp stream to scan.
 * @return The type of token found.
 */
static int _scan(lisp_stream_t *stream)
{
  static const char *delims = "\"();";
  int c;

  _token_clear();

  do
  {
    c = _next_char(stream);
    if (c == EOF)
    {
      return TOKEN_EOF;
    }
    else if (c == ';')  /* comment start */
    {
      while (1)
      {
        c = _next_char(stream);
        if (c == EOF)
        {
          return TOKEN_EOF;
        }
        else if (c == '\n')
        {
          break;
        }
      }
    }
  } while (isspace(c));

  switch (c)
  {
    case '(':
      return TOKEN_OPEN_PAREN;

    case ')':
      return TOKEN_CLOSE_PAREN;

    case '"':
      while (1)
      {
        c = _next_char(stream);
        if (c == EOF)
        {
          return TOKEN_ERROR;
        }
        if (c == '"')
        {
          break;
        }
        if (c == '\\')
        {
          c = _next_char(stream);
          switch (c)
          {
            case EOF:
              return TOKEN_ERROR;

            case 'n':
              c = '\n';
              break;

            case 't':
              c = '\t';
              break;
          }
        }
        _token_append(c);
      }
      return TOKEN_STRING;

    case '#':
      c = _next_char(stream);
      if (c == EOF)
      {
        return TOKEN_ERROR;
      }

      switch (c)
      {
        case 't':
          return TOKEN_TRUE;

        case 'f':
          return TOKEN_FALSE;

        case '?':
          c = _next_char(stream);
          if (c == EOF)
          {
            return TOKEN_ERROR;
          }
          if (c == '(')
          {
            return TOKEN_PATTERN_OPEN_PAREN;
          }
          else
          {
            return TOKEN_ERROR;
          }
      }
      return TOKEN_ERROR;

    default:
      if (isdigit(c) || c == '-')
      {
        int have_nondigits = 0;
        int have_digits = 0;
        int have_floating_point = 0;

        do
        {
          if (isdigit(c))
          {
            have_digits = 1;
          }
          else if (c == '.')
          {
            have_floating_point++;
          }
          _token_append(c);

          c = _next_char(stream);

          if (c != EOF && !isdigit(c) && !isspace(c) && c != '.' && !strchr(delims, c))
          {
            have_nondigits = 1;
          }
        } while (c != EOF && !isspace(c) && !strchr(delims, c));

        if (c != EOF)
        {
          _unget_char(c, stream);
        }

        if (have_nondigits || !have_digits || have_floating_point > 1)
        {
          return TOKEN_SYMBOL;
        }
        else if (have_floating_point == 1)
        {
          return TOKEN_REAL;
        }
        else
        {
          return TOKEN_INTEGER;
        }
      }
      else
      {
        if (c == '.')
        {
          c = _next_char(stream);
          if (c != EOF && !isspace(c) && !strchr(delims, c))
          {
            _token_append('.');
          }
          else
          {
            _unget_char(c, stream);
            return TOKEN_DOT;
          }
        }

        do
        {
          _token_append(c);
          c = _next_char(stream);
        } while (c != EOF && !isspace(c) && !strchr(delims, c));

        if (c != EOF)
        {
          _unget_char(c, stream);
        }

        return TOKEN_SYMBOL;
      }
  }

  assert(0);
  return TOKEN_ERROR;
}

/**
 * Allocates a new Lisp object.
 * @param type The type of the object.
 * @return A pointer to the allocated object.
 */
static lisp_object_t* lisp_object_alloc(int type)
{
  lisp_object_t *obj = (lisp_object_t*)malloc(sizeof(lisp_object_t));

  obj->type = type;

  return obj;
}

/**
 * Initializes a file-based Lisp stream.
 * @param stream The stream to initialize.
 * @param file The file to read from.
 * @return The initialized stream.
 */
lisp_stream_t* lisp_stream_init_file(lisp_stream_t *stream, FILE *file)
{
  stream->type = LISP_STREAM_FILE;
  stream->v.file = file;

  return stream;
}

/**
 * Initializes a string-based Lisp stream.
 * @param stream The stream to initialize.
 * @param buf The buffer to read from.
 * @return The initialized stream.
 */
lisp_stream_t* lisp_stream_init_string(lisp_stream_t *stream, char *buf)
{
  stream->type = LISP_STREAM_STRING;
  stream->v.string.buf = buf;
  stream->v.string.pos = 0;

  return stream;
}

/**
 * Initializes a general-purpose Lisp stream.
 * @param stream The stream to initialize.
 * @param data The data context for the stream.
 * @param next_char Function to fetch the next character.
 * @param unget_char Function to unget a character.
 * @return The initialized stream.
 */
lisp_stream_t* lisp_stream_init_any(lisp_stream_t *stream, void *data,
                      int (*next_char) (void *data),
                      void (*unget_char) (char c, void *data))
{
  assert(next_char != 0 && unget_char != 0);

  stream->type = LISP_STREAM_ANY;
  stream->v.any.data = data;
  stream->v.any.next_char = next_char;
  stream->v.any.unget_char = unget_char;

  return stream;
}

/**
 * Creates a Lisp integer object.
 * @param value The integer value.
 * @return A Lisp object representing the integer.
 */
lisp_object_t* lisp_make_integer(int value)
{
  lisp_object_t *obj = lisp_object_alloc(LISP_TYPE_INTEGER);

  obj->v.integer = value;

  return obj;
}

/**
 * Creates a Lisp real (floating-point) object.
 * @param value The float value.
 * @return A Lisp object representing the real number.
 */
lisp_object_t* lisp_make_real(float value)
{
  lisp_object_t *obj = lisp_object_alloc(LISP_TYPE_REAL);

  obj->v.real = value;

  return obj;
}

/**
 * Creates a Lisp symbol object.
 * @param value The symbol string.
 * @return A Lisp object representing the symbol.
 */
lisp_object_t* lisp_make_symbol(const char *value)
{
  lisp_object_t *obj = lisp_object_alloc(LISP_TYPE_SYMBOL);
  obj->v.string = strdup(value);
  return obj;
}

/**
 * Creates a Lisp string object.
 * @param value The string value.
 * @return A Lisp object representing the string.
 */
lisp_object_t* lisp_make_string(const char *value)
{
  lisp_object_t *obj = lisp_object_alloc(LISP_TYPE_STRING);
  obj->v.string = strdup(value);
  return obj;
}

/**
 * Creates a Lisp cons cell.
 * @param car The car part of the cons.
 * @param cdr The cdr part of the cons.
 * @return A Lisp object representing the cons.
 */
lisp_object_t* lisp_make_cons(lisp_object_t *car, lisp_object_t *cdr)
{
  lisp_object_t *obj = lisp_object_alloc(LISP_TYPE_CONS);
  obj->v.cons.car = car;
  obj->v.cons.cdr = cdr;
  return obj;
}

/**
 * Creates a Lisp boolean object.
 * @param value The boolean value (0 for false, non-zero for true).
 * @return A Lisp object representing the boolean.
 */
lisp_object_t* lisp_make_boolean(int value)
{
  lisp_object_t *obj = lisp_object_alloc(LISP_TYPE_BOOLEAN);
  obj->v.integer = value ? 1 : 0;
  return obj;
}

/**
 * Creates a Lisp pattern cons cell.
 * @param car The car part of the cons.
 * @param cdr The cdr part of the cons.
 * @return A Lisp object representing the pattern cons.
 */
static lisp_object_t* lisp_make_pattern_cons(lisp_object_t *car, lisp_object_t *cdr)
{
  lisp_object_t *obj = lisp_object_alloc(LISP_TYPE_PATTERN_CONS);
  obj->v.cons.car = car;
  obj->v.cons.cdr = cdr;
  return obj;
}

/**
 * Creates a Lisp pattern variable.
 * @param type The pattern type.
 * @param index The index of the variable.
 * @param sub The sub-pattern.
 * @return A Lisp object representing the pattern variable.
 */
static lisp_object_t* lisp_make_pattern_var(int type, int index, lisp_object_t *sub)
{
  lisp_object_t *obj = lisp_object_alloc(LISP_TYPE_PATTERN_VAR);
  obj->v.pattern.type = type;
  obj->v.pattern.index = index;
  obj->v.pattern.sub = sub;
  return obj;
}

/**
 * Reads a Lisp object from the stream.
 * @param in The input stream.
 * @return The Lisp object read.
 */
lisp_object_t* lisp_read(lisp_stream_t *in)
{
  int token = _scan(in);
  lisp_object_t *obj = lisp_nil();

  if (token == TOKEN_EOF)
  {
    return &end_marker;
  }

  switch (token)
  {
    case TOKEN_ERROR:
      return &error_object;

    case TOKEN_EOF:
      return &end_marker;

    case TOKEN_OPEN_PAREN:
    case TOKEN_PATTERN_OPEN_PAREN:
    {
      lisp_object_t *last = lisp_nil(), *car;

      do
      {
        car = lisp_read(in);
        if (car == &error_object || car == &end_marker)
        {
          lisp_free(obj);
          return &error_object;
        }
        else if (car == &dot_marker)
        {
          if (lisp_nil_p(last))
          {
            lisp_free(obj);
            return &error_object;
          }

          car = lisp_read(in);
          if (car == &error_object || car == &end_marker)
          {
            lisp_free(obj);
            return car;
          }
          else
          {
            last->v.cons.cdr = car;

            if (_scan(in) != TOKEN_CLOSE_PAREN)
            {
              lisp_free(obj);
              return &error_object;
            }

            car = &close_paren_marker;
          }
        }
        else if (car != &close_paren_marker)
        {
          if (lisp_nil_p(last))
          {
            obj = last = (token == TOKEN_OPEN_PAREN ? lisp_make_cons(car, lisp_nil()) : lisp_make_pattern_cons(car, lisp_nil()));
          }
          else
          {
            last = last->v.cons.cdr = lisp_make_cons(car, lisp_nil());
          }
        }
      }
      while (car != &close_paren_marker);
    }
    return obj;

    case TOKEN_CLOSE_PAREN:
      return &close_paren_marker;

    case TOKEN_SYMBOL:
      return lisp_make_symbol(token_string);

    case TOKEN_STRING:
      return lisp_make_string(token_string);

    case TOKEN_INTEGER:
    {
      // Convert string to long with error checking
      char* endptr;
      errno = 0;
      long int_val = strtol(token_string, &endptr, 10);
      if (errno != 0 || *endptr != '\0' || int_val < INT_MIN || int_val > INT_MAX)
      {
        return &error_object;  // Handle conversion error
      }
      return lisp_make_integer((int)int_val);
    }

    case TOKEN_REAL:
    {
      // Convert string to float with error checking
      char* endptr;
      errno = 0;
      float real_val = strtof(token_string, &endptr);
      if (errno != 0 || *endptr != '\0')
      {
        return &error_object;  // Handle conversion error
      }
      return lisp_make_real(real_val);
    }

    case TOKEN_DOT:
      return &dot_marker;

    case TOKEN_TRUE:
      return lisp_make_boolean(1);

    case TOKEN_FALSE:
      return lisp_make_boolean(0);
  }

  assert(0);
  return &error_object;
}

/**
 * Frees a Lisp object and its sub-objects.
 * @param obj The Lisp object to free.
 */
void lisp_free(lisp_object_t *obj)
{
  /** This goto solution has to be done cause using a recursion
      may cause a stack overflow (for instance, in MacOS 10.2). */
 restart:

  if (obj == 0)
  {
    return;
  }

  switch (obj->type)
  {
    case LISP_TYPE_INTERNAL:
    case LISP_TYPE_PARSE_ERROR:
    case LISP_TYPE_EOF:
      return;

    case LISP_TYPE_SYMBOL:
    case LISP_TYPE_STRING:
      free(obj->v.string);
      break;

    case LISP_TYPE_CONS:
    case LISP_TYPE_PATTERN_CONS:
      /* If we just recursively free car and cdr we risk a stack
         overflow because lists may be nested arbitrarily deep.

         We can get rid of one recursive call with a tail call,
         but there's still one remaining.

         The solution is to flatten a recursive list until we
         can free the car without recursion.  Then we free the
         cdr with a tail call.

         The transformation we perform on the list is this:

           ((a . b) . c) -> (a . (b . c))
      */
    {
      if (!lisp_nil_p(obj->v.cons.car)
        && (lisp_type(obj->v.cons.car) == LISP_TYPE_CONS
        || lisp_type(obj->v.cons.car) == LISP_TYPE_PATTERN_CONS))
      {
        /* this is the transformation */
        lisp_object_t *car, *cdar;

        car = obj->v.cons.car;
        cdar = car->v.cons.cdr;

        car->v.cons.cdr = obj;
        obj->v.cons.car = cdar;
        obj = car;
        goto restart;
      }
      else
      {
        /* here we just free the car (which is not recursive),
           the cons itself and the cdr via a tail call.  */

        lisp_object_t *tmp;

        lisp_free(obj->v.cons.car);
        tmp = obj;
        obj = obj->v.cons.cdr;
        free(tmp);
        goto restart;
      }
    }

    case LISP_TYPE_PATTERN_VAR:
      lisp_free(obj->v.pattern.sub);
      break;
  }

  free(obj);
}

/**
 * Reads a Lisp object from a string.
 * @param buf The string buffer to read from.
 * @return The Lisp object read.
 */
lisp_object_t* lisp_read_from_string(const char *buf)
{
  lisp_stream_t stream;
  lisp_stream_init_string(&stream, (char*)buf);
  return lisp_read(&stream);
}

/**
 * Compiles a Lisp pattern.
 * @param obj The Lisp object to compile.
 * @param index The index for the pattern variable.
 * @return 1 if successful, 0 if an error occurred.
 */
static int _compile_pattern(lisp_object_t **obj, int *index)
{
  if (*obj == 0)
  {
    return 1;
  }

  switch (lisp_type(*obj))
  {
    case LISP_TYPE_PATTERN_CONS:
    {
      struct
      {
        const char *name;
        int type;
      } types[] =
      {
        { "any", LISP_PATTERN_ANY },
        { "symbol", LISP_PATTERN_SYMBOL },
        { "string", LISP_PATTERN_STRING },
        { "integer", LISP_PATTERN_INTEGER },
        { "real", LISP_PATTERN_REAL },
        { "boolean", LISP_PATTERN_BOOLEAN },
        { "list", LISP_PATTERN_LIST },
        { "or", LISP_PATTERN_OR },
        { 0, 0 }
      };

      char *type_name;
      int type = -1;
      int i;
      lisp_object_t *pattern;

      if (lisp_type(lisp_car(*obj)) != LISP_TYPE_SYMBOL)
      {
        return 0;
      }

      type_name = lisp_symbol(lisp_car(*obj));
      for (i = 0; types[i].name != 0; ++i)
      {
        if (strcmp(types[i].name, type_name) == 0)
        {
          type = types[i].type;
          break;
        }
      }

      if (types[i].name == 0)
      {
        return 0;
      }

      if (type != LISP_PATTERN_OR && lisp_cdr(*obj) != 0)
      {
        return 0;
      }

      pattern = lisp_make_pattern_var(type, (*index)++, lisp_nil());

      if (type == LISP_PATTERN_OR)
      {
        lisp_object_t *cdr = lisp_cdr(*obj);

        if (!_compile_pattern(&cdr, index))
        {
          lisp_free(pattern);
          return 0;
        }

        pattern->v.pattern.sub = cdr;
        (*obj)->v.cons.cdr = lisp_nil();
      }

      lisp_free(*obj);
      *obj = pattern;
    }
    break;

    case LISP_TYPE_CONS:
      if (!_compile_pattern(&(*obj)->v.cons.car, index))
      {
        return 0;
      }
      if (!_compile_pattern(&(*obj)->v.cons.cdr, index))
      {
        return 0;
      }
      break;
  }

  return 1;
}

/**
 * Compiles a Lisp pattern.
 * @param obj The Lisp object to compile.
 * @param num_subs The number of sub-patterns.
 * @return 1 if successful, 0 if an error occurred.
 */
int lisp_compile_pattern(lisp_object_t **obj, int *num_subs)
{
  int index = 0;
  int result;

  result = _compile_pattern(obj, &index);

  if (result && num_subs != 0)
  {
    *num_subs = index;
  }

  return result;
}

/**
 * Matches a pattern variable against a Lisp object.
 * @param pattern The pattern to match.
 * @param obj The Lisp object to match against.
 * @param vars Array of matched variables.
 * @return 1 if the pattern matches, 0 otherwise.
 */
static int _match_pattern_var(lisp_object_t *pattern, lisp_object_t *obj, lisp_object_t **vars)
{
  assert(lisp_type(pattern) == LISP_TYPE_PATTERN_VAR);

  switch (pattern->v.pattern.type)
  {
    case LISP_PATTERN_ANY:
      break;

    case LISP_PATTERN_SYMBOL:
      if (obj == 0 || lisp_type(obj) != LISP_TYPE_SYMBOL)
      {
        return 0;
      }
      break;

    case LISP_PATTERN_STRING:
      if (obj == 0 || lisp_type(obj) != LISP_TYPE_STRING)
      {
        return 0;
      }
      break;

    case LISP_PATTERN_INTEGER:
      if (obj == 0 || lisp_type(obj) != LISP_TYPE_INTEGER)
      {
        return 0;
      }
      break;

    case LISP_PATTERN_REAL:
      if (obj == 0 || lisp_type(obj) != LISP_TYPE_REAL)
      {
        return 0;
      }
      break;

    case LISP_PATTERN_BOOLEAN:
      if (obj == 0 || lisp_type(obj) != LISP_TYPE_BOOLEAN)
      {
        return 0;
      }
      break;

    case LISP_PATTERN_LIST:
      if (obj == 0 || lisp_type(obj) != LISP_TYPE_CONS)
      {
        return 0;
      }
      break;

    case LISP_PATTERN_OR:
    {
      lisp_object_t *sub;
      int matched = 0;

      for (sub = pattern->v.pattern.sub; sub != 0; sub = lisp_cdr(sub))
      {
        assert(lisp_type(sub) == LISP_TYPE_CONS);

        if (_match_pattern(lisp_car(sub), obj, vars))
        {
          matched = 1;
        }
      }

      if (!matched)
      {
        return 0;
      }
    }
    break;

    default:
      assert(0);
  }

  if (vars != 0)
  {
    vars[pattern->v.pattern.index] = obj;
  }

  return 1;
}

/**
 * Matches a pattern against a Lisp object.
 * @param pattern The pattern to match.
 * @param obj The Lisp object to match against.
 * @param vars Array of matched variables.
 * @return 1 if the pattern matches, 0 otherwise.
 */
static int _match_pattern(lisp_object_t *pattern, lisp_object_t *obj, lisp_object_t **vars)
{
  if (pattern == 0)
  {
    return obj == 0;
  }

  if (obj == 0)
  {
    return 0;
  }

  if (lisp_type(pattern) == LISP_TYPE_PATTERN_VAR)
  {
    return _match_pattern_var(pattern, obj, vars);
  }

  if (lisp_type(pattern) != lisp_type(obj))
  {
    return 0;
  }

  switch (lisp_type(pattern))
  {
    case LISP_TYPE_SYMBOL:
      return strcmp(lisp_symbol(pattern), lisp_symbol(obj)) == 0;

    case LISP_TYPE_STRING:
      return strcmp(lisp_string(pattern), lisp_string(obj)) == 0;

    case LISP_TYPE_INTEGER:
      return lisp_integer(pattern) == lisp_integer(obj);

    case LISP_TYPE_REAL:
      return lisp_real(pattern) == lisp_real(obj);

    case LISP_TYPE_CONS:
    {
      int result1, result2;

      result1 = _match_pattern(lisp_car(pattern), lisp_car(obj), vars);
      result2 = _match_pattern(lisp_cdr(pattern), lisp_cdr(obj), vars);

      return result1 && result2;
    }
    break;

    default:
      assert(0);
  }

  return 0;
}

/**
 * Matches a Lisp pattern against an object.
 * @param pattern The pattern to match.
 * @param obj The object to match against.
 * @param vars The array to store matched variables.
 * @param num_subs The number of sub-patterns.
 * @return 1 if the pattern matches, 0 otherwise.
 */
int lisp_match_pattern(lisp_object_t *pattern, lisp_object_t *obj, lisp_object_t **vars, int num_subs)
{
  int i;

  if (vars != 0)
  {
    for (i = 0; i < num_subs; ++i)
    {
      vars[i] = &error_object;
    }
  }

  return _match_pattern(pattern, obj, vars);
}

/**
 * Matches a string-based pattern against a Lisp object.
 * @param pattern_string The pattern string.
 * @param obj The Lisp object to match against.
 * @param vars The array to store matched variables.
 * @return 1 if the pattern matches, 0 otherwise.
 */
int lisp_match_string(const char *pattern_string, lisp_object_t *obj, lisp_object_t **vars)
{
  lisp_object_t *pattern;
  int result;
  int num_subs;

  pattern = lisp_read_from_string(pattern_string);

  if (pattern != 0 && (lisp_type(pattern) == LISP_TYPE_EOF || lisp_type(pattern) == LISP_TYPE_PARSE_ERROR))
  {
    return 0;
  }

  if (!lisp_compile_pattern(&pattern, &num_subs))
  {
    lisp_free(pattern);
    return 0;
  }

  result = lisp_match_pattern(pattern, obj, vars, num_subs);
  lisp_free(pattern);

  return result;
}

/**
 * Returns the type of a Lisp object.
 * @param obj The Lisp object.
 * @return The type of the object.
 */
int lisp_type(lisp_object_t *obj)
{
  if (obj == 0)
  {
    return LISP_TYPE_NIL;
  }
  return obj->type;
}

/**
 * Returns the integer value of a Lisp object.
 * @param obj The Lisp object.
 * @return The integer value.
 */
int lisp_integer(lisp_object_t *obj)
{
  assert(obj->type == LISP_TYPE_INTEGER);
  return obj->v.integer;
}

/**
 * Returns the symbol value of a Lisp object.
 * @param obj The Lisp object.
 * @return The symbol string.
 */
char* lisp_symbol(lisp_object_t *obj)
{
  assert(obj->type == LISP_TYPE_SYMBOL);
  return obj->v.string;
}

/**
 * Returns the string value of a Lisp object.
 * @param obj The Lisp object.
 * @return The string value.
 */
char* lisp_string(lisp_object_t *obj)
{
  assert(obj->type == LISP_TYPE_STRING);
  return obj->v.string;
}

/**
 * Returns the boolean value of a Lisp object.
 * @param obj The Lisp object.
 * @return The boolean value (0 or 1).
 */
int lisp_boolean(lisp_object_t *obj)
{
  assert(obj->type == LISP_TYPE_BOOLEAN);
  return obj->v.integer;
}

/**
 * Returns the real value of a Lisp object.
 * @param obj The Lisp object.
 * @return The real value.
 */
float lisp_real(lisp_object_t *obj)
{
  assert(obj->type == LISP_TYPE_REAL || obj->type == LISP_TYPE_INTEGER);

  if (obj->type == LISP_TYPE_INTEGER)
  {
    return obj->v.integer;
  }
  return obj->v.real;
}

/**
 * Returns the car (head) of a Lisp cons cell.
 * @param obj The cons cell.
 * @return The car part of the cons.
 */
lisp_object_t* lisp_car(lisp_object_t *obj)
{
  assert(obj->type == LISP_TYPE_CONS || obj->type == LISP_TYPE_PATTERN_CONS);
  return obj->v.cons.car;
}

/**
 * Returns the cdr (tail) of a Lisp cons cell.
 * @param obj The cons cell.
 * @return The cdr part of the cons.
 */
lisp_object_t* lisp_cdr(lisp_object_t *obj)
{
  assert(obj->type == LISP_TYPE_CONS || obj->type == LISP_TYPE_PATTERN_CONS);
  return obj->v.cons.cdr;
}

/**
 * Traverses a Lisp list based on the given string of 'a' (car) and 'd' (cdr).
 * @param obj The cons cell.
 * @param x The string indicating the traversal (e.g., "cdr" or "cadr").
 * @return The resulting Lisp object after traversal.
 */
lisp_object_t* lisp_cxr(lisp_object_t *obj, const char *x)
{
  int i;

  // Validate string before using strnlen.
  if (x == nullptr)
  {
    return nullptr;
  }

  // Use a defined maximum length (1025) to align with MAX_TOKEN_LENGTH
  size_t x_len = strnlen(x, 1025);

  // Traverse the string backwards from the last valid character.
  for (i = x_len - 1; i >= 0; --i)
  {
    if (x[i] == 'a')
    {
      obj = lisp_car(obj);
    }
    else if (x[i] == 'd')
    {
      obj = lisp_cdr(obj);
    }
    else
    {
      assert(0);  // Invalid character in string
    }
  }

  return obj;
}

/**
 * Returns the length of a Lisp list.
 * @param obj The Lisp list.
 * @return The length of the list.
 */
int lisp_list_length(lisp_object_t *obj)
{
  int length = 0;

  while (obj != 0)
  {
    assert(obj->type == LISP_TYPE_CONS || obj->type == LISP_TYPE_PATTERN_CONS);
    ++length;
    obj = obj->v.cons.cdr;
  }

  return length;
}

/**
 * Returns the nth cdr (tail) of a Lisp list.
 * @param obj The Lisp list.
 * @param index The index of the nth cdr.
 * @return The nth cdr of the list.
 */
lisp_object_t* lisp_list_nth_cdr(lisp_object_t *obj, int index)
{
  while (index > 0)
  {
    assert(obj != 0);
    assert(obj->type == LISP_TYPE_CONS || obj->type == LISP_TYPE_PATTERN_CONS);
    --index;
    obj = obj->v.cons.cdr;
  }

  return obj;
}

/**
 * Returns the nth car (head) of a Lisp list.
 * @param obj The Lisp list.
 * @param index The index of the nth car.
 * @return The nth car of the list.
 */
lisp_object_t* lisp_list_nth(lisp_object_t *obj, int index)
{
  obj = lisp_list_nth_cdr(obj, index);
  assert(obj != 0);
  return obj->v.cons.car;
}

/**
 * Dumps a Lisp object to a file stream.
 * @param obj The Lisp object to dump.
 * @param out The file stream to write to.
 */
void lisp_dump(lisp_object_t *obj, FILE *out)
{
  if (obj == 0)
  {
    fprintf(out, "()");
    return;
  }

  switch (lisp_type(obj))
  {
    case LISP_TYPE_EOF:
      fputs("#<eof>", out);
      break;

    case LISP_TYPE_PARSE_ERROR:
      fputs("#<error>", out);
      break;

    case LISP_TYPE_INTEGER:
      fprintf(out, "%d", lisp_integer(obj));
      break;

    case LISP_TYPE_REAL:
      fprintf(out, "%f", lisp_real(obj));
      break;

    case LISP_TYPE_SYMBOL:
      fputs(lisp_symbol(obj), out);
      break;

    case LISP_TYPE_STRING:
    {
      char *p;
      fputc('"', out);
      for (p = lisp_string(obj); *p != 0; ++p)
      {
        if (*p == '"' || *p == '\\')
        {
          fputc('\\', out);
        }
        fputc(*p, out);
      }
      fputc('"', out);
    }
    break;

    case LISP_TYPE_CONS:
    case LISP_TYPE_PATTERN_CONS:
      fputs(lisp_type(obj) == LISP_TYPE_CONS ? "(" : "#?(", out);
      while (obj != 0)
      {
        lisp_dump(lisp_car(obj), out);
        obj = lisp_cdr(obj);
        if (obj != 0)
        {
          if (lisp_type(obj) != LISP_TYPE_CONS && lisp_type(obj) != LISP_TYPE_PATTERN_CONS)
          {
            fputs(" . ", out);
            lisp_dump(obj, out);
            break;
          }
          else
          {
            fputc(' ', out);
          }
        }
      }
      fputc(')', out);
      break;

    case LISP_TYPE_BOOLEAN:
      if (lisp_boolean(obj))
      {
        fputs("#t", out);
      }
      else
      {
        fputs("#f", out);
      }
      break;

    default:
      assert(0);
  }
}

/**
 * Constructor for LispReader.
 * @param l The Lisp object to be read.
 */
LispReader::LispReader(lisp_object_t* l)
  : lst(l)
{
  // std::cout << "LispReader: " << std::flush;
  // lisp_dump(lst, stdout);
  // std::cout << std::endl;
}

/**
 * Searches for a symbol in the Lisp list.
 * @param name The symbol name to search for.
 * @return The Lisp object associated with the symbol, or 0 if not found.
 */
lisp_object_t* LispReader::search_for(const char* name)
{
  lisp_object_t* cursor = lst;

  while (!lisp_nil_p(cursor))
  {
    lisp_object_t* cur = lisp_car(cursor);

    if (!lisp_cons_p(cur) || !lisp_symbol_p(lisp_car(cur)))
    {
      lisp_dump(cur, stdout);
      //throw ConstruoError (std::string("LispReader: Read error in search_for ") + name);
      printf("LispReader: Read error in search\n");
    }
    else
    {
      if (strcmp(lisp_symbol(lisp_car(cur)), name) == 0)
      {
        return lisp_cdr(cur);
      }
    }

    cursor = lisp_cdr(cursor);
  }
  return 0;
}

/**
 * Reads an integer from the Lisp list.
 * @param name The symbol name to search for.
 * @param i Pointer to store the integer.
 * @return true if successful, false otherwise.
 */
bool LispReader::read_int(const char* name, int* i)
{
  lisp_object_t* obj = search_for(name);
  if (obj)
  {
    if (!lisp_integer_p(lisp_car(obj)))
    {
      return false;
    }
    *i = lisp_integer(lisp_car(obj));
    return true;
  }
  return false;
}

/**
 * Reads a Lisp object.
 * @param name The symbol name to search for.
 * @param b Pointer to store the Lisp object.
 * @return true if successful, false otherwise.
 */
bool LispReader::read_lisp(const char* name, lisp_object_t** b)
{
  lisp_object_t* obj = search_for(name);
  if (obj)
  {
    *b = obj;
    return true;
  }
  return false;
}

/**
 * Reads a float from the Lisp list.
 * @param name The symbol name to search for.
 * @param f Pointer to store the float.
 * @return true if successful, false otherwise.
 */
bool LispReader::read_float(const char* name, float* f)
{
  lisp_object_t* obj = search_for(name);
  if (obj)
  {
    if (!lisp_real_p(lisp_car(obj)) && !lisp_integer_p(lisp_car(obj)))
    {
      st_abort("LispReader expected type real at token: ", name);
    }
    *f = lisp_real(lisp_car(obj));
    return true;
  }
  return false;
}

/**
 * Reads a vector of strings from the Lisp list.
 * @param name The symbol name to search for.
 * @param vec Pointer to store the vector of strings.
 * @return true if successful, false otherwise.
 */
bool LispReader::read_string_vector(const char* name, std::vector<std::string>* vec)
{
  lisp_object_t* obj = search_for(name);
  if (obj)
  {
    while (!lisp_nil_p(obj))
    {
      if (!lisp_string_p(lisp_car(obj)))
      {
        st_abort("LispReader expected type string at token: ", name);
      }
      vec->push_back(lisp_string(lisp_car(obj)));
      obj = lisp_cdr(obj);
    }
    return true;
  }
  return false;
}

/**
 * Reads a vector of integers from the Lisp list.
 * @param name The symbol name to search for.
 * @param vec Pointer to store the vector of integers.
 * @return true if successful, false otherwise.
 */
bool LispReader::read_int_vector(const char* name, std::vector<int>* vec)
{
  lisp_object_t* obj = search_for(name);
  if (obj)
  {
    while (!lisp_nil_p(obj))
    {
      if (!lisp_integer_p(lisp_car(obj)))
      {
        st_abort("LispReader expected type integer at token: ", name);
      }
      vec->push_back(lisp_integer(lisp_car(obj)));
      obj = lisp_cdr(obj);
    }
    return true;
  }
  return false;
}

/**
 * Reads a vector of characters from the Lisp list.
 * @param name The symbol name to search for.
 * @param vec Pointer to store the vector of characters.
 * @return true if successful, false otherwise.
 */
bool LispReader::read_char_vector(const char* name, std::vector<char>* vec)
{
  lisp_object_t* obj = search_for(name);
  if (obj)
  {
    while (!lisp_nil_p(obj))
    {
      vec->push_back(*lisp_string(lisp_car(obj)));
      obj = lisp_cdr(obj);
    }
    return true;
  }
  return false;
}

/**
 * Reads a string from the Lisp list.
 * @param name The symbol name to search for.
 * @param str Pointer to store the string.
 * @return true if successful, false otherwise.
 */
bool LispReader::read_string(const char* name, std::string* str)
{
  lisp_object_t* obj = search_for(name);
  if (obj)
  {
    if (!lisp_string_p(lisp_car(obj)))
    {
      st_abort("LispReader expected type string at token: ", name);
    }
    *str = lisp_string(lisp_car(obj));
    return true;
  }
  return false;
}

/**
 * Reads a boolean from the Lisp list.
 * @param name The symbol name to search for.
 * @param b Pointer to store the boolean.
 * @return true if successful, false otherwise.
 */
bool LispReader::read_bool(const char* name, bool* b)
{
  lisp_object_t* obj = search_for(name);
  if (obj)
  {
    if (!lisp_boolean_p(lisp_car(obj)))
    {
      st_abort("LispReader expected type bool at token: ", name);
    }
    *b = lisp_boolean(lisp_car(obj));
    return true;
  }
  return false;
}

/**
 * Constructor for LispWriter.
 * @param name The name of the symbol to start the writer with.
 */
LispWriter::LispWriter(const char* name)
{
  lisp_objs.push_back(lisp_make_symbol(name));
}

/**
 * Appends a Lisp object to the writer.
 * @param obj The Lisp object to append.
 */
void LispWriter::append(lisp_object_t* obj)
{
  lisp_objs.push_back(obj);
}

/**
 * Creates a Lisp list with three elements.
 * @param a The first element.
 * @param b The second element.
 * @param c The third element.
 * @return A Lisp cons list containing the three elements.
 */
lisp_object_t* LispWriter::make_list3(lisp_object_t* a, lisp_object_t* b, lisp_object_t* c)
{
  return lisp_make_cons(a, lisp_make_cons(b, lisp_make_cons(c, lisp_nil())));
}

/**
 * Creates a Lisp list with two elements.
 * @param a The first element.
 * @param b The second element.
 * @return A Lisp cons list containing the two elements.
 */
lisp_object_t* LispWriter::make_list2(lisp_object_t* a, lisp_object_t* b)
{
  return lisp_make_cons(a, lisp_make_cons(b, lisp_nil()));
}

/**
 * Writes a float to the Lisp writer.
 * @param name The symbol name.
 * @param f The float value.
 */
void LispWriter::write_float(const char* name, float f)
{
  append(make_list2(lisp_make_symbol(name), lisp_make_real(f)));
}

/**
 * Writes an integer to the Lisp writer.
 * @param name The symbol name.
 * @param i The integer value.
 */
void LispWriter::write_int(const char* name, int i)
{
  append(make_list2(lisp_make_symbol(name), lisp_make_integer(i)));
}

/**
 * Writes a string to the Lisp writer.
 * @param name The symbol name.
 * @param str The string value.
 */
void LispWriter::write_string(const char* name, const char* str)
{
  append(make_list2(lisp_make_symbol(name), lisp_make_string(str)));
}

/**
 * Writes a symbol to the Lisp writer.
 * @param name The symbol name.
 * @param symname The symbol to write.
 */
void LispWriter::write_symbol(const char* name, const char* symname)
{
  append(make_list2(lisp_make_symbol(name), lisp_make_symbol(symname)));
}

/**
 * Writes a Lisp object to the writer.
 * @param name The symbol name.
 * @param lst The Lisp object to write.
 */
void LispWriter::write_lisp_obj(const char* name, lisp_object_t* lst)
{
  append(make_list2(lisp_make_symbol(name), lst));
}

/**
 * Writes a boolean value to the Lisp writer.
 * @param name The symbol name.
 * @param b The boolean value.
 */
void LispWriter::write_boolean(const char* name, bool b)
{
  append(make_list2(lisp_make_symbol(name), lisp_make_boolean(b)));
}

/**
 * Creates a Lisp object from the list of written objects.
 * @return The created Lisp object.
 */
lisp_object_t* LispWriter::create_lisp()
{
  lisp_object_t* lisp_obj = lisp_nil();

  for (std::vector<lisp_object_t*>::reverse_iterator i = lisp_objs.rbegin();
       i != lisp_objs.rend(); ++i)
  {
    lisp_obj = lisp_make_cons(*i, lisp_obj);
  }

  lisp_objs.clear();
  return lisp_obj;
}

#if 0
/**
 * Ungets a character from a gzipped file.
 * @param c The character to unget.
 * @param file The gzipped file.
 */
void mygzungetc(char c, void* file)
{
  gzungetc(c, file);
}

/**
 * Initializes a Lisp stream for reading from a gzipped file.
 * @param stream The Lisp stream to initialize.
 * @param file The gzipped file to read from.
 * @return The initialized Lisp stream.
 */
lisp_stream_t* lisp_stream_init_gzfile(lisp_stream_t *stream, gzFile file)
{
  return lisp_stream_init_any(stream, file, gzgetc, mygzungetc);
}
#endif

/**
 * Reads a Lisp object from a gzipped file.
 * @param filename The name of the file to read from.
 * @return The Lisp object read from the file.
 */
lisp_object_t* lisp_read_from_gzfile(const char* filename)
{
  bool done = false;
  lisp_object_t* root_obj = 0;
  int chunk_size = 128 * 1024;
  int buf_pos = 0;
  int try_number = 1;
  char* buf = static_cast<char*>(malloc(chunk_size));
  assert(buf);

  gzFile in = gzopen(filename, "r");

  while (!done)
  {
    int ret = gzread(in, buf + buf_pos, chunk_size);
    if (ret == -1)
    {
      free(buf);
      assert(!"Error while reading from file");
    }
    else if (ret == chunk_size) // buffer got full, resize if EOF not yet reached
    {
      buf_pos = chunk_size * try_number;
      try_number += 1;
      buf = static_cast<char*>(realloc(buf, chunk_size * try_number));
      assert(buf);
    }
    else
    {
      // Everything fine, encountered EOF
      done = true;
    }
  }

  lisp_stream_t stream;
  lisp_stream_init_string(&stream, buf);
  root_obj = lisp_read(&stream);

  free(buf);
  gzclose(in);

  return root_obj;
}

/**
 * Checks if a string has a specific suffix.
 * @param data The string to check.
 * @param suffix The suffix to look for.
 * @return true if the string has the suffix, false otherwise.
 */
bool has_suffix(const char* data, const char* suffix)
{
  // Validate input before calling strnlen
  if (data == nullptr || suffix == nullptr)
  {
    return false;
  }

  // Use a defined maximum length (1025) to align with MAX_TOKEN_LENGTH
  size_t suffix_len = strnlen(suffix, 1025);
  size_t data_len = strnlen(data, 1025);

  if (data_len < suffix_len)
  {
    return false;
  }

  const char* data_suffix = (data + data_len - suffix_len);
  return (strncmp(data_suffix, suffix, suffix_len) == 0);
}

/**
 * Reads a Lisp object from a file, handling gzipped files if necessary.
 * @param filename The name of the file to read.
 * @return The Lisp object read from the file.
 */
lisp_object_t* lisp_read_from_file(const std::string& filename)
{
  lisp_stream_t stream;

  if (has_suffix(filename.c_str(), ".gz"))
  {
    return lisp_read_from_gzfile(filename.c_str());
  }
  else
  {
    lisp_object_t* obj = 0;
    FILE* in = fopen(filename.c_str(), "r");

    if (in)
    {
      lisp_stream_init_file(&stream, in);
      obj = lisp_read(&stream);
      fclose(in);
    }

    return obj;
  }
}

// EOF
