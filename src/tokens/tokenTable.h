#ifndef TOKEN_TABLE_H
#define TOKEN_TABLE_H

#include "token.enum.h"
#include <cstddef>

struct TokenSpec {
  TokenType type;
  const char *pattern;
};

inline constexpr TokenSpec TOKEN_SPECS[] = {
  // punctuation
  {TokenType::LPAREN, R"(\()"},
  {TokenType::RPAREN, R"(\))"},
  {TokenType::LBRACE, R"(\{)"},
  {TokenType::RBRACE, R"(\})"},
  {TokenType::SEMI, R"(;)"},
  {TokenType::ASSIGN, R"(=)"},
  {TokenType::COLON, R"(:)"},
  {TokenType::DOLLAR_EOF, R"(\$)"},

  // keywords
  {TokenType::MOD, R"(\bmod\b)"},
  {TokenType::VOID, R"(\bvoid\b)"},
  {TokenType::RETURN, R"(\breturn\b)"},
  {TokenType::NUM_KEYWORD, R"(\bnum\b)"},
  {TokenType::NOP, R"(\bnop\b)"},
  {TokenType::COMMENT, R"(\bcomment\b)"},
  {TokenType::ADD, R"(\badd\b)"},
  {TokenType::SUB, R"(\bsub\b)"},
  {TokenType::MUL, R"(\bmul\b)"},
  {TokenType::DIV, R"(\bdiv\b)"},
  {TokenType::NEG, R"(\bneg\b)"},
  {TokenType::IF, R"(\bif\b)"},
  {TokenType::THEN, R"(\bthen\b)"},
  {TokenType::ELSE, R"(\belse\b)"},
  {TokenType::NOT, R"(\bnot\b)"},
  {TokenType::AND, R"(\band\b)"},
  {TokenType::OR, R"(\bor\b)"},
  {TokenType::EQ, R"(\beq\b)"},
  {TokenType::LARGER, R"(\blarger\b)"},
  {TokenType::LESSER, R"(\blesser\b)"},
  {TokenType::DO, R"(\bdo\b)"},
  {TokenType::WHILE, R"(\bwhile\b)"},
  {TokenType::UNTIL, R"(\buntil\b)"},
  {TokenType::PRINT, R"(\bprint\b)"},

  // the rest
  {TokenType::NAME, R"(#[0-9a-z]*)"},
  {TokenType::STRING, R"("[,.:\-?!0-9a-z]*")"},
  {TokenType::NUM,
    R"(0|-?0\.[0-9]*[1-9]|-?[1-9][0-9]*\.[0-9]*[1-9]|-?[1-9][0-9]*)"},
};

inline constexpr std::size_t TOKEN_SPEC_COUNT =
  sizeof(TOKEN_SPECS) / sizeof(TOKEN_SPECS[0]);

#endif
