#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include <tokenize/tokenize.hpp>
#include <string>
#include <filesystem>

TEST_CASE("Empty code string.")
{
    std::string code = "";
    dfa_cpp dfa;
    token_stream_context context;
    tokenize::string(code, dfa, context);
    REQUIRE(context.m_file_path == "");
    REQUIRE(context.m_stream == "");
    REQUIRE(context.m_tokens.size() == 0);
    REQUIRE(context.m_num_lines == 1);
}

TEST_CASE("Comments")
{
    std::string code = 
    "// Single Line Comment.\n/*\n\tmulti line comment\n*/";
    dfa_cpp dfa;
    token_stream_context context;
    tokenize::string(code, dfa, context);
    REQUIRE(context.m_file_path == "");
    REQUIRE(context.m_stream == code);
    REQUIRE(context.m_tokens.size() == 3);
    REQUIRE(context.m_num_lines == 2);
}

TEST_CASE("Tokenize non-existant-file")
{
    std::filesystem::path path = "this-file-does-not-exist.cpp";
    dfa_cpp dfa;
    token_stream_context context;
    tokenize::file(path, dfa, context);
    REQUIRE(context.m_file_path == path.string());
    REQUIRE(context.m_num_lines == 1);
    REQUIRE(context.m_stream == "");
    REQUIRE(context.m_tokens.size() == 0);
}