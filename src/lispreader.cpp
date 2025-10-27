/*
 * lispreader.cpp
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

#include "lispreader.h"
#include "setup.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector> // Required for lisp_read_from_file buffer

// All implementation details are hidden in an anonymous namespace
// to prevent symbol conflicts and improve encapsulation.
namespace
{
  constexpr size_t MAX_TOKEN_LENGTH = 1024;
  // 8192 objects * ~8 bytes/object = ~64KB per block. A reasonable starting size.
  constexpr size_t OBJECT_POOL_SIZE = 8192;

  enum CharProperty : uint8_t {
    PROP_NONE = 0,
    PROP_WHITESPACE = 1 << 0,
    PROP_DELIMITER  = 1 << 1,
    PROP_DIGIT      = 1 << 2,
  };

  static const std::array<uint8_t, 256> g_char_props = [] {
    std::array<uint8_t, 256> props{};
    for (int i = 0; i < 256; ++i) {
        if (isspace(i)) props[i] |= PROP_WHITESPACE;
        if (strchr("\"();", i)) props[i] |= PROP_DELIMITER;
        if (isdigit(i)) props[i] |= PROP_DIGIT;
    }
    return props;
  }();


  enum class TokenType : int
  {
    ERROR = -1,
    END_OF_FILE = 0,
    OPEN_PAREN = 1,
    CLOSE_PAREN = 2,
    SYMBOL = 3,
    STRING = 4,
    INTEGER = 5,
    REAL = 6,
    PATTERN_OPEN_PAREN = 7,
    DOT = 8,
    TRUE = 9,
    FALSE = 10
  };

  /**
   * Memory pool for lisp_object_t allocation.
   * Drastically improves performance and prevents memory fragmentation on the Wii
   * by avoiding frequent calls to malloc(). Allocations are O(1).
   */
  class LispObjectPool
  {
  private:
    struct PoolBlock
    {
      std::array<lisp_object_t, OBJECT_POOL_SIZE> objects;
      PoolBlock* next;
    };

    PoolBlock* head;
    PoolBlock* current_block;
    size_t     next_free_index;

    void allocate_block()
    {
      PoolBlock* block = new PoolBlock();
      block->next = head;
      head = block;

      current_block = block;
      next_free_index = 0;
    }

  public:
    LispObjectPool() : head(nullptr), current_block(nullptr), next_free_index(OBJECT_POOL_SIZE)
    {
    }

    ~LispObjectPool()
    {
      reset();
    }

    lisp_object_t* allocate(int type)
    {
      if (next_free_index >= OBJECT_POOL_SIZE)
      {
        allocate_block();
      }

      lisp_object_t* obj = &current_block->objects[next_free_index];
      obj->type = type;
      ++next_free_index;
      return obj;
    }

    void reset()
    {
      while (head)
      {
        PoolBlock* next = head->next;
        delete head;
        head = next;
      }
      current_block = nullptr;
      next_free_index = OBJECT_POOL_SIZE;
    }
  };

  /**
   * String interning system for symbols AND strings.
   * Reduces memory usage by ensuring identical strings share the same memory.
   * Crucially, this allows for fast O(1) pointer comparison instead of slow strcmp().
   */
  class StringInterner
  {
  private:
    // std::unordered_map is a good general-purpose choice. While it has some memory
    // overhead, its average O(1) lookup is ideal for load-time performance.
    std::unordered_map<std::string, char*> intern_table;

  public:
    ~StringInterner()
    {
      for (auto& pair : intern_table)
      {
        free(pair.second);
      }
    }

    const char* intern(const char* str)
    {
      auto it = intern_table.find(str);
      if (it != intern_table.end())
      {
        return it->second;
      }

      char* interned_str = strdup(str);
      intern_table[interned_str] = interned_str;
      return interned_str;
    }
  };

  // Global instances for memory management.
  static LispObjectPool g_object_pool;
  static StringInterner g_string_interner;

  // Sentinel objects to avoid allocating for simple markers.
  static lisp_object_t end_marker = { LISP_TYPE_EOF, {{0, 0}} };
  static lisp_object_t error_object = { LISP_TYPE_PARSE_ERROR , {{0, 0}} };
  static lisp_object_t close_paren_marker = { LISP_TYPE_PARSE_ERROR , {{0, 0}} };
  static lisp_object_t dot_marker = { LISP_TYPE_PARSE_ERROR , {{0, 0}} };

  lisp_object_t* lisp_object_alloc(int type)
  {
    return g_object_pool.allocate(type);
  }

  static lisp_object_t* lisp_make_pattern_cons(lisp_object_t* car, lisp_object_t* cdr)
  {
    lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_PATTERN_CONS);
    obj->v.cons.car = car;
    obj->v.cons.cdr = cdr;
    return obj;
  }

  static lisp_object_t* lisp_make_pattern_var(int type, int index, lisp_object_t* sub)
  {
    lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_PATTERN_VAR);
    obj->v.pattern.type = type;
    obj->v.pattern.index = index;
    obj->v.pattern.sub = sub;
    return obj;
  }

  /**
   * Internal parser state encapsulated in a class.
   * This makes the parser re-entrant and avoids static global variables for state.
   */
  class LispParserInternal
  {
  private:
    std::array<char, MAX_TOKEN_LENGTH + 1> token_string;
    size_t token_length;

    void token_clear();
    void token_append(char c);
    const char* token_c_str() const;

    int next_char(lisp_stream_t* stream);
    void unget_char(char c, lisp_stream_t* stream);
    TokenType scan(lisp_stream_t* stream);

    int match_pattern_var(lisp_object_t* pattern, lisp_object_t* obj, lisp_object_t** vars);
    int match_pattern(lisp_object_t* pattern, lisp_object_t* obj, lisp_object_t** vars);

  public:
    LispParserInternal();
    lisp_object_t* read(lisp_stream_t* in);
    int compile_pattern(lisp_object_t** obj, int* index);
    int do_match_pattern(lisp_object_t* pattern, lisp_object_t* obj, lisp_object_t** vars, int num_subs);
  };

  LispParserInternal::LispParserInternal() : token_length(0)
  {
    token_string[0] = '\0';
  }

  void LispParserInternal::token_clear()
  {
    token_string[0] = '\0';
    token_length = 0;
  }

  void LispParserInternal::token_append(char c)
  {
    if (token_length < MAX_TOKEN_LENGTH)
    {
      token_string[token_length++] = c;
      token_string[token_length] = '\0';
    }
  }

  const char* LispParserInternal::token_c_str() const
  {
    return token_string.data();
  }

  int LispParserInternal::next_char(lisp_stream_t* stream)
  {
    switch (stream->type)
    {
      case LISP_STREAM_FILE:
      {
        return fgetc(stream->v.file);
      }
      case LISP_STREAM_STRING:
      {
        if (stream->v.string.pos >= stream->v.string.len)
        {
          return EOF;
        }
        return static_cast<unsigned char>(stream->v.string.buf[stream->v.string.pos++]);
      }
      case LISP_STREAM_ANY:
      {
        return stream->v.any.next_char(stream->v.any.data);
      }
    }
    assert(0);
    return EOF;
  }

  void LispParserInternal::unget_char(char c, lisp_stream_t* stream)
  {
    switch (stream->type)
    {
      case LISP_STREAM_FILE:
      {
        ungetc(c, stream->v.file);
        break;
      }
      case LISP_STREAM_STRING:
      {
        if (stream->v.string.pos > 0)
        {
          --stream->v.string.pos;
        }
        break;
      }
      case LISP_STREAM_ANY:
      {
        stream->v.any.unget_char(c, stream->v.any.data);
        break;
      }
      default:
      {
        assert(0);
      }
    }
  }

  TokenType LispParserInternal::scan(lisp_stream_t* stream)
  {
    int c;

    token_clear();

    do
    {
      c = next_char(stream);
      if (c == EOF)
      {
        return TokenType::END_OF_FILE;
      }
      else if (c == ';')
      {
        while ((c = next_char(stream)) != EOF && c != '\n')
        {
          // Skip comment
        }
        if (c == EOF)
        {
          return TokenType::END_OF_FILE;
        }
      }
    }
    while (g_char_props[static_cast<unsigned char>(c)] & PROP_WHITESPACE);

    switch (c)
    {
      case '(': return TokenType::OPEN_PAREN;
      case ')': return TokenType::CLOSE_PAREN;

      case '"':
      {
        while (true)
        {
          c = next_char(stream);
          if (c == EOF) return TokenType::ERROR;
          if (c == '"') break;

          if (c == '\\')
          {
            c = next_char(stream);
            switch (c)
            {
              case EOF: return TokenType::ERROR;
              case 'n': c = '\n'; break;
              case 't': c = '\t'; break;
            }
          }
          token_append(c);
        }
        return TokenType::STRING;
      }

      case '#':
      {
        c = next_char(stream);
        switch (c)
        {
          case 't': return TokenType::TRUE;
          case 'f': return TokenType::FALSE;
          case '?':
          {
            c = next_char(stream);
            if (c == '(')
            {
              return TokenType::PATTERN_OPEN_PAREN;
            }
            return TokenType::ERROR;
          }
        }
        return TokenType::ERROR;
      }
    }

    unget_char(c, stream);

    c = next_char(stream);

    if (c == '.')
    {
      c = next_char(stream);
      unget_char(c, stream);

      if ((g_char_props[static_cast<unsigned char>(c)] & (PROP_WHITESPACE | PROP_DELIMITER)))
      {
        return TokenType::DOT;
      }
    }

    unget_char(c, stream);

    bool have_digits = false;
    bool have_nondigits = false;
    int dot_count = 0;

    while (true)
    {
      c = next_char(stream);
      if (c == EOF || (g_char_props[static_cast<unsigned char>(c)] & (PROP_WHITESPACE | PROP_DELIMITER)))
      {
        if (c != EOF)
        {
          unget_char(c, stream);
        }
        break;
      }

      token_append(c);

      if (g_char_props[static_cast<unsigned char>(c)] & PROP_DIGIT)
      {
        have_digits = true;
      }
      else if (c == '.')
      {
        dot_count++;
      }
      else if (c != '-')
      {
        have_nondigits = true;
      }
    }

    if (have_nondigits || !have_digits || dot_count > 1)
    {
      return TokenType::SYMBOL;
    }
    else if (dot_count == 1)
    {
      return TokenType::REAL;
    }
    else
    {
      return TokenType::INTEGER;
    }
  }

  lisp_object_t* LispParserInternal::read(lisp_stream_t* in)
  {
    TokenType token = scan(in);
    lisp_object_t* obj = lisp_nil();

    switch (token)
    {
      case TokenType::ERROR:
        return &error_object;

      case TokenType::END_OF_FILE:
        return &end_marker;

      case TokenType::OPEN_PAREN:
      case TokenType::PATTERN_OPEN_PAREN:
      {
        lisp_object_t* last = lisp_nil();
        lisp_object_t* car;

        while (true)
        {
          car = read(in);
          if (car == &error_object || car == &end_marker)
          {
            return &error_object;
          }

          if (car == &close_paren_marker)
          {
            break; // End of list
          }

          if (car == &dot_marker)
          {
            if (lisp_nil_p(last)) return &error_object;
            last->v.cons.cdr = read(in);
            if (read(in) != &close_paren_marker) return &error_object;
            break;
          }

          lisp_object_t* new_cons = (token == TokenType::OPEN_PAREN) ?
          lisp_make_cons(car, lisp_nil()) :
          lisp_make_pattern_cons(car, lisp_nil());
          if (lisp_nil_p(last))
          {
            obj = last = new_cons;
          }
          else
          {
            last->v.cons.cdr = new_cons;
            last = new_cons;
          }
        }
        return obj;
      }

      case TokenType::CLOSE_PAREN:
        return &close_paren_marker;

      case TokenType::SYMBOL:
        return lisp_make_symbol(token_c_str());

      case TokenType::STRING:
        return lisp_make_string(token_c_str());

      case TokenType::INTEGER:
      {
        char* endptr;
        errno = 0;
        long int_val = strtol(token_c_str(), &endptr, 10);
        if (errno != 0 || *endptr != '\0' || int_val < INT_MIN || int_val > INT_MAX)
        {
          return &error_object;
        }
        return lisp_make_integer(static_cast<int>(int_val));
      }

      case TokenType::REAL:
      {
        char* endptr;
        errno = 0;
        float real_val = strtof(token_c_str(), &endptr);
        if (errno != 0 || *endptr != '\0')
        {
          return &error_object;
        }
        return lisp_make_real(real_val);
      }

      case TokenType::DOT:
        return &dot_marker;

      case TokenType::TRUE:
        return lisp_make_boolean(1);

      case TokenType::FALSE:
        return lisp_make_boolean(0);
    }

    assert(0);
    return &error_object;
  }

  int LispParserInternal::compile_pattern(lisp_object_t** obj, int* index)
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
          const char* name;
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

        if (lisp_type(lisp_car(*obj)) != LISP_TYPE_SYMBOL) return 0;

        char* type_name = lisp_symbol(lisp_car(*obj));
        int type = -1;
        for (int i = 0; types[i].name != 0; ++i)
        {
          if (strcmp(types[i].name, type_name) == 0)
          {
            type = types[i].type;
            break;
          }
        }

        if (type == -1) return 0;
        if (type != LISP_PATTERN_OR && lisp_cdr(*obj) != 0) return 0;

        lisp_object_t* pattern = lisp_make_pattern_var(type, (*index)++, lisp_nil());
        if (type == LISP_PATTERN_OR)
        {
          lisp_object_t* cdr = lisp_cdr(*obj);
          if (!compile_pattern(&cdr, index)) return 0;
          pattern->v.pattern.sub = cdr;
          (*obj)->v.cons.cdr = lisp_nil();
        }
        *obj = pattern;
        break;
      }
      case LISP_TYPE_CONS:
      {
        if (!compile_pattern(&(*obj)->v.cons.car, index)) return 0;
        if (!compile_pattern(&(*obj)->v.cons.cdr, index)) return 0;
        break;
      }
    }
    return 1;
  }

  int LispParserInternal::do_match_pattern(lisp_object_t* pattern, lisp_object_t* obj, lisp_object_t** vars, int num_subs)
  {
    if (vars != 0)
    {
      for (int i = 0; i < num_subs; ++i)
      {
        vars[i] = &error_object;
      }
    }
    return match_pattern(pattern, obj, vars);
  }

  int LispParserInternal::match_pattern(lisp_object_t* pattern, lisp_object_t* obj, lisp_object_t** vars)
  {
    if (pattern == 0) return obj == 0;
    if (obj == 0) return 0;

    if (lisp_type(pattern) == LISP_TYPE_PATTERN_VAR)
    {
      return match_pattern_var(pattern, obj, vars);
    }

    if (lisp_type(pattern) != lisp_type(obj)) return 0;

    switch (lisp_type(pattern))
    {
      case LISP_TYPE_SYMBOL:
        return pattern->v.string == obj->v.string; // Pointer comparison
      case LISP_TYPE_STRING:
        return pattern->v.string == obj->v.string; // Pointer comparison
      case LISP_TYPE_INTEGER:
        return lisp_integer(pattern) == lisp_integer(obj);
      case LISP_TYPE_REAL:
        return lisp_real(pattern) == lisp_real(obj);
      case LISP_TYPE_CONS:
        return match_pattern(lisp_car(pattern), lisp_car(obj), vars) &&
        match_pattern(lisp_cdr(pattern), lisp_cdr(obj), vars);
      default:
        assert(0);
    }
    return 0;
  }

  int LispParserInternal::match_pattern_var(lisp_object_t* pattern, lisp_object_t* obj, lisp_object_t** vars)
  {
    assert(lisp_type(pattern) == LISP_TYPE_PATTERN_VAR);
    bool match = false;
    switch (pattern->v.pattern.type)
    {
      case LISP_PATTERN_ANY:     match = true; break;
      case LISP_PATTERN_SYMBOL:  match = (obj && lisp_type(obj) == LISP_TYPE_SYMBOL); break;
      case LISP_PATTERN_STRING:  match = (obj && lisp_type(obj) == LISP_TYPE_STRING); break;
      case LISP_PATTERN_INTEGER: match = (obj && lisp_type(obj) == LISP_TYPE_INTEGER); break;
      case LISP_PATTERN_REAL:    match = (obj && lisp_type(obj) == LISP_TYPE_REAL); break;
      case LISP_PATTERN_BOOLEAN: match = (obj && lisp_type(obj) == LISP_TYPE_BOOLEAN); break;
      case LISP_PATTERN_LIST:    match = (obj && lisp_type(obj) == LISP_TYPE_CONS); break;
      case LISP_PATTERN_OR:
      {
        for (lisp_object_t* sub = pattern->v.pattern.sub; sub != 0; sub = lisp_cdr(sub))
        {
          if (match_pattern(lisp_car(sub), obj, vars))
          {
            match = true;
            break;
          }
        }
        break;
      }
      default: assert(0);
    }

    if (match && vars)
    {
      vars[pattern->v.pattern.index] = obj;
    }
    return match;
  }
} // anonymous namespace

//
// --- Public API ---
//

lisp_stream_t* lisp_stream_init_string(lisp_stream_t* stream, const char* buf, size_t len)
{
  stream->type = LISP_STREAM_STRING;
  stream->v.string.buf = buf;
  stream->v.string.pos = 0;
  stream->v.string.len = len;
  return stream;
}

lisp_stream_t* lisp_stream_init_file(lisp_stream_t* stream, FILE* file)
{
  stream->type = LISP_STREAM_FILE;
  stream->v.file = file;
  return stream;
}

lisp_stream_t* lisp_stream_init_string(lisp_stream_t* stream, const char* buf)
{
  // Maintain backward compatibility.
  return lisp_stream_init_string(stream, buf, buf ? strlen(buf) : 0);
}

lisp_stream_t* lisp_stream_init_any(lisp_stream_t* stream, void* data,
                                    int (*next_char) (void* data),
                                    void (*unget_char) (char c, void* data))
{
  assert(next_char != 0 && unget_char != 0);
  stream->type = LISP_STREAM_ANY;
  stream->v.any.data = data;
  stream->v.any.next_char = next_char;
  stream->v.any.unget_char = unget_char;
  return stream;
}

lisp_object_t* lisp_make_integer(int value)
{
  lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_INTEGER);
  obj->v.integer = value;
  return obj;
}

lisp_object_t* lisp_make_real(float value)
{
  lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_REAL);
  obj->v.real = value;
  return obj;
}

lisp_object_t* lisp_make_symbol(const char* value)
{
  lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_SYMBOL);
  obj->v.string = const_cast<char*>(g_string_interner.intern(value));
  return obj;
}

lisp_object_t* lisp_make_string(const char* value)
{
  lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_STRING);
  obj->v.string = const_cast<char*>(g_string_interner.intern(value));
  return obj;
}

lisp_object_t* lisp_make_cons(lisp_object_t* car, lisp_object_t* cdr)
{
  lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_CONS);
  obj->v.cons.car = car;
  obj->v.cons.cdr = cdr;
  return obj;
}

lisp_object_t* lisp_make_boolean(int value)
{
  lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_BOOLEAN);
  obj->v.integer = value ? 1 : 0;
  return obj;
}

lisp_object_t* lisp_read(lisp_stream_t* in)
{
  LispParserInternal parser;
  return parser.read(in);
}

void lisp_free(lisp_object_t* obj)
{
  if (obj == 0 || obj == &error_object || obj == &end_marker || obj == &close_paren_marker || obj == &dot_marker)
  {
    return;
  }

  switch (obj->type)
  {
    case LISP_TYPE_CONS:
    case LISP_TYPE_PATTERN_CONS:
      lisp_free(obj->v.cons.car);
      lisp_free(obj->v.cons.cdr);
      break;

    case LISP_TYPE_PATTERN_VAR:
      lisp_free(obj->v.pattern.sub);
      break;
  }
}

lisp_object_t* lisp_read_from_string(const char* buf, size_t len)
{
  lisp_stream_t stream;
  lisp_stream_init_string(&stream, buf, len);
  return lisp_read(&stream);
}

lisp_object_t* lisp_read_from_string(const char* buf)
{
  // Maintain backward compatibility.
  lisp_stream_t stream;
  lisp_stream_init_string(&stream, buf);
  return lisp_read(&stream);
}

int lisp_compile_pattern(lisp_object_t** obj, int* num_subs)
{
  int index = 0;
  LispParserInternal parser;
  int result = parser.compile_pattern(obj, &index);

  if (result && num_subs != 0)
  {
    *num_subs = index;
  }
  return result;
}

int lisp_match_pattern(lisp_object_t* pattern, lisp_object_t* obj, lisp_object_t** vars, int num_subs)
{
  LispParserInternal parser;
  return parser.do_match_pattern(pattern, obj, vars, num_subs);
}

int lisp_match_string(const char* pattern_string, lisp_object_t* obj, lisp_object_t** vars)
{
  lisp_object_t* pattern = lisp_read_from_string(pattern_string);
  if (lisp_type(pattern) == LISP_TYPE_EOF || lisp_type(pattern) == LISP_TYPE_PARSE_ERROR)
  {
    return 0;
  }

  int num_subs;
  if (!lisp_compile_pattern(&pattern, &num_subs))
  {
    return 0;
  }

  return lisp_match_pattern(pattern, obj, vars, num_subs);
}

int lisp_type(lisp_object_t* obj)
{
  return obj ? obj->type : LISP_TYPE_NIL;
}

int lisp_integer(lisp_object_t* obj)
{
  assert(obj->type == LISP_TYPE_INTEGER);
  return obj->v.integer;
}

char* lisp_symbol(lisp_object_t* obj)
{
  assert(obj->type == LISP_TYPE_SYMBOL);
  return obj->v.string;
}

char* lisp_string(lisp_object_t* obj)
{
  assert(obj->type == LISP_TYPE_STRING);
  return obj->v.string;
}

int lisp_boolean(lisp_object_t* obj)
{
  assert(obj->type == LISP_TYPE_BOOLEAN);
  return obj->v.integer;
}

float lisp_real(lisp_object_t* obj)
{
  assert(obj->type == LISP_TYPE_REAL || obj->type == LISP_TYPE_INTEGER);
  return (obj->type == LISP_TYPE_INTEGER) ? static_cast<float>(obj->v.integer) : obj->v.real;
}

lisp_object_t* lisp_car(lisp_object_t* obj)
{
  assert(obj->type == LISP_TYPE_CONS || obj->type == LISP_TYPE_PATTERN_CONS);
  return obj->v.cons.car;
}

lisp_object_t* lisp_cdr(lisp_object_t* obj)
{
  assert(obj->type == LISP_TYPE_CONS || obj->type == LISP_TYPE_PATTERN_CONS);
  return obj->v.cons.cdr;
}

lisp_object_t* lisp_cxr(lisp_object_t* obj, const char* x)
{
  if (x == nullptr) return nullptr;
  const size_t len = strlen(x);
  for (size_t i = len; i-- > 0; )
  {
    if (obj == nullptr) return nullptr;
    if (x[i] == 'a') obj = lisp_car(obj);
    else if (x[i] == 'd') obj = lisp_cdr(obj);
    else assert(0);
  }
  return obj;
}

int lisp_list_length(lisp_object_t* obj)
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

lisp_object_t* lisp_list_nth_cdr(lisp_object_t* obj, int index)
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

lisp_object_t* lisp_list_nth(lisp_object_t* obj, int index)
{
  obj = lisp_list_nth_cdr(obj, index);
  assert(obj != 0);
  return obj->v.cons.car;
}

void lisp_dump(lisp_object_t* obj, FILE* out)
{
  if (obj == 0)
  {
    fprintf(out, "()");
    return;
  }

  switch (lisp_type(obj))
  {
    case LISP_TYPE_EOF: fputs("#<eof>", out); break;
    case LISP_TYPE_PARSE_ERROR: fputs("#<error>", out); break;
    case LISP_TYPE_INTEGER: fprintf(out, "%d", lisp_integer(obj)); break;
    case LISP_TYPE_REAL: fprintf(out, "%f", lisp_real(obj)); break;
    case LISP_TYPE_SYMBOL: fputs(lisp_symbol(obj), out); break;
    case LISP_TYPE_STRING:
    {
      fputc('"', out);
      for (char* p = lisp_string(obj); *p != 0; ++p)
      {
        if (*p == '"' || *p == '\\') fputc('\\', out);
        fputc(*p, out);
      }
      fputc('"', out);
      break;
    }
    case LISP_TYPE_CONS:
    case LISP_TYPE_PATTERN_CONS:
    {
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
          fputc(' ', out);
        }
      }
      fputc(')', out);
      break;
    }
    case LISP_TYPE_BOOLEAN: fputs(lisp_boolean(obj) ? "#t" : "#f", out); break;
    default: assert(0);
  }
}

LispReader::LispReader(lisp_object_t* l) : lst(l)
{
  for (lisp_object_t* cursor = lst; !lisp_nil_p(cursor); cursor = lisp_cdr(cursor))
  {
    lisp_object_t* cur = lisp_car(cursor);
    if (lisp_cons_p(cur) && lisp_symbol_p(lisp_car(cur)))
    {
      property_map[lisp_symbol(lisp_car(cur))] = lisp_cdr(cur);
    }
  }
}

lisp_object_t* LispReader::search_for(const char* name)
{
  const char* interned_name = g_string_interner.intern(name);
  auto it = property_map.find(interned_name);
  if (it != property_map.end())
  {
    return it->second;
  }
  return nullptr;
}

bool LispReader::read_int(const char* name, int* i)
{
  lisp_object_t* obj = search_for(name);
  if (obj && lisp_integer_p(lisp_car(obj)))
  {
    *i = lisp_integer(lisp_car(obj));
    return true;
  }
  return false;
}

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

bool LispReader::read_float(const char* name, float* f)
{
  lisp_object_t* obj = search_for(name);
  if (obj)
  {
    lisp_object_t* val = lisp_car(obj);
    if (lisp_real_p(val) || lisp_integer_p(val))
    {
      *f = lisp_real(val);
      return true;
    }
  }
  return false;
}

bool LispReader::read_string_vector(const char* name, std::vector<std::string>* vec)
{
  lisp_object_t* obj = search_for(name);
  if (obj)
  {
    while (!lisp_nil_p(obj))
    {
      if (!lisp_string_p(lisp_car(obj))) return false;
      vec->push_back(lisp_string(lisp_car(obj)));
      obj = lisp_cdr(obj);
    }
    return true;
  }
  return false;
}

bool LispReader::read_int_vector(const char* name, std::vector<int>* vec)
{
  lisp_object_t* obj = search_for(name);
  if (obj)
  {
    while (!lisp_nil_p(obj))
    {
      if (!lisp_integer_p(lisp_car(obj))) return false;
      vec->push_back(lisp_integer(lisp_car(obj)));
      obj = lisp_cdr(obj);
    }
    return true;
  }
  return false;
}

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

bool LispReader::read_string(const char* name, std::string* str)
{
  lisp_object_t* obj = search_for(name);
  if (obj && lisp_string_p(lisp_car(obj)))
  {
    *str = lisp_string(lisp_car(obj));
    return true;
  }
  return false;
}

bool LispReader::read_bool(const char* name, bool* b)
{
  lisp_object_t* obj = search_for(name);
  if (obj && lisp_boolean_p(lisp_car(obj)))
  {
    *b = lisp_boolean(lisp_car(obj));
    return true;
  }
  return false;
}

LispWriter::LispWriter(const char* name)
{
  lisp_objs.push_back(lisp_make_symbol(name));
}

void LispWriter::append(lisp_object_t* obj)
{
  lisp_objs.push_back(obj);
}

lisp_object_t* LispWriter::make_list3(lisp_object_t* a, lisp_object_t* b, lisp_object_t* c)
{
  return lisp_make_cons(a, lisp_make_cons(b, lisp_make_cons(c, lisp_nil())));
}

lisp_object_t* LispWriter::make_list2(lisp_object_t* a, lisp_object_t* b)
{
  return lisp_make_cons(a, lisp_make_cons(b, lisp_nil()));
}

void LispWriter::write_float(const char* name, float f)
{
  append(make_list2(lisp_make_symbol(name), lisp_make_real(f)));
}

void LispWriter::write_int(const char* name, int i)
{
  append(make_list2(lisp_make_symbol(name), lisp_make_integer(i)));
}

void LispWriter::write_string(const char* name, const char* str)
{
  append(make_list2(lisp_make_symbol(name), lisp_make_string(str)));
}

void LispWriter::write_symbol(const char* name, const char* symname)
{
  append(make_list2(lisp_make_symbol(name), lisp_make_symbol(symname)));
}

void LispWriter::write_lisp_obj(const char* name, lisp_object_t* lst)
{
  append(make_list2(lisp_make_symbol(name), lst));
}

void LispWriter::write_boolean(const char* name, bool b)
{
  append(make_list2(lisp_make_symbol(name), lisp_make_boolean(b)));
}

lisp_object_t* LispWriter::create_lisp()
{
  lisp_object_t* lisp_obj = lisp_nil();
  for (std::vector<lisp_object_t*>::reverse_iterator i = lisp_objs.rbegin(); i != lisp_objs.rend(); ++i)
  {
    lisp_obj = lisp_make_cons(*i, lisp_obj);
  }
  lisp_objs.clear();
  return lisp_obj;
}

lisp_object_t* lisp_read_from_file(const std::string& filename)
{
  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
  {
    return nullptr;
  }

  fseek(in, 0, SEEK_END);
  long file_size = ftell(in);
  fseek(in, 0, SEEK_SET);

  if (file_size < 0)
  {
    fclose(in);
    return nullptr;
  }

  std::vector<char> buffer(file_size);
  if (fread(buffer.data(), 1, file_size, in) != static_cast<size_t>(file_size))
  {
    fclose(in);
    return nullptr;
  }
  fclose(in);

  // Call the new overload of lisp_read_from_string that takes a size.
  return lisp_read_from_string(buffer.data(), file_size);
}

void lisp_reset_pool()
{
  g_object_pool.reset();
}
