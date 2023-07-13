#pragma once

#include <limits.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <memory>
#include <algorithm>
#include <exception>
#include <tokenize/defines/tokenizer_types.hpp>

namespace tokenize
{
struct token_exception : public std::exception
{
  std::string m_error_message;

  token_exception(const std::string& error_message)
    : m_error_message(error_message)
  {
  }

  token_exception(const std::string& file_path, size_t line_number, const std::string& error_message)
    : m_error_message(file_path.empty() ? error_message : file_path + "(" + std::to_string(line_number) + "): " + error_message)
  {
  }

  virtual const char* what() throw()
  {
    return m_error_message.c_str();
  }
};

struct stream_context
{
  std::string m_file_path;
  std::string m_stream;
  std::vector<token> m_tokens;
  size_t m_num_lines = 0;
};

struct dfa_state
{
  token_id m_token_id;
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

  dfa_state_ptr add_state(token_id accepting_token)
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
  void add_string(dfa_state_ptr from, dfa_state_ptr default_state, token_id id, const std::string& word, const std::string accepted)
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
          from->m_edge[character] = add_state(token_id::invalid);
        }

        add_range(from->m_edge[character], default_state, accepted);
      }

      from = from->m_edge[character];
    }

    from->m_token_id = id;
  }
};

  struct parsing_context
  {
    stream_context m_token_context;
    int m_current_token = 0;
    int m_previous_token = 0;

  bool end_of_token_stream()
  {
    return m_current_token >= m_token_context.m_tokens.size();
  }

  const token& get_current_token()
  {
    return m_token_context.m_tokens[m_current_token];
  }

  const token& get_previous_token()
  {
    return m_token_context.m_tokens[m_previous_token];
  }

  void set_current_token_index(int new_index = 0)
  {
    m_current_token = std::min(static_cast<int>(m_token_context.m_tokens.size()) - 1, new_index);
    m_previous_token = m_current_token;
  }

  void advance_token_stream(bool skip_whitespace_and_comments = true)
  {
    m_previous_token = m_current_token;
    ++m_current_token;

    if (skip_whitespace_and_comments && !end_of_token_stream())
    {
      while (m_token_context.m_tokens[m_current_token].m_id == token_id::new_line ||
        m_token_context.m_tokens[m_current_token].m_id == token_id::whitespace ||
        m_token_context.m_tokens[m_current_token].m_id == token_id::single_line_comment ||
        m_token_context.m_tokens[m_current_token].m_id == token_id::multi_line_comment)
      {
        ++m_current_token;

        if (end_of_token_stream())
        {
          break;
        }
      }
    }
  }

  bool accept(const std::string& identifier, bool skip_whitespace_and_comments = true)
  {
    expect(!end_of_token_stream(), "Unexpected end of stream.");

    if (std::string(m_token_context.m_tokens[m_current_token].m_stream, m_token_context.m_tokens[m_current_token].m_length) == identifier)
    {
      advance_token_stream(skip_whitespace_and_comments);
      return true;
    }

    return false;
  }

  bool accept(token_id id, bool skip_whitespace_and_comments = true)
  {
    expect(!end_of_token_stream(), "Unexpected end of stream.");

    if (m_token_context.m_tokens[m_current_token].m_id == id)
    {
      advance_token_stream(skip_whitespace_and_comments);
      return true;
    }

    return false;
  }

  bool expect(token_id id, const std::string& error_message, bool skip_whitespace_and_comments = true)
  {
    return expect(accept(id, skip_whitespace_and_comments), error_message);
  }

  bool expect(bool expression, const std::string& error_message)
  {
    if (expression)
    {
      return true;
    }

    // TODO: Exception Handler Class
    if (end_of_token_stream())
    {
      if (m_previous_token >= 0 && m_previous_token < m_token_context.m_tokens.size())
      {
        token& previous_token = m_token_context.m_tokens[m_previous_token];
        throw token_exception(previous_token.m_file_path, previous_token.m_line_number, error_message);
      }
      else
      {
        throw token_exception(error_message);
      }
    }
    else
    {
      token& error_token = m_token_context.m_tokens[m_previous_token];
      throw token_exception(error_token.m_file_path, error_token.m_line_number, error_message);
    }
  }

  void remove_identifier_tokens(const std::vector<std::string>& identifiers)
  {
    for (int i = 0; i < m_token_context.m_tokens.size();)
    {
      bool found = false;

      if (m_token_context.m_tokens[i].m_id == token_id::identifier)
      {
        std::string token_string = std::string(m_token_context.m_tokens[i].m_stream, m_token_context.m_tokens[i].m_length);

        for (int j = 0; j < identifiers.size(); ++j)
        {
          if (token_string == identifiers[j])
          {
            remove_tokens(i, i + 1);
            found = true;
            break;
          }
        }
      }

      if (!found)
      {
        ++i;
      }
    }
  }

  void remove_tokens(int start_index, int end_index)
  {
    if (start_index >= end_index || start_index < 0 || end_index >= m_token_context.m_tokens.size())
    {
      return;
    }

    m_token_context.m_tokens.erase(m_token_context.m_tokens.begin() + start_index + 1, m_token_context.m_tokens.begin() + end_index);
  }

  void remove_tokens(token_id id)
  {
    std::vector<token> new_list;
    new_list.reserve(m_token_context.m_tokens.size());
    for (size_t i = 0; i < m_token_context.m_tokens.size(); ++i)
    {
      if (m_token_context.m_tokens[i].m_id != id)
      {
        new_list.push_back(m_token_context.m_tokens[i]);
      }
    }

    m_token_context.m_tokens = new_list;
  }

};

struct dfa_cpp : public dfa_base
{
  dfa_cpp()
  {
    root = add_state(token_id::invalid);
    dfa_state_ptr white_space = add_state(token_id::whitespace);
    dfa_state_ptr new_line = add_state(token_id::new_line);
    dfa_state_ptr identifier = add_state(token_id::identifier);
    dfa_state_ptr integer_literal = add_state(token_id::integer_literal);
    dfa_state_ptr float_literal = add_state(token_id::float_literal);
    dfa_state_ptr scientific_inv = add_state(token_id::invalid);
    dfa_state_ptr plus_minus_inv = add_state(token_id::invalid);
    dfa_state_ptr scientific_float = add_state(token_id::float_literal);
    dfa_state_ptr optional_f = add_state(token_id::float_literal);
    dfa_state_ptr string_back_slash = add_state(token_id::invalid);
    dfa_state_ptr string_literal_inv = add_state(token_id::invalid);
    dfa_state_ptr string_literal = add_state(token_id::string_literal);

    dfa_state_ptr integer_literal_zero = add_state(token_id::integer_literal);
    dfa_state_ptr hex_literal_inv = add_state(token_id::invalid);
    dfa_state_ptr binary_literal_inv = add_state(token_id::invalid);

    dfa_state_ptr hex_literal = add_state(token_id::hex_literal);
    dfa_state_ptr binary_literal = add_state(token_id::binary_literal);

    dfa_state_ptr character_literal_inv = add_state(token_id::invalid);
    dfa_state_ptr character_backslash = add_state(token_id::invalid);
    dfa_state_ptr character_finish = add_state(token_id::invalid);
    dfa_state_ptr character_literal = add_state(token_id::character_literal);
    dfa_state_ptr single_line_comment_first_slash = add_state(token_id::invalid);
    dfa_state_ptr single_line_comment = add_state(token_id::single_line_comment);
    dfa_state_ptr multi_line_comment_inv = add_state(token_id::invalid);
    dfa_state_ptr multi_line_comment_escape = add_state(token_id::invalid);
    dfa_state_ptr multi_line_comment = add_state(token_id::multi_line_comment);

    std::string lower_case = "abcdefghijklmnopqrstuvwxyz";
    std::string upper_case = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string letters = lower_case + upper_case;
    std::string numbers = "0123456789";
    std::string hex_characters = numbers + "abcdef" + "ABCDEF";
    std::string identifier_characters = letters + numbers + '_';

#define TOKEN(text, name) add_string(root, nullptr, token_id::name, text, "");
    // Add Symbols
#include "defines/symbol.inl"
#undef TOKEN

#define TOKEN(text, name) add_string(root, identifier, token_id::name, text, identifier_characters);
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

namespace internal
{
void read_token(const char* stream, const dfa_base& dfa, token& out_token)
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

void read_language_token(const char* stream, const dfa_base& dfa, stream_context& out_token_stream, token& out_token)
{
  read_token(stream, dfa, out_token);

  if (out_token.m_id == token_id::float_literal && out_token.m_stream[out_token.m_length - 1] == '.')
  {
    out_token.m_id = token_id::integer_literal;
    out_token.m_length -= 1;
  }

  out_token.m_line_number = out_token_stream.m_num_lines;

  if (out_token.m_id == token_id::new_line)
  {
    ++out_token_stream.m_num_lines;
  }
  else if (out_token.m_id == token_id::multi_line_comment)
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

void tokenize_stream(const dfa_base& dfa, stream_context& out_token_stream)
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

}
 
void from_string(const std::string& string, const dfa_base& dfa, stream_context& out_token_stream)
{
  out_token_stream.m_num_lines = 1;
  out_token_stream.m_stream = string;
  out_token_stream.m_tokens.clear();
  internal::tokenize_stream(dfa, out_token_stream);
}

void from_file(const std::filesystem::path& file_path, const dfa_base& dfa, stream_context& out_token_stream)
{
  out_token_stream.m_num_lines = 1;
  std::ifstream file;
  file.open(file_path, std::ifstream::in);
  std::stringstream buffer;
  buffer << file.rdbuf();
  out_token_stream.m_stream = buffer.str();
  out_token_stream.m_file_path = file_path.string();
  out_token_stream.m_tokens.clear();
  internal::tokenize_stream(dfa, out_token_stream);
}
}