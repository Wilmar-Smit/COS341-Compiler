#ifndef TOKEN_TABLE_H
#define TOKEN_TABLE_H

#include "token.enum.h"
#include <array>
#include <cstddef>

// Number of TokenType enumerators. Assumes they are contiguous (0..N-1) and
// that NUM is the last one. you MUST keep NUM LAST if you add types.
inline constexpr std::size_t TOKEN_TYPE_COUNT =
  static_cast<std::size_t>(TokenType::NUM) + 1;


// array w/ regex for every tokentype. It is indexed directly by enum value 
// (chose this approach for O(1) lookup time)
// the only real constraint is that we must ensure this 'lines up' exactly with the enums in 
// token.enum.h and it must always end with NUM

// Example of use: patternFor(TokenType::MOD);
inline constexpr std::array<const char *, TOKEN_TYPE_COUNT> PATTERN_BY_TYPE = {
  // punctuation
  R"(\()",  // LPAREN
  R"(\))",  // RPAREN
  R"(\{)",  // LBRACE
  R"(\})",  // RBRACE
  R"(;)",   // SEMI
  R"(=)",   // ASSIGN
  R"(:)",   // COLON
  R"(\$)",  // DOLLAR_EOF
  R"( )",// space 

  // keywords
  R"(\bmod\b)",     // MOD
  R"(\bvoid\b)",    // VOID
  R"(\breturn\b)",  // RETURN
  R"(\bnum\b)",     // NUM_KEYWORD
  R"(\bnop\b)",     // NOP
  R"(\bcomment\b)", // COMMENT
  R"(\badd\b)",     // ADD
  R"(\bsub\b)",     // SUB
  R"(\bmul\b)",     // MUL
  R"(\bdiv\b)",     // DIV
  R"(\bneg\b)",     // NEG
  R"(\bif\b)",      // IF
  R"(\bthen\b)",    // THEN
  R"(\belse\b)",    // ELSE
  R"(\bnot\b)",     // NOT
  R"(\band\b)",     // AND
  R"(\bor\b)",      // OR
  R"(\beq\b)",      // EQ
  R"(\blarger\b)",  // LARGER
  R"(\blesser\b)",  // LESSER
  R"(\bdo\b)",      // DO
  R"(\bwhile\b)",   // WHILE
  R"(\buntil\b)",   // UNTIL
  R"(\bprint\b)",   // PRINT

  // the rest
  R"(#[0-9a-z]*)",                                                 // NAME
  R"("[,.:\-?!0-9a-z]*")",                                         // STRING
  R"(0|-?0\.[0-9]*[1-9]|-?[1-9][0-9]*\.[0-9]*[1-9]|-?[1-9][0-9]*)" // NUM
};

inline constexpr const char *patternFor(TokenType type) {
  return PATTERN_BY_TYPE[static_cast<std::size_t>(type)];// the enum value is the array index.
}

// some asserts to avoid  quiet failure when array is misaligned:
inline constexpr bool cstrEq(const char *a, const char *b) {
  while (*a && *a == *b) {
    ++a;
    ++b;
  }
  return *a == *b;
}
static_assert(patternFor(TokenType::NUM) != nullptr,
              "PATTERN_BY_TYPE has fewer rows than there are TokenTypes");
static_assert(cstrEq(patternFor(TokenType::LPAREN), R"(\()"));
static_assert(cstrEq(patternFor(TokenType::MOD), R"(\bmod\b)"));
static_assert(cstrEq(patternFor(TokenType::PRINT), R"(\bprint\b)"));
static_assert(cstrEq(patternFor(TokenType::NAME), R"(#[0-9a-z]*)"));

#endif
