#pragma once

#include <limits.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <memory>
#include <tokenize/defines/tokenizer_types.hpp>

struct token_stream_context
{
  std::string m_file_path;
  std::string m_stream;
  std::vector<token> m_tokens;
  size_t m_num_lines = 0;
};

struct dfa_state
{
  e_token_id m_token_id;
  dfa_state* m_edge[CHAR_MAX];
};

typedef dfa_state* dfa_state_ptr;

struct dfa_base
{
  dfa_state_ptr root;
  std::vector<dfa_state_ptr> states;

  virtual ~dfa_base()
  {
    for (auto state : states)
    {
      delete state;
    }
  }

  dfa_state_ptr add_state(e_token_id accepting_token)
  {
    dfa_state_ptr state = new dfa_state();
    state->m_token_id = accepting_token;
    states.push_back(state);
    return state;
  }

  void add_edge(dfa_state_ptr from, dfa_state_ptr to, char c)
  {
    from->m_edge[c] = to;
  }

  void add_range(dfa_state_ptr from, dfa_state_ptr to, char start, char end, bool ignore = false)
  {
    for (; start < end + 1; ++start)
    {
      if (ignore || !from->m_edge[start])
      {
        add_edge(from, to, start);
      }
    }
  }

  /*! Adds a range of edges between two different DFA nodes as given by strings of characters. */
  void add_range(dfa_state_ptr from, dfa_state_ptr to, const std::string& characters, bool ignore = false)
  {
    for (char character : characters)
    {
      if (ignore || !from->m_edge[character])
      {
        add_edge(from, to, character);
      }
    }
  }

  /*! Adds a string keyword to the DFA. */
  void add_string(dfa_state_ptr from, dfa_state_ptr default_state, e_token_id id, const std::string& word, const std::string accepted)
  {
    for (char character : word)
    {
      dfa_state_ptr state_to_modify = from->m_edge[character];

      if (!state_to_modify || state_to_modify == default_state)
      {
        if (default_state)
        {
          from->m_edge[character] = add_state(default_state->m_token_id);
        }
        else
        {
          from->m_edge[character] = add_state(e_token_id::invalid);
        }

        add_range(from->m_edge[character], default_state, accepted);
      }

      from = from->m_edge[character];
    }

    from->m_token_id = id;
  }
};

struct dfa_cpp : public dfa_base
{
  dfa_cpp()
  {
    root = add_state(e_token_id::invalid);
    dfa_state_ptr white_space = add_state(e_token_id::whitespace);
    dfa_state_ptr new_line = add_state(e_token_id::new_line);
    dfa_state_ptr identifier = add_state(e_token_id::identifier);
    dfa_state_ptr integer_literal = add_state(e_token_id::integer_literal);
    dfa_state_ptr float_literal = add_state(e_token_id::float_literal);
    dfa_state_ptr scientific_inv = add_state(e_token_id::invalid);
    dfa_state_ptr plus_minus_inv = add_state(e_token_id::invalid);
    dfa_state_ptr scientific_float = add_state(e_token_id::float_literal);
    dfa_state_ptr optional_f = add_state(e_token_id::float_literal);
    dfa_state_ptr string_back_slash = add_state(e_token_id::invalid);
    dfa_state_ptr string_literal_inv = add_state(e_token_id::invalid);
    dfa_state_ptr string_literal = add_state(e_token_id::string_literal);

    dfa_state_ptr integer_literal_zero = add_state(e_token_id::integer_literal);
    dfa_state_ptr hex_literal_inv = add_state(e_token_id::invalid);
    dfa_state_ptr binary_literal_inv = add_state(e_token_id::invalid);

    dfa_state_ptr hex_literal = add_state(e_token_id::hex_literal);
    dfa_state_ptr binary_literal = add_state(e_token_id::binary_literal);

    dfa_state_ptr character_literal_inv = add_state(e_token_id::invalid);
    dfa_state_ptr character_backslash = add_state(e_token_id::invalid);
    dfa_state_ptr character_finish = add_state(e_token_id::invalid);
    dfa_state_ptr character_literal = add_state(e_token_id::character_literal);
    dfa_state_ptr single_line_comment_first_slash = add_state(e_token_id::invalid);
    dfa_state_ptr single_line_comment = add_state(e_token_id::single_line_comment);
    dfa_state_ptr multi_line_comment_inv = add_state(e_token_id::invalid);
    dfa_state_ptr multi_line_comment_escape = add_state(e_token_id::invalid);
    dfa_state_ptr multi_line_comment = add_state(e_token_id::multi_line_comment);

    std::string lower_case = "abcdefghijklmnopqrstuvwxyz";
    std::string upper_case = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string letters = lower_case + upper_case;
    std::string numbers = "0123456789";
    std::string hex_characters = numbers + "abcdef" + "ABCDEF";
    std::string identifier_characters = letters + numbers + '_';

#define TOKEN(text, name) add_string(root, nullptr, e_token_id::name, text, "");
    // Add Symbols
#include "defines/symbol.inl"
#undef TOKEN

#define TOKEN(text, name) add_string(root, identifier, e_token_id::name, text, identifier_characters);
#include "defines/keywords.inl"
#include "defines/preprocessor_directives.inl"
#undef TOKEN

    // WhiteSpace
    add_edge(root, white_space, ' ');
    add_edge(root, white_space, '\r');
    add_edge(root, white_space, '\t');
    add_edge(white_space, white_space, ' ');
    add_edge(white_space, white_space, '\r');
    add_edge(white_space, white_space, '\t');

    // Newline (For multi line macros)
    add_edge(root, new_line, '\n');

    // Identifiers
    add_range(root, identifier, letters + '_');
    add_range(identifier, identifier, identifier_characters);

    // Integer Literals

    add_range(root, integer_literal, "123456789");
    add_range(integer_literal, integer_literal, numbers);

    add_range(root, integer_literal_zero, "0");
    add_range(integer_literal_zero, integer_literal, numbers);

    add_range(integer_literal_zero, hex_literal_inv, "x");
    add_range(hex_literal_inv, hex_literal, hex_characters);
    add_range(hex_literal, hex_literal, hex_characters);

    add_range(integer_literal_zero, binary_literal_inv, "b");
    add_range(binary_literal_inv, binary_literal, "01");
    add_range(binary_literal, binary_literal, "01");

    // Float Literals
    add_edge(integer_literal, float_literal, '.');
    add_edge(integer_literal_zero, float_literal, '.');

    add_range(float_literal, float_literal, numbers);
    add_edge(float_literal, optional_f, 'f');
    add_edge(float_literal, scientific_inv, 'e');

    add_range(scientific_inv, scientific_float, numbers);
    add_edge(scientific_inv, plus_minus_inv, '+');
    add_edge(scientific_inv, plus_minus_inv, '-');

    add_range(plus_minus_inv, scientific_float, numbers);
    add_range(scientific_float, scientific_float, numbers);
    add_edge(scientific_float, optional_f, 'f');

    //String Literals
    add_edge(root, string_literal_inv, '"');
    add_range(string_literal_inv, string_literal_inv, 0, 126);
    add_edge(string_literal_inv, string_literal, '"');

    add_edge(string_literal_inv, string_back_slash, '\\');
    add_range(string_back_slash, string_literal_inv, 0, 126);

    //Character literals
    add_edge(root, character_literal_inv, '\'');

    add_range(character_literal_inv, character_finish, 0, 126);
    add_edge(character_literal_inv, character_backslash, '\\');

    add_range(character_backslash, character_finish, "nrt'");

    add_edge(character_finish, character_literal, '\'');

    // single line comments
    add_edge(root->m_edge['/'], single_line_comment, '/');

    add_range(single_line_comment, single_line_comment, 0, 126);
    add_range(single_line_comment, nullptr, "\n\0\r", true);

    // multi line comments
    add_edge(root->m_edge['/'], multi_line_comment_inv, '*');
    add_range(multi_line_comment_inv, multi_line_comment_inv, 0, 126);

    add_edge(multi_line_comment_inv, multi_line_comment_escape, '*');
    add_range(multi_line_comment_escape, multi_line_comment_inv, 0, 126);
    add_edge(multi_line_comment_escape, multi_line_comment_escape, '*');
    add_edge(multi_line_comment_escape, multi_line_comment, '/');
  }
};

class tokenize
{
public:
 

  static void string(const std::string& string, const dfa_base& dfa, token_stream_context& out_token_stream)
  {
    out_token_stream.m_num_lines = 1;

    out_token_stream.m_stream = string;
    out_token_stream.m_tokens.clear();

    tokenize_stream(dfa, out_token_stream);
  }

  static void file(const std::filesystem::path& file_path, const dfa_base& dfa, token_stream_context& out_token_stream)
  {
    out_token_stream.m_num_lines = 1;

    std::ifstream file;
    file.open(file_path, std::ifstream::in);
    std::stringstream buffer;
    buffer << file.rdbuf();
    out_token_stream.m_stream = buffer.str();
    out_token_stream.m_file_path = file_path.string();
    out_token_stream.m_tokens.clear();

    tokenize_stream(dfa, out_token_stream);
  }

private:

  static void tokenize_stream(const dfa_base& dfa, token_stream_context& out_token_stream)
  {
    const char* stream = out_token_stream.m_stream.c_str();

    while (*stream != '\0')
    {
      token language_token;
      read_language_token(stream, dfa, out_token_stream, language_token);
      language_token.m_file_path = out_token_stream.m_file_path.c_str();

      stream += language_token.m_length;

      if (language_token.m_length == 0)
      {
        ++stream;
      }
      else
      {
        out_token_stream.m_tokens.push_back(language_token);
      }
    }
  }

  static void read_language_token(const char* stream, const dfa_base& dfa, token_stream_context& out_token_stream, token& out_token)
  {
    read_token(stream, dfa, out_token);

    if (out_token.m_id == e_token_id::float_literal && out_token.m_stream[out_token.m_length - 1] == '.')
    {
      out_token.m_id = e_token_id::integer_literal;
      out_token.m_length -= 1;
    }

    out_token.m_line_number = out_token_stream.m_num_lines;

    if (out_token.m_id == e_token_id::new_line)
    {
      ++out_token_stream.m_num_lines;
    }
    else if (out_token.m_id == e_token_id::multi_line_comment)
    {
      std::string comment(out_token.m_stream, out_token.m_length);
      
      for (char character : comment)
      {
        if (character == '\0')
        {
          ++out_token_stream.m_num_lines;
        }
      }
    }
  }

  static void read_token(const char* stream, const dfa_base& dfa, token& out_token)
  {
    int i = 0;
    dfa_state_ptr state = dfa.root;

    while (state)
    {
      dfa_state_ptr next = state->m_edge[stream[i]];
      int length = i;

      if (!next || stream[i++] == '\0')
      {
        out_token.m_id = state->m_token_id;
        out_token.m_stream = stream;
        out_token.m_length = length;
        return;
      }

      state = next;
    }
  }
};