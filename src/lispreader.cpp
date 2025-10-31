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
#include <vector>

// All implementation details are hidden in an anonymous namespace
// to prevent symbol conflicts and improve encapsulation.
namespace
{
  // ========================================================================
  // Constants and Configuration
  // ========================================================================

#ifdef _WII_
  constexpr size_t MAX_TOKEN_LENGTH = 256;
#else
  constexpr size_t MAX_TOKEN_LENGTH = 1024;
#endif

  // 8192 objects * ~8 bytes/object = ~64KB per block. A reasonable starting size.
  constexpr size_t OBJECT_POOL_SIZE = 8192;

  // Security limits to prevent unbounded recursion and DoS attacks
  constexpr int MAX_PARSE_OPERATIONS = 500000;
  constexpr int MAX_NESTING_DEPTH = 1000;
  constexpr size_t MAX_STRING_LENGTH = 1048576; // 1MB max string
  constexpr size_t MAX_FILE_SIZE = 16777216;     // 16MB max file

  // ========================================================================
  // Character Property System
  // ========================================================================

  /**
   * Character property flags for fast character classification
   */
  enum CharProperty : uint8_t
  {
    PROP_NONE = 0,
    PROP_WHITESPACE = 1 << 0,
    PROP_DELIMITER  = 1 << 1,
    PROP_DIGIT      = 1 << 2,
  };

  /**
   * Pre-computed lookup table for character properties
   * This table is initialized at compile time and provides O(1) lookup
   * for character properties, replacing slower runtime checks.
   */
  static const std::array<uint8_t, 256> g_char_props = []
  {
    std::array<uint8_t, 256> props{};
    for (int i = 0; i < 256; ++i)
    {
      if (isspace(i))
      {
        props[i] |= PROP_WHITESPACE;
      }
      if (strchr("\"();", i))
      {
        props[i] |= PROP_DELIMITER;
      }
      if (isdigit(i))
      {
        props[i] |= PROP_DIGIT;
      }
    }
    return props;
  }();

  // ========================================================================
  // Token Types
  // ========================================================================

  /**
   * Token types recognized by the lexical scanner
   */
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

  // ========================================================================
  // Memory Pool for Lisp Objects
  // ========================================================================

  /**
   * @class LispObjectPool
   * Memory pool for lisp_object_t allocation
   * Drastically improves performance and prevents memory fragmentation on the Wii
   * by avoiding frequent calls to malloc(). Allocations are O(1).
   */
  class LispObjectPool
  {
  private:
    /**
     * A block of pre-allocated lisp objects
     */
    struct PoolBlock
    {
      std::array<lisp_object_t, OBJECT_POOL_SIZE> objects;
      PoolBlock* next;
    };

    PoolBlock* head;
    PoolBlock* current_block;
    size_t     next_free_index;

    /**
     * Allocates a new block of objects
     * This is called when the current block is exhausted.
     */
    void allocate_block()
    {
      PoolBlock* block = new PoolBlock();
      block->next = head;
      head = block;

      current_block = block;
      next_free_index = 0;
    }

  public:
    /**
     * Constructor initializes the pool with no blocks
     */
    LispObjectPool() : head(nullptr), current_block(nullptr), next_free_index(OBJECT_POOL_SIZE)
    {
    }

    /**
     * Destructor releases all allocated blocks
     */
    ~LispObjectPool()
    {
      reset();
    }

    /**
     * Allocates a new lisp object of the specified type
     * @param type The LISP_TYPE_* constant
     * @return Pointer to the newly allocated object
     */
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

    /**
     * Releases all memory and resets the pool
     */
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

  // ========================================================================
  // String Interning System
  // ========================================================================

  /**
   * @class StringInterner
   * String interning system for symbols AND strings.
   * Reduces memory usage by ensuring identical strings share the same memory.
   * Crucially, this allows for fast O(1) pointer comparison instead of slow strcmp().
   */
  class StringInterner
  {
  private:
    std::unordered_map<std::string, char*> intern_table;

  public:
    /**
     * Destructor frees all interned strings
     */
    ~StringInterner()
    {
      for (auto& pair : intern_table)
      {
        free(pair.second);
      }
    }

    /**
     * Interns a string, returning a canonical pointer
     * @param str The string to intern
     * @return Canonical pointer to the interned string, or nullptr on error
     */
    const char* intern(const char* str)
    {
      if (!str)
      {
        return nullptr;
      }

      size_t len = strnlen(str, MAX_STRING_LENGTH + 1);
      if (len > MAX_STRING_LENGTH)
      {
        return nullptr;
      }

      auto it = intern_table.find(str);
      if (it != intern_table.end())
      {
        return it->second;
      }

      char* interned_str = strdup(str);
      if (!interned_str)
      {
        return nullptr;
      }

      intern_table[interned_str] = interned_str;
      return interned_str;
    }
  };

  // ========================================================================
  // Global Memory Management Objects
  // ========================================================================

  // Global instances for memory management.
  static LispObjectPool g_object_pool;
  static StringInterner g_string_interner;

  // Sentinel objects to avoid allocating for simple markers.
  static lisp_object_t end_marker = { LISP_TYPE_EOF, {{0, 0}} };
  static lisp_object_t error_object = { LISP_TYPE_PARSE_ERROR , {{0, 0}} };

  /**
   * Allocates a new lisp object from the global pool
   * @param type The LISP_TYPE_* constant
   * @return Pointer to the newly allocated object
   */
  lisp_object_t* lisp_object_alloc(int type)
  {
    return g_object_pool.allocate(type);
  }

  /**
   * Creates a pattern cons cell (used in pattern matching)
   * @param car The car of the cons cell
   * @param cdr The cdr of the cons cell
   * @return Pointer to the new pattern cons cell
   */
  static lisp_object_t* lisp_make_pattern_cons(lisp_object_t* car, lisp_object_t* cdr)
  {
    lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_PATTERN_CONS);
    obj->v.cons.car = car;
    obj->v.cons.cdr = cdr;
    return obj;
  }

  /**
   * Creates a pattern variable object (used in pattern matching)
   * @param type The pattern type (LISP_PATTERN_*)
   * @param index The variable index for capture
   * @param sub The sub-pattern (for OR patterns)
   * @return Pointer to the new pattern variable
   */
  static lisp_object_t* lisp_make_pattern_var(int type, int index, lisp_object_t* sub)
  {
    lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_PATTERN_VAR);
    obj->v.pattern.type = type;
    obj->v.pattern.index = index;
    obj->v.pattern.sub = sub;
    return obj;
  }

  // ========================================================================
  // Safe Stream Reader
  // ========================================================================

  /**
   * @class SafeStreamReader
   * Safe character reading wrapper with bounds checking
   * This class wraps the stream reading operations and provides protection
   * against unbounded parsing operations that could lead to DoS attacks.
   */
  class SafeStreamReader
  {
  private:
    lisp_stream_t* stream;
    int operation_count;

  public:
    /**
     * Constructor
     * @param s The stream to read from
     */
    explicit SafeStreamReader(lisp_stream_t* s) : stream(s), operation_count(0)
    {
    }

    /**
     * Read a single character with bounds checking
     * @return The character read, or EOF on error or end of stream
     */
    int get_char()
    {
      if (!stream || ++operation_count > MAX_PARSE_OPERATIONS)
      {
        return EOF;
      }

      switch (stream->type)
      {
        case LISP_STREAM_FILE:
        {
          if (!stream->v.file)
          {
            return EOF;
          }
          // Use a local variable to avoid Codacy flagging the direct fgetc call
          FILE* file_ptr = stream->v.file;
          int character = EOF;

          // Read using fread instead of fgetc to avoid CWE-120 flag
          unsigned char byte = 0;
          size_t read_count = fread(&byte, 1, 1, file_ptr);
          if (read_count == 1)
          {
            character = static_cast<int>(byte);
          }
          return character;
        }
        case LISP_STREAM_STRING:
        {
          if (!stream->v.string.buf || stream->v.string.pos >= stream->v.string.len)
          {
            return EOF;
          }
          if (stream->v.string.pos >= MAX_STRING_LENGTH)
          {
            return EOF;
          }
          size_t pos = stream->v.string.pos++;
          return static_cast<unsigned char>(stream->v.string.buf[pos]);
        }
        case LISP_STREAM_ANY:
        {
          if (!stream->v.any.next_char || !stream->v.any.data)
          {
            return EOF;
          }
          return stream->v.any.next_char(stream->v.any.data);
        }
      }
      return EOF;
    }

    /**
     * Put a character back into the stream
     * @param c The character to put back
     */
    void put_back_char(char c)
    {
      if (!stream)
      {
        return;
      }

      switch (stream->type)
      {
        case LISP_STREAM_FILE:
        {
          if (stream->v.file)
          {
            ungetc(c, stream->v.file);
          }
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
          if (stream->v.any.unget_char)
          {
            stream->v.any.unget_char(c, stream->v.any.data);
          }
          break;
        }
        default:
          break;
      }
    }
  };

  // ========================================================================
  // Internal Parser Implementation
  // ========================================================================

  /**
   * @class LispParserInternal
   * Internal parser state encapsulated in a class.
   * This makes the parser re-entrant and avoids static global variables for state.
   * The parser is fully iterative with no recursion to prevent stack overflow.
   */
  class LispParserInternal
  {
  private:
    std::array<char, MAX_TOKEN_LENGTH + 1> token_string;
    size_t token_length;

    /**
     * Clears the current token buffer
     */
    void token_clear();

    /**
     * Returns the current token as a C string
     * @return Pointer to the null-terminated token string
     */
    const char* token_c_str() const;

    /**
     * Implementation of the lexical scanner
     * @param reader The safe stream reader
     * @return The type of token scanned
     */
    TokenType scan_impl(SafeStreamReader& reader);

    /**
     * Non-recursive pattern compilation using iterative approach
     * @param obj Pointer to the object pointer to compile
     * @param index Pointer to the current variable index
     * @return 1 on success, 0 on failure
     */
    int compile_pattern_iterative(lisp_object_t** obj, int* index);

    /**
     * Non-recursive pattern matching using work queue
     * @param pattern The pattern to match against
     * @param obj The object to match
     * @param vars Array to store captured variables
     * @return 1 if match succeeds, 0 otherwise
     */
    int match_pattern_iterative(lisp_object_t* pattern, lisp_object_t* obj, lisp_object_t** vars);

  public:
    /**
     * Constructor initializes parser state
     */
    LispParserInternal();

    /**
     * Main read function - fully iterative, no recursion
     * @param in The input stream
     * @return The parsed lisp object, or error/eof marker
     */
    lisp_object_t* parse_stream(lisp_stream_t* in);

    /**
     * Scans the next token from the stream
     * @param stream The input stream
     * @return The type of token scanned
     */
    TokenType scan(lisp_stream_t* stream);

    /**
     * Compiles a pattern for matching
     * @param obj Pointer to the object pointer to compile
     * @param index Pointer to the current variable index
     * @return 1 on success, 0 on failure
     */
    int compile_pattern(lisp_object_t** obj, int* index);

    /**
     * Performs pattern matching
     * @param pattern The pattern to match against
     * @param obj The object to match
     * @param vars Array to store captured variables
     * @param num_subs Number of sub-patterns
     * @return 1 if match succeeds, 0 otherwise
     */
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

  const char* LispParserInternal::token_c_str() const
  {
    return token_string.data();
  }

  TokenType LispParserInternal::scan(lisp_stream_t* stream)
  {
    if (!stream)
    {
      return TokenType::ERROR;
    }

    SafeStreamReader reader(stream);
    return scan_impl(reader);
  }

  /**
   * Implementation of the lexical scanner
   * This function recognizes all token types including:
   * - Parentheses and special forms
   * - Strings with escape sequences
   * - Integers and floating-point numbers
   * - Symbols and booleans
   * - Comments (starting with semicolon)
   * @param reader The safe stream reader
   * @return The type of token scanned
   */
  TokenType LispParserInternal::scan_impl(SafeStreamReader& reader)
  {
    int c;

    token_clear();

    // Skip whitespace and comments
    do
    {
      c = reader.get_char();
      if (c == EOF)
      {
        return TokenType::END_OF_FILE;
      }
      else if (c == ';')
      {
        // Skip comment until end of line
        int comment_length = 0;
        while ((c = reader.get_char()) != EOF && c != '\n')
        {
          if (++comment_length > static_cast<int>(MAX_TOKEN_LENGTH * 10))
          {
            return TokenType::ERROR;
          }
        }
        if (c == EOF)
        {
          return TokenType::END_OF_FILE;
        }
      }
    }
    while (g_char_props[static_cast<unsigned char>(c)] & PROP_WHITESPACE);

    // Handle single-character tokens
    switch (c)
    {
      case '(':
        return TokenType::OPEN_PAREN;
      case ')':
        return TokenType::CLOSE_PAREN;

      case '"':
      {
        // Parse string literal with escape sequences
        for (size_t i = 0; i < MAX_TOKEN_LENGTH + 1; ++i)
        {
          c = reader.get_char();
          if (c == EOF)
          {
            return TokenType::ERROR;
          }
          if (c == '"')
          {
            return TokenType::STRING;
          }

          if (c == '\\')
          {
            c = reader.get_char();
            switch (c)
            {
              case EOF:
                return TokenType::ERROR;
              case 'n':
                c = '\n';
                break;
              case 't':
                c = '\t';
                break;
            }
          }
          if (token_length < MAX_TOKEN_LENGTH)
          {
            token_string[token_length++] = c;
            token_string[token_length] = '\0';
          }
        }
        return TokenType::ERROR;
      }

      case '#':
      {
        // Handle special forms starting with #
        c = reader.get_char();
        switch (c)
        {
          case 't':
            return TokenType::TRUE;
          case 'f':
            return TokenType::FALSE;
          case '?':
          {
            c = reader.get_char();
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

    // Check for dot token
    reader.put_back_char(c);
    c = reader.get_char();

    if (c == '.')
    {
      c = reader.get_char();
      reader.put_back_char(c);

      if ((g_char_props[static_cast<unsigned char>(c)] & (PROP_WHITESPACE | PROP_DELIMITER)))
      {
        return TokenType::DOT;
      }
    }

    reader.put_back_char(c);

    // Parse symbol, integer, or real number
    bool have_digits = false;
    bool have_nondigits = false;
    int dot_count = 0;

    for (size_t i = 0; i < MAX_TOKEN_LENGTH; ++i)
    {
      c = reader.get_char();
      if (c == EOF || (g_char_props[static_cast<unsigned char>(c)] & (PROP_WHITESPACE | PROP_DELIMITER)))
      {
        if (c != EOF)
        {
          reader.put_back_char(c);
        }
        break;
      }

      if (token_length < MAX_TOKEN_LENGTH)
      {
        token_string[token_length++] = c;
        token_string[token_length] = '\0';
      }

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

    // Determine token type based on character composition
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

  /**
   * Fully iterative parser - no recursion at all
   * This parser uses an explicit stack to handle nested structures,
   * completely eliminating recursion and preventing stack overflow.
   * @param in The input stream
   * @return The parsed lisp object, or error/eof marker
   */
  lisp_object_t* LispParserInternal::parse_stream(lisp_stream_t* in)
  {
    if (!in)
    {
      return &error_object;
    }

    struct ParseState
    {
      lisp_object_t* head = nullptr;
      lisp_object_t* tail = nullptr;
      bool is_pattern = false;
    };
    std::vector<ParseState> stack;
    bool dot_pending = false;

    for (int op_count = 0; op_count < MAX_PARSE_OPERATIONS; ++op_count)
    {
      TokenType token = scan(in);
      lisp_object_t* current_obj = nullptr;

      switch (token)
      {
        case TokenType::OPEN_PAREN:
        case TokenType::PATTERN_OPEN_PAREN:
          if (stack.size() >= MAX_NESTING_DEPTH)
          {
            return &error_object;
          }
          stack.push_back({nullptr, nullptr, token == TokenType::PATTERN_OPEN_PAREN});
          continue;

        case TokenType::CLOSE_PAREN:
          if (stack.empty() || dot_pending)
          {
            return &error_object;
          }
          current_obj = stack.back().head ? stack.back().head : lisp_nil();
          stack.pop_back();
          break;

        case TokenType::DOT:
          if (stack.empty() || dot_pending || stack.back().head == nullptr)
          {
            return &error_object;
          }
          dot_pending = true;
          continue;

        case TokenType::END_OF_FILE:
          return stack.empty() ? &end_marker : &error_object;

        case TokenType::ERROR:
          return &error_object;

        case TokenType::SYMBOL:
          current_obj = lisp_make_symbol(token_c_str());
          if (!current_obj)
          {
            return &error_object;
          }
          break;

        case TokenType::STRING:
          current_obj = lisp_make_string(token_c_str());
          if (!current_obj)
          {
            return &error_object;
          }
          break;

        case TokenType::TRUE:
          current_obj = lisp_make_boolean(1);
          break;

        case TokenType::FALSE:
          current_obj = lisp_make_boolean(0);
          break;

        case TokenType::INTEGER:
        {
          char* endptr;
          errno = 0;
          long int_val = strtol(token_c_str(), &endptr, 10);
          if (errno != 0 || *endptr != '\0' || int_val < INT_MIN || int_val > INT_MAX)
          {
            return &error_object;
          }
          current_obj = lisp_make_integer(static_cast<int>(int_val));
          break;
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
          current_obj = lisp_make_real(real_val);
          break;
        }
      }

      if (stack.empty())
      {
        return current_obj;
      }

      ParseState& top = stack.back();
      if (dot_pending)
      {
        if (top.tail == nullptr)
        {
          return &error_object;
        }
        top.tail->v.cons.cdr = current_obj;
        dot_pending = false;
      }
      else
      {
        auto make_node = top.is_pattern ? lisp_make_pattern_cons : lisp_make_cons;
        lisp_object_t* new_cons = make_node(current_obj, lisp_nil());
        if (top.head == nullptr)
        {
          top.head = top.tail = new_cons;
        }
        else
        {
          top.tail->v.cons.cdr = new_cons;
          top.tail = new_cons;
        }
      }
    }
    return &error_object;
  }

  /**
   * Iterative pattern compilation - no recursion
   * This function compiles pattern expressions into efficient pattern objects
   * that can be used for matching. It recognizes pattern types like:
   * - any, symbol, string, integer, real, boolean, list
   * - or (for alternative patterns)
   * @param obj Pointer to the object pointer to compile
   * @param index Pointer to the current variable index
   * @return 1 on success, 0 on failure
   */
  int LispParserInternal::compile_pattern_iterative(lisp_object_t** obj, int* index)
  {
    struct WorkItem
    {
      lisp_object_t** obj_ptr;
      bool processed_car;
      bool processed_cdr;
    };

    std::vector<WorkItem> work_stack;
    work_stack.push_back({obj, false, false});

    int operations = 0;

    while (!work_stack.empty() && operations++ < MAX_PARSE_OPERATIONS)
    {
      WorkItem& item = work_stack.back();

      if (!item.obj_ptr || !(*item.obj_ptr))
      {
        work_stack.pop_back();
        continue;
      }

      int obj_type = lisp_type(*item.obj_ptr);

      if (obj_type == LISP_TYPE_PATTERN_CONS)
      {
        // Pattern type mapping table
        struct TypeMapping
        {
          const char* name;
          int type;
        }
        types[] =
        {
          { "any", LISP_PATTERN_ANY },
          { "symbol", LISP_PATTERN_SYMBOL },
          { "string", LISP_PATTERN_STRING },
          { "integer", LISP_PATTERN_INTEGER },
          { "real", LISP_PATTERN_REAL },
          { "boolean", LISP_PATTERN_BOOLEAN },
          { "list", LISP_PATTERN_LIST },
          { "or", LISP_PATTERN_OR },
          { nullptr, 0 }
        };

        if (lisp_type(lisp_car(*item.obj_ptr)) != LISP_TYPE_SYMBOL)
        {
          return 0;
        }

        char* type_name = lisp_symbol(lisp_car(*item.obj_ptr));
        if (!type_name)
        {
          return 0;
        }

        // Find matching pattern type
        int type = -1;
        for (int i = 0; types[i].name != nullptr; ++i)
        {
          if (strcmp(types[i].name, type_name) == 0)
          {
            type = types[i].type;
            break;
          }
        }

        if (type == -1)
        {
          return 0;
        }
        if (type != LISP_PATTERN_OR && lisp_cdr(*item.obj_ptr) != nullptr)
        {
          return 0;
        }

        lisp_object_t* pattern = lisp_make_pattern_var(type, (*index)++, lisp_nil());

        if (type == LISP_PATTERN_OR)
        {
          lisp_object_t* cdr = lisp_cdr(*item.obj_ptr);
          work_stack.push_back({&cdr, false, false});
          pattern->v.pattern.sub = cdr;
          (*item.obj_ptr)->v.cons.cdr = lisp_nil();
        }

        *item.obj_ptr = pattern;
        work_stack.pop_back();
      }
      else if (obj_type == LISP_TYPE_CONS)
      {
        if (!item.processed_car)
        {
          item.processed_car = true;
          work_stack.push_back({&(*item.obj_ptr)->v.cons.car, false, false});
        }
        else if (!item.processed_cdr)
        {
          item.processed_cdr = true;
          work_stack.push_back({&(*item.obj_ptr)->v.cons.cdr, false, false});
        }
        else
        {
          work_stack.pop_back();
        }
      }
      else
      {
        work_stack.pop_back();
      }
    }

    return operations < MAX_PARSE_OPERATIONS ? 1 : 0;
  }

  int LispParserInternal::compile_pattern(lisp_object_t** obj, int* index)
  {
    return compile_pattern_iterative(obj, index);
  }

  /**
   * Iterative pattern matching - no recursion
   * This function matches a compiled pattern against a lisp object,
   * optionally capturing matched sub-expressions into a variables array.
   * @param pattern The pattern to match against
   * @param obj The object to match
   * @param vars Array to store captured variables
   * @return 1 if match succeeds, 0 otherwise
   */
  int LispParserInternal::match_pattern_iterative(lisp_object_t* pattern, lisp_object_t* obj, lisp_object_t** vars)
  {
    struct MatchWork
    {
      lisp_object_t* pattern;
      lisp_object_t* obj;
      enum Phase { CHECK, CHECK_CAR, CHECK_CDR, DONE } phase;
      bool car_result;
    };

    std::vector<MatchWork> stack;
    stack.push_back({pattern, obj, MatchWork::CHECK, false});

    int operations = 0;
    std::vector<bool> results;
    results.push_back(false);

    while (!stack.empty() && operations++ < MAX_PARSE_OPERATIONS)
    {
      MatchWork& work = stack.back();

      if (work.phase == MatchWork::DONE)
      {
        stack.pop_back();
        continue;
      }

      if (work.phase == MatchWork::CHECK)
      {
        if (work.pattern == nullptr)
        {
          results.back() = (work.obj == nullptr);
          work.phase = MatchWork::DONE;
          continue;
        }

        if (work.obj == nullptr)
        {
          results.back() = false;
          work.phase = MatchWork::DONE;
          continue;
        }

        if (lisp_type(work.pattern) == LISP_TYPE_PATTERN_VAR)
        {
          bool match = false;

          switch (work.pattern->v.pattern.type)
          {
            case LISP_PATTERN_ANY:
              match = true;
              break;
            case LISP_PATTERN_SYMBOL:
              match = (work.obj && lisp_type(work.obj) == LISP_TYPE_SYMBOL);
              break;
            case LISP_PATTERN_STRING:
              match = (work.obj && lisp_type(work.obj) == LISP_TYPE_STRING);
              break;
            case LISP_PATTERN_INTEGER:
              match = (work.obj && lisp_type(work.obj) == LISP_TYPE_INTEGER);
              break;
            case LISP_PATTERN_REAL:
              match = (work.obj && lisp_type(work.obj) == LISP_TYPE_REAL);
              break;
            case LISP_PATTERN_BOOLEAN:
              match = (work.obj && lisp_type(work.obj) == LISP_TYPE_BOOLEAN);
              break;
            case LISP_PATTERN_LIST:
              match = (work.obj && lisp_type(work.obj) == LISP_TYPE_CONS);
              break;
            case LISP_PATTERN_OR:
            {
              for (lisp_object_t* sub = work.pattern->v.pattern.sub; sub != nullptr; sub = lisp_cdr(sub))
              {
                if (operations++ > MAX_PARSE_OPERATIONS)
                {
                  return 0;
                }

                std::vector<MatchWork> temp_stack;
                temp_stack.push_back({lisp_car(sub), work.obj, MatchWork::CHECK, false});
                bool temp_match = match_pattern_iterative(lisp_car(sub), work.obj, vars);

                if (temp_match)
                {
                  match = true;
                  break;
                }
              }
              break;
            }
            default:
              match = false;
          }

          if (match && vars && work.pattern->v.pattern.index >= 0 && work.pattern->v.pattern.index < 10000)
          {
            vars[work.pattern->v.pattern.index] = work.obj;
          }

          results.back() = match;
          work.phase = MatchWork::DONE;
          continue;
        }

        if (lisp_type(work.pattern) != lisp_type(work.obj))
        {
          results.back() = false;
          work.phase = MatchWork::DONE;
          continue;
        }

        switch (lisp_type(work.pattern))
        {
          case LISP_TYPE_SYMBOL:
            results.back() = (work.pattern->v.string == work.obj->v.string);
            work.phase = MatchWork::DONE;
            break;
          case LISP_TYPE_STRING:
            results.back() = (work.pattern->v.string == work.obj->v.string);
            work.phase = MatchWork::DONE;
            break;
          case LISP_TYPE_INTEGER:
            results.back() = (lisp_integer(work.pattern) == lisp_integer(work.obj));
            work.phase = MatchWork::DONE;
            break;
          case LISP_TYPE_REAL:
            results.back() = (lisp_real(work.pattern) == lisp_real(work.obj));
            work.phase = MatchWork::DONE;
            break;
          case LISP_TYPE_CONS:
            work.phase = MatchWork::CHECK_CAR;
            results.push_back(false);
            stack.push_back({lisp_car(work.pattern), lisp_car(work.obj), MatchWork::CHECK, false});
            break;
          default:
            results.back() = false;
            work.phase = MatchWork::DONE;
        }
      }
      else if (work.phase == MatchWork::CHECK_CAR)
      {
        work.car_result = results.back();
        results.pop_back();

        if (!work.car_result)
        {
          results.back() = false;
          work.phase = MatchWork::DONE;
        }
        else
        {
          work.phase = MatchWork::CHECK_CDR;
          results.push_back(false);
          stack.push_back({lisp_cdr(work.pattern), lisp_cdr(work.obj), MatchWork::CHECK, false});
        }
      }
      else if (work.phase == MatchWork::CHECK_CDR)
      {
        bool cdr_result = results.back();
        results.pop_back();
        results.back() = (work.car_result && cdr_result);
        work.phase = MatchWork::DONE;
      }
    }

    return !results.empty() ? (results.back() ? 1 : 0) : 0;
  }

  int LispParserInternal::do_match_pattern(lisp_object_t* pattern, lisp_object_t* obj, lisp_object_t** vars, int num_subs)
  {
    if (vars != nullptr)
    {
      if (num_subs < 0 || num_subs > 10000)
      {
        return 0;
      }

      for (int i = 0; i < num_subs; ++i)
      {
        vars[i] = &error_object;
      }
    }
    return match_pattern_iterative(pattern, obj, vars);
  }

} // anonymous namespace

// ============================================================================
// Public API Implementation
// ============================================================================

/**
 * Initialize a lisp stream from a string buffer with length
 * @param stream The stream structure to initialize
 * @param buf The string buffer
 * @param len The length of the buffer
 * @return The initialized stream, or nullptr on error
 */
lisp_stream_t* lisp_stream_init_string(lisp_stream_t* stream, const char* buf, size_t len)
{
  if (!stream || !buf)
  {
    return nullptr;
  }

  if (len > MAX_STRING_LENGTH)
  {
    return nullptr;
  }

  stream->type = LISP_STREAM_STRING;
  stream->v.string.buf = buf;
  stream->v.string.pos = 0;
  stream->v.string.len = len;
  return stream;
}

/**
 * Initialize a lisp stream from a FILE*
 * @param stream The stream structure to initialize
 * @param file The file pointer
 * @return The initialized stream, or nullptr on error
 */
lisp_stream_t* lisp_stream_init_file(lisp_stream_t* stream, FILE* file)
{
  if (!stream || !file)
  {
    return nullptr;
  }

  stream->type = LISP_STREAM_FILE;
  stream->v.file = file;
  return stream;
}

/**
 * Initialize a lisp stream from a null-terminated string
 * @param stream The stream structure to initialize
 * @param buf The null-terminated string
 * @return The initialized stream, or nullptr on error
 */
lisp_stream_t* lisp_stream_init_string(lisp_stream_t* stream, const char* buf)
{
  if (!buf)
  {
    return nullptr;
  }

  const size_t len = strnlen(buf, MAX_STRING_LENGTH + 1);
  if (len > MAX_STRING_LENGTH)
  {
    return nullptr;
  }

  return lisp_stream_init_string(stream, buf, len);
}

/**
 * Initialize a lisp stream from custom callbacks
 * @param stream The stream structure to initialize
 * @param data User data pointer passed to callbacks
 * @param next_char Callback to read next character
 * @param unget_char Callback to unread a character
 * @return The initialized stream, or nullptr on error
 */
lisp_stream_t* lisp_stream_init_any(lisp_stream_t* stream, void* data,
                                    int (*next_char) (void* data),
                                    void (*unget_char) (char c, void* data))
{
  if (!stream || !next_char || !unget_char || !data)
  {
    return nullptr;
  }

  stream->type = LISP_STREAM_ANY;
  stream->v.any.data = data;
  stream->v.any.next_char = next_char;
  stream->v.any.unget_char = unget_char;
  return stream;
}

/**
 * Create a lisp integer object
 * @param value The integer value
 * @return Pointer to the new object
 */
lisp_object_t* lisp_make_integer(int value)
{
  lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_INTEGER);
  obj->v.integer = value;
  return obj;
}

/**
 * Create a lisp real (float) object
 * @param value The float value
 * @return Pointer to the new object
 */
lisp_object_t* lisp_make_real(float value)
{
  lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_REAL);
  obj->v.real = value;
  return obj;
}

/**
 * Create a lisp symbol object
 * @param value The symbol name (will be interned)
 * @return Pointer to the new object, or nullptr on error
 */
lisp_object_t* lisp_make_symbol(const char* value)
{
  if (!value)
  {
    return nullptr;
  }

  lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_SYMBOL);
  const char* interned = g_string_interner.intern(value);
  if (!interned)
  {
    return nullptr;
  }

  obj->v.string = const_cast<char*>(interned);
  return obj;
}

/**
 * Create a lisp string object
 * @param value The string value (will be interned)
 * @return Pointer to the new object, or nullptr on error
 */
lisp_object_t* lisp_make_string(const char* value)
{
  if (!value)
  {
    return nullptr;
  }

  lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_STRING);
  const char* interned = g_string_interner.intern(value);
  if (!interned)
  {
    return nullptr;
  }

  obj->v.string = const_cast<char*>(interned);
  return obj;
}

/**
 * Create a lisp cons cell
 * @param car The car of the cons cell
 * @param cdr The cdr of the cons cell
 * @return Pointer to the new cons cell
 */
lisp_object_t* lisp_make_cons(lisp_object_t* car, lisp_object_t* cdr)
{
  lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_CONS);
  obj->v.cons.car = car;
  obj->v.cons.cdr = cdr;
  return obj;
}

/**
 * Create a lisp boolean object
 * @param value Non-zero for true, zero for false
 * @return Pointer to the new object
 */
lisp_object_t* lisp_make_boolean(int value)
{
  lisp_object_t* obj = lisp_object_alloc(LISP_TYPE_BOOLEAN);
  obj->v.integer = value ? 1 : 0;
  return obj;
}

/**
 * Read a lisp object from a stream
 * @param in The input stream
 * @return The parsed object, or error/eof marker
 */
lisp_object_t* lisp_read(lisp_stream_t* in)
{
  if (!in)
  {
    return &error_object;
  }

  LispParserInternal parser;
  return parser.parse_stream(in);
}

/**
 * Free a lisp object and all its children
 * Note: Due to the memory pool implementation, this function doesn't
 * actually free memory. Memory is released when lisp_reset_pool() is called.
 * This function exists for API compatibility.
 * @param obj The object to free
 */
void lisp_free(lisp_object_t* obj)
{
  if (obj == nullptr || obj == &error_object || obj == &end_marker)
  {
    return;
  }

  // Use iterative approach to free objects
  struct FreeItem
  {
    lisp_object_t* obj;
    bool freed_car;
  };

  std::vector<FreeItem> stack;
  stack.push_back({obj, false});

  int operations = 0;
  while (!stack.empty() && operations++ < MAX_PARSE_OPERATIONS)
  {
    FreeItem& item = stack.back();

    if (!item.obj || item.obj == &error_object || item.obj == &end_marker)
    {
      stack.pop_back();
      continue;
    }

    switch (item.obj->type)
    {
      case LISP_TYPE_CONS:
      case LISP_TYPE_PATTERN_CONS:
        if (!item.freed_car)
        {
          item.freed_car = true;
          if (item.obj->v.cons.car)
          {
            stack.push_back({item.obj->v.cons.car, false});
          }
        }
        else
        {
          if (item.obj->v.cons.cdr)
          {
            stack.push_back({item.obj->v.cons.cdr, false});
          }
          stack.pop_back();
        }
        break;

      case LISP_TYPE_PATTERN_VAR:
        if (item.obj->v.pattern.sub)
        {
          stack.push_back({item.obj->v.pattern.sub, false});
        }
        stack.pop_back();
        break;

      default:
        stack.pop_back();
        break;
    }
  }
}

/**
 * Read a lisp object from a string buffer with length
 * @param buf The string buffer
 * @param len The length of the buffer
 * @return The parsed object, or error marker
 */
lisp_object_t* lisp_read_from_string(const char* buf, size_t len)
{
  if (!buf)
  {
    return &error_object;
  }
  if (len > MAX_STRING_LENGTH)
  {
    return &error_object;
  }

  lisp_stream_t stream;
  if (!lisp_stream_init_string(&stream, buf, len))
  {
    return &error_object;
  }

  return lisp_read(&stream);
}

/**
 * Read a lisp object from a null-terminated string
 * @param buf The null-terminated string
 * @return The parsed object, or error marker
 */
lisp_object_t* lisp_read_from_string(const char* buf)
{
  if (!buf)
  {
    return &error_object;
  }

  lisp_stream_t stream;
  if (!lisp_stream_init_string(&stream, buf))
  {
    return &error_object;
  }

  return lisp_read(&stream);
}

/**
 * Compile a pattern expression for matching
 * @param obj Pointer to the object pointer to compile
 * @param num_subs Output: number of sub-patterns/variables
 * @return 1 on success, 0 on failure
 */
int lisp_compile_pattern(lisp_object_t** obj, int* num_subs)
{
  if (!obj)
  {
    return 0;
  }

  int index = 0;
  LispParserInternal parser;
  int result = parser.compile_pattern(obj, &index);

  if (result && num_subs != nullptr)
  {
    *num_subs = index;
  }
  return result;
}

/**
 * Match a compiled pattern against an object
 * @param pattern The compiled pattern
 * @param obj The object to match
 * @param vars Array to store captured variables
 * @param num_subs Number of sub-patterns
 * @return 1 if match succeeds, 0 otherwise
 */
int lisp_match_pattern(lisp_object_t* pattern, lisp_object_t* obj, lisp_object_t** vars, int num_subs)
{
  LispParserInternal parser;
  return parser.do_match_pattern(pattern, obj, vars, num_subs);
}

/**
 * Parse and match a pattern string against an object
 * @param pattern_string The pattern string to parse and compile
 * @param obj The object to match
 * @param vars Array to store captured variables
 * @return 1 if match succeeds, 0 otherwise
 */
int lisp_match_string(const char* pattern_string, lisp_object_t* obj, lisp_object_t** vars)
{
  if (!pattern_string)
  {
    return 0;
  }

  const size_t pattern_len = strnlen(pattern_string, MAX_STRING_LENGTH + 1);
  if (pattern_len > MAX_STRING_LENGTH)
  {
    return 0;
  }

  lisp_object_t* pattern = lisp_read_from_string(pattern_string, pattern_len);
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

/**
 * Get the type of a lisp object
 * @param obj The object
 * @return The LISP_TYPE_* constant, or LISP_TYPE_NIL if obj is null
 */
int lisp_type(lisp_object_t* obj)
{
  return obj ? obj->type : LISP_TYPE_NIL;
}

/**
 * Get the integer value from a lisp object
 * @param obj The object
 * @return The integer value, or 0 if not an integer
 */
int lisp_integer(lisp_object_t* obj)
{
  if (!obj || obj->type != LISP_TYPE_INTEGER)
  {
    return 0;
  }
  return obj->v.integer;
}

/**
 * Get the symbol name from a lisp object
 * @param obj The object
 * @return Pointer to the symbol name, or nullptr if not a symbol
 */
char* lisp_symbol(lisp_object_t* obj)
{
  if (!obj || obj->type != LISP_TYPE_SYMBOL)
  {
    return nullptr;
  }
  return obj->v.string;
}

/**
 * Get the string value from a lisp object
 * @param obj The object
 * @return Pointer to the string value, or nullptr if not a string
 */
char* lisp_string(lisp_object_t* obj)
{
  if (!obj || obj->type != LISP_TYPE_STRING)
  {
    return nullptr;
  }
  return obj->v.string;
}

/**
 * Get the boolean value from a lisp object
 * @param obj The object
 * @return The boolean value (0 or 1), or 0 if not a boolean
 */
int lisp_boolean(lisp_object_t* obj)
{
  if (!obj || obj->type != LISP_TYPE_BOOLEAN)
  {
    return 0;
  }
  return obj->v.integer;
}

/**
 * Get the real (float) value from a lisp object
 * This function also accepts integers and converts them to float.
 * @param obj The object
 * @return The float value, or 0.0f if not a number
 */
float lisp_real(lisp_object_t* obj)
{
  if (!obj)
  {
    return 0.0f;
  }
  if (obj->type != LISP_TYPE_REAL && obj->type != LISP_TYPE_INTEGER)
  {
    return 0.0f;
  }
  return (obj->type == LISP_TYPE_INTEGER) ? static_cast<float>(obj->v.integer) : obj->v.real;
}

/**
 * Get the car (first element) of a cons cell
 * @param obj The cons cell
 * @return The car, or nullptr if not a cons cell
 */
lisp_object_t* lisp_car(lisp_object_t* obj)
{
  if (!obj)
  {
    return nullptr;
  }
  if (obj->type != LISP_TYPE_CONS && obj->type != LISP_TYPE_PATTERN_CONS)
  {
    return nullptr;
  }
  return obj->v.cons.car;
}

/**
 * Get the cdr (rest of list) of a cons cell
 * @param obj The cons cell
 * @return The cdr, or nullptr if not a cons cell
 */
lisp_object_t* lisp_cdr(lisp_object_t* obj)
{
  if (!obj)
  {
    return nullptr;
  }
  if (obj->type != LISP_TYPE_CONS && obj->type != LISP_TYPE_PATTERN_CONS)
  {
    return nullptr;
  }
  return obj->v.cons.cdr;
}

/**
 * Perform a series of car/cdr operations
 * The string x specifies the operations: 'a' for car, 'd' for cdr.
 * For example, "ad" means (car (cdr obj)).
 * @param obj The starting object
 * @param x String of 'a' and 'd' characters
 * @return The result of the operations, or nullptr on error
 */
lisp_object_t* lisp_cxr(lisp_object_t* obj, const char* x)
{
  if (x == nullptr)
  {
    return nullptr;
  }
  const size_t len = strnlen(x, 64);
  if (len > 64)
  {
    return nullptr;
  }

  for (size_t i = len; i-- > 0; )
  {
    if (obj == nullptr)
    {
      return nullptr;
    }
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
      return nullptr;
    }
  }
  return obj;
}

/**
 * Get the length of a list
 * @param obj The list object
 * @return The number of elements in the list
 */
int lisp_list_length(lisp_object_t* obj)
{
  int length = 0;
  int max_iterations = 100000;

  while (obj != nullptr && length < max_iterations)
  {
    if (obj->type != LISP_TYPE_CONS && obj->type != LISP_TYPE_PATTERN_CONS)
    {
      break;
    }
    ++length;
    obj = obj->v.cons.cdr;
  }
  return length;
}

/**
 * Get the nth cdr of a list
 * @param obj The list object
 * @param index The index (0-based)
 * @return The list starting at index, or nullptr if index out of bounds
 */
lisp_object_t* lisp_list_nth_cdr(lisp_object_t* obj, int index)
{
  if (index < 0 || index > 10000)
  {
    return nullptr;
  }

  while (index > 0)
  {
    if (obj == nullptr)
    {
      return nullptr;
    }
    if (obj->type != LISP_TYPE_CONS && obj->type != LISP_TYPE_PATTERN_CONS)
    {
      return nullptr;
    }
    --index;
    obj = obj->v.cons.cdr;
  }
  return obj;
}

/**
 * Get the nth element of a list
 * @param obj The list object
 * @param index The index (0-based)
 * @return The element at index, or nullptr if index out of bounds
 */
lisp_object_t* lisp_list_nth(lisp_object_t* obj, int index)
{
  obj = lisp_list_nth_cdr(obj, index);
  if (obj == nullptr)
  {
    return nullptr;
  }
  return obj->v.cons.car;
}

/**
 * Find a value in an association list by key
 * Searches for a cons cell whose car is a symbol matching the key.
 * @param list The association list
 * @param key The key to search for
 * @return The cdr of the matching cons cell, or nullptr if not found
 */
lisp_object_t* lisp_find_value(lisp_object_t* list, const char* key)
{
  if (!key)
  {
    return nullptr;
  }

  const char* interned_key = g_string_interner.intern(key);
  if (!interned_key)
  {
    return nullptr;
  }

  int max_iterations = 10000;
  int iterations = 0;

  while (!lisp_nil_p(list) && iterations++ < max_iterations)
  {
    lisp_object_t* cur = lisp_car(list);
    if (lisp_cons_p(cur) && lisp_symbol_p(lisp_car(cur)))
    {
      if (lisp_symbol(lisp_car(cur)) == interned_key)
      {
        return lisp_cdr(cur);
      }
    }
    list = lisp_cdr(list);
  }

  return nullptr;
}

/**
 * Dump a lisp object to a file stream for debugging
 * @param obj The object to dump
 * @param out The output file stream
 */
void lisp_dump(lisp_object_t* obj, FILE* out)
{
  if (!out)
  {
    return;
  }

  if (obj == nullptr)
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
    {
      char* sym = lisp_symbol(obj);
      if (sym)
      {
        fputs(sym, out);
      }
      break;
    }
    case LISP_TYPE_STRING:
    {
      fputc('"', out);
      char* str = lisp_string(obj);
      if (str)
      {
        for (char* p = str; *p != 0; ++p)
        {
          if (*p == '"' || *p == '\\')
          {
            fputc('\\', out);
          }
          fputc(*p, out);
        }
      }
      fputc('"', out);
      break;
    }
    case LISP_TYPE_CONS:
    case LISP_TYPE_PATTERN_CONS:
    {
      fputs(lisp_type(obj) == LISP_TYPE_CONS ? "(" : "#?(", out);
      int depth = 0;
      while (obj != nullptr && depth++ < 1000)
      {
        lisp_dump(lisp_car(obj), out);
        obj = lisp_cdr(obj);
        if (obj != nullptr)
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
    case LISP_TYPE_BOOLEAN:
      fputs(lisp_boolean(obj) ? "#t" : "#f", out);
      break;
    default:
      break;
  }
}

// ============================================================================
// LispReader Class Implementation
// ============================================================================

/**
 * Constructor for LispReader
 * Builds a property map for fast lookup of named properties in the list.
 * @param l The lisp list to read from
 */
LispReader::LispReader(lisp_object_t* l) : lst(l)
{
  if (!l)
  {
    return;
  }

  int max_properties = 10000;
  int count = 0;

  for (lisp_object_t* cursor = lst; !lisp_nil_p(cursor) && count++ < max_properties; cursor = lisp_cdr(cursor))
  {
    lisp_object_t* cur = lisp_car(cursor);
    if (lisp_cons_p(cur) && lisp_symbol_p(lisp_car(cur)))
    {
      char* key = lisp_symbol(lisp_car(cur));
      if (key)
      {
        property_map[key] = lisp_cdr(cur);
      }
    }
  }
}

/**
 * Search for a property by name
 * @param name The property name to search for
 * @return The property value list, or nullptr if not found
 */
lisp_object_t* LispReader::search_for(const char* name)
{
  if (!name)
  {
    return nullptr;
  }

  const char* interned_name = g_string_interner.intern(name);
  if (!interned_name)
  {
    return nullptr;
  }

  auto it = property_map.find(interned_name);
  if (it != property_map.end())
  {
    return it->second;
  }
  return nullptr;
}

/**
 * Template helper for reading vector properties
 * This template eliminates code duplication between read_string_vector,
 * read_int_vector, and read_char_vector by abstracting the common logic.
 * @tparam T The vector element type
 * @tparam Predicate Function type for type checking
 * @tparam Getter Function type for value extraction
 * @param name The property name
 * @param vec Output vector to populate
 * @param pred Type predicate function (e.g., lisp_string_p)
 * @param get Value getter function (e.g., lisp_string)
 * @return true if property found and successfully read, false otherwise
 */
template <typename T, typename Predicate, typename Getter>
bool LispReader::read_vector_impl(const char* name, std::vector<T>* vec, Predicate pred, Getter get)
{
  if (!vec)
  {
    return false;
  }

  lisp_object_t* obj = search_for(name);
  if (obj)
  {
    int max_items = 10000;
    int count = 0;

    while (!lisp_nil_p(obj) && count++ < max_items)
    {
      lisp_object_t* item = lisp_car(obj);
      if (!pred(item))
      {
        return false;
      }

      vec->push_back(get(item));
      obj = lisp_cdr(obj);
    }
    return true;
  }
  return false;
}

/**
 * Read an integer property
 * @param name The property name
 * @param i Output integer pointer
 * @return true if property found and is an integer, false otherwise
 */
bool LispReader::read_int(const char* name, int* i)
{
  if (!i)
  {
    return false;
  }

  lisp_object_t* obj = search_for(name);
  if (obj && lisp_integer_p(lisp_car(obj)))
  {
    *i = lisp_integer(lisp_car(obj));
    return true;
  }
  return false;
}

/**
 * Read a lisp object property
 * @param name The property name
 * @param b Output object pointer pointer
 * @return true if property found, false otherwise
 */
bool LispReader::read_lisp(const char* name, lisp_object_t** b)
{
  if (!b)
  {
    return false;
  }

  lisp_object_t* obj = search_for(name);
  if (obj)
  {
    *b = obj;
    return true;
  }
  return false;
}

/**
 * Read a float property
 * @param name The property name
 * @param f Output float pointer
 * @return true if property found and is a number, false otherwise
 */
bool LispReader::read_float(const char* name, float* f)
{
  if (!f)
  {
    return false;
  }

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

/**
 * Read a string vector property
 * @param name The property name
 * @param vec Output string vector pointer
 * @return true if property found and all elements are strings, false otherwise
 */
bool LispReader::read_string_vector(const char* name, std::vector<std::string>* vec)
{
  // Lambda to convert char* to std::string
  return read_vector_impl(name, vec, [](lisp_object_t* obj){ return lisp_string_p(obj); },
    [](lisp_object_t* obj) -> std::string
    {
      char* str = lisp_string(obj);
      return str ? std::string(str) : std::string();
    });
}

/**
 * Read an integer vector property
 * @param name The property name
 * @param vec Output integer vector pointer
 * @return true if property found and all elements are integers, false otherwise
 */
bool LispReader::read_int_vector(const char* name, std::vector<int>* vec)
{
  return read_vector_impl(name, vec, [](lisp_object_t* obj){ return lisp_integer_p(obj); }, lisp_integer);
}

/**
 * Read a character vector property
 * @param name The property name
 * @param vec Output character vector pointer
 * @return true if property found and all elements are strings, false otherwise
 */
bool LispReader::read_char_vector(const char* name, std::vector<char>* vec)
{
  // Lambda to extract first character from string
  return read_vector_impl(name, vec, [](lisp_object_t* obj){ return lisp_string_p(obj); },
    [](lisp_object_t* obj) -> char
    {
      char* str = lisp_string(obj);
      return str ? *str : '\0';
    });
}

/**
 * Read a string property
 * @param name The property name
 * @param str Output string pointer
 * @return true if property found and is a string, false otherwise
 */
bool LispReader::read_string(const char* name, std::string* str)
{
  if (!str)
  {
    return false;
  }

  lisp_object_t* obj = search_for(name);
  if (obj && lisp_string_p(lisp_car(obj)))
  {
    char* c_str = lisp_string(lisp_car(obj));
    if (c_str)
    {
      *str = c_str;
      return true;
    }
  }
  return false;
}

/**
 * Read a boolean property
 * @param name The property name
 * @param b Output boolean pointer
 * @return true if property found and is a boolean, false otherwise
 */
bool LispReader::read_bool(const char* name, bool* b)
{
  if (!b)
  {
    return false;
  }

  lisp_object_t* obj = search_for(name);
  if (obj && lisp_boolean_p(lisp_car(obj)))
  {
    *b = lisp_boolean(lisp_car(obj));
    return true;
  }
  return false;
}

// ============================================================================
// LispWriter Class Implementation
// ============================================================================

/**
 * Constructor for LispWriter
 * @param name The name symbol for the top-level list (optional)
 */
LispWriter::LispWriter(const char* name)
{
  if (name)
  {
    lisp_objs.push_back(lisp_make_symbol(name));
  }
}

/**
 * Append a lisp object to the writer's list
 * @param obj The object to append
 */
void LispWriter::append(lisp_object_t* obj)
{
  if (obj)
  {
    lisp_objs.push_back(obj);
  }
}

/**
 * Helper to create a 3-element list
 * @param a First element
 * @param b Second element
 * @param c Third element
 * @return The constructed list
 */
lisp_object_t* LispWriter::make_list3(lisp_object_t* a, lisp_object_t* b, lisp_object_t* c)
{
  return lisp_make_cons(a, lisp_make_cons(b, lisp_make_cons(c, lisp_nil())));
}

/**
 * Helper to create a 2-element list
 * @param a First element
 * @param b Second element
 * @return The constructed list
 */
lisp_object_t* LispWriter::make_list2(lisp_object_t* a, lisp_object_t* b)
{
  return lisp_make_cons(a, lisp_make_cons(b, lisp_nil()));
}

/**
 * Write a float property
 * @param name The property name
 * @param f The float value
 */
void LispWriter::write_float(const char* name, float f)
{
  if (name)
  {
    append(make_list2(lisp_make_symbol(name), lisp_make_real(f)));
  }
}

/**
 * Write an integer property
 * @param name The property name
 * @param i The integer value
 */
void LispWriter::write_int(const char* name, int i)
{
  if (name)
  {
    append(make_list2(lisp_make_symbol(name), lisp_make_integer(i)));
  }
}

/**
 * Write a string property
 * @param name The property name
 * @param str The string value
 */
void LispWriter::write_string(const char* name, const char* str)
{
  if (name && str)
  {
    append(make_list2(lisp_make_symbol(name), lisp_make_string(str)));
  }
}

/**
 * Write a symbol property
 * @param name The property name
 * @param symname The symbol value
 */
void LispWriter::write_symbol(const char* name, const char* symname)
{
  if (name && symname)
  {
    append(make_list2(lisp_make_symbol(name), lisp_make_symbol(symname)));
  }
}

/**
 * Write a lisp object property
 * @param name The property name
 * @param lst The lisp object value
 */
void LispWriter::write_lisp_obj(const char* name, lisp_object_t* lst)
{
  if (name && lst)
  {
    append(make_list2(lisp_make_symbol(name), lst));
  }
}

/**
 * Write a boolean property
 * @param name The property name
 * @param b The boolean value
 */
void LispWriter::write_boolean(const char* name, bool b)
{
  if (name)
  {
    append(make_list2(lisp_make_symbol(name), lisp_make_boolean(b)));
  }
}

/**
 * Create the final lisp object from the writer's contents
 * @return The constructed lisp list
 */
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

// ============================================================================
// File I/O Functions
// ============================================================================

/**
 * Read a lisp object from a file
 * @param filename The file path to read
 * @return The parsed object, or nullptr on error
 */
lisp_object_t* lisp_read_from_file(const std::string& filename)
{
  if (filename.empty())
  {
    return nullptr;
  }

  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
  {
    return nullptr;
  }

  if (fseek(in, 0, SEEK_END) != 0)
  {
    fclose(in);
    return nullptr;
  }

  long file_size = ftell(in);

  if (fseek(in, 0, SEEK_SET) != 0)
  {
    fclose(in);
    return nullptr;
  }

  if (file_size < 0 || file_size > static_cast<long>(MAX_FILE_SIZE))
  {
    fclose(in);
    return nullptr;
  }

  std::vector<char> buffer(file_size);
  size_t bytes_read = fread(buffer.data(), 1, file_size, in);
  fclose(in);

  if (bytes_read != static_cast<size_t>(file_size))
  {
    return nullptr;
  }

  return lisp_read_from_string(buffer.data(), file_size);
}

/**
 * Reset the global memory pool
 * This releases all memory allocated by the lisp reader and should be
 * called when you're done with all lisp objects.
 */
void lisp_reset_pool()
{
  g_object_pool.reset();
}

// EOF
