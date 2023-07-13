#pragma once

#include <string>
#include <unordered_map>

namespace tokenize
{

#undef TOKEN
#define TOKEN(text, name) text,

static std::string token_text[] = 
{
#include <tokenize/defines/tokens.inl>
};

#undef TOKEN
#define TOKEN(text, name) name,

enum class token_id
{
#include <tokenize/defines/tokens.inl>
};

#undef TOKEN
#define TOKEN(text, name) std::pair<std::string, token_id>( text, token_id::name),

static std::unordered_map<std::string, token_id> keyword_map =
{
#include <tokenize/defines/tokens.inl>
};

#undef TOKEN

struct token
{
  token_id m_id;
  const char* m_stream;
  size_t m_length;

  size_t m_line_number;
  const char* m_file_path;

  const char* comment_stream;
  size_t comment_length;
};

}