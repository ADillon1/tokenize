TOKEN("invalid", invalid)

TOKEN("identifier", identifier)
TOKEN("integer_literal", integer_literal)
TOKEN("float_literal", float_literal)
TOKEN("string_literal", string_literal)
TOKEN("character_literal", character_literal)
TOKEN("binary_literal", binary_literal)
TOKEN("hex_literal", hex_literal)
TOKEN("whitespace", whitespace)
TOKEN("new_line", new_line)
TOKEN("single_line_comment", single_line_comment)
TOKEN("multi_line_comment", multi_line_comment)


// Every value below this value aligns with the list of symbol names /values 
// provided in TokenSymbols.inl (remember the +1)
TOKEN("symbol_start", symbol_start)
#include <tokenize/defines/symbol.inl>

// Every value below this value aligns with the list of keyword names / values
// provided in TokenKeywords.inl (remember the +1)
TOKEN("keyword_start", keyword_start)
#include <tokenize/defines/keywords.inl>

// Every value below this value aligns with the list of preprocessor directive names / values
// provided in PreprocessorDirectives.inl (remember the +1)
TOKEN("preprocessor_start", preprocessor_start)
#include <tokenize/defines/preprocessor_directives.inl>