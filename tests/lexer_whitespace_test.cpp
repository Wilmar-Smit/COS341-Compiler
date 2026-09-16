// Lexer whitespace tests - TokenHandler / createChain() driven directly,
// bypassing the parser entirely.
//
// Blank-space handling lives in two different places in this codebase:
//   1. src/fileReader/filereader.h (readFileToString) normalizes '\n' and
//      '\r' to a literal ' ' (ASCII 32) and collapses runs of spaces,
//      before the lexer ever sees the text.
//   2. src/tokens/tokenHandler/tokenHandler.cpp / tokenTable.h implement
//      blank_space in TokenHandler itself purely as the literal ASCII 32
//      space character (TokenType::SPACE's pattern is R"( )" - nothing
//      else matches it).
//
// These tests call TokenHandler directly, so only (2) is in effect. That
// means "does the lexer itself, independent of the file-reading
// preprocessing step, treat something other than a literal space as
// blank_space" gets a real answer instead of an assumption.

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "test_helpers.h"

namespace {

struct ExpectedToken {
  TokenType type;
  std::string code;
};

// Tokenizes `input` directly against TokenHandler/createChain(), then
// strips SPACE-type tokens (mirroring what ParseVisitor does before
// consulting the parse table) and returns what's left.
std::vector<ExpectedToken> tokenizeAndStripSpaces(const std::string &input) {
  std::vector<Token *> raw = tokenizeAll(input);
  std::vector<ExpectedToken> filtered;
  for (auto *t : raw) {
    if (t->getType() != TokenType::SPACE) {
      filtered.push_back({t->getType(), t->getCode()});
    }
  }
  for (auto *t : raw) delete t;
  return filtered;
}

void requireSequence(const std::vector<ExpectedToken> &actual,
                      const std::vector<ExpectedToken> &expected) {
  INFO("actual token count: " << actual.size()
                               << ", expected: " << expected.size());
  REQUIRE(actual.size() == expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    INFO("token " << i << ": expected code '" << expected[i].code
                   << "', got '" << (i < actual.size() ? actual[i].code : "")
                   << "'");
    REQUIRE(actual[i].type == expected[i].type);
    REQUIRE(actual[i].code == expected[i].code);
  }
}

}  // namespace

TEST_CASE("small known input tokenizes to the expected non-space sequence",
          "[lexer][whitespace]") {
  auto tokens = tokenizeAndStripSpaces("( #x )");
  requireSequence(tokens, {
                              {TokenType::LPAREN, "("},
                              {TokenType::NAME, "#x"},
                              {TokenType::RPAREN, ")"},
                          });
}

TEST_CASE("multiple consecutive spaces between two tokens collapse cleanly",
          "[lexer][whitespace]") {
  auto tokens = tokenizeAndStripSpaces("(   )");
  requireSequence(tokens, {
                              {TokenType::LPAREN, "("},
                              {TokenType::RPAREN, ")"},
                          });
}

TEST_CASE("leading whitespace before the first real token is skipped",
          "[lexer][whitespace]") {
  auto tokens = tokenizeAndStripSpaces("  ( #x )");
  requireSequence(tokens, {
                              {TokenType::LPAREN, "("},
                              {TokenType::NAME, "#x"},
                              {TokenType::RPAREN, ")"},
                          });
}

TEST_CASE("trailing whitespace after the last real token is skipped",
          "[lexer][whitespace]") {
  auto tokens = tokenizeAndStripSpaces("( #x )   ");
  requireSequence(tokens, {
                              {TokenType::LPAREN, "("},
                              {TokenType::NAME, "#x"},
                              {TokenType::RPAREN, ")"},
                          });
}

TEST_CASE("empty input string produces no token and no throw",
          "[lexer][whitespace][edge]") {
  TokenHandler *chain = newHandlerChain();
  TokenResult result;
  REQUIRE_NOTHROW(result = chain->handle(""));
  REQUIRE(result.token == nullptr);
  REQUIRE(result.remainingStream == "");
  delete chain;
}

TEST_CASE(
    "a tab character is NOT recognised as blank_space by TokenHandler "
    "itself",
    "[lexer][whitespace][gap]") {
  // GAP: TokenType::SPACE's pattern (tokenTable.h) is the literal ASCII 32
  // space character only. There is no ASCII 9 (tab) handling anywhere in
  // TokenHandler/tokenTable - a tab between two otherwise-valid tokens
  // makes the whole run-of-non-space-characters fail every handler's
  // regex_match and TokenHandler::handle falls through to
  // `throw std::runtime_error("Unexpected token symbol: ...")`.
  // Documenting the actual (failing) behaviour here rather than silently
  // assuming tabs work.
  TokenHandler *chain = newHandlerChain();
  INFO("expected: throws, because no TokenType pattern matches a run "
       "containing a raw tab character, and TokenHandler has no "
       "tab-specific handling");
  REQUIRE_THROWS_AS(chain->handle("(\t)"), std::runtime_error);
  delete chain;
}

TEST_CASE(
    "a raw carriage return is NOT recognised as blank_space by "
    "TokenHandler itself (only ASCII 32 is)",
    "[lexer][whitespace][gap]") {
  // GAP: the project spec (per the practical brief) allows blank_space to
  // be ASCII 32 (space) or ASCII 13 (carriage return). Carriage-return
  // handling does exist in this codebase, but only as a preprocessing
  // step in src/fileReader/filereader.h::readFileToString, which rewrites
  // '\r'/'\n' to ' ' *before* the lexer ever runs. TokenHandler itself
  // (tokenTable.h's SPACE pattern is literal ASCII 32 only) has no
  // handling for '\r' at all. Fed a raw '\r' directly - i.e. any caller
  // that doesn't route input through readFileToString first - tokenizing
  // fails the same way it does for a tab.
  TokenHandler *chain = newHandlerChain();
  INFO("expected: throws, because TokenHandler has no ASCII-13 handling of "
       "its own; that only exists upstream in filereader.h's "
       "readFileToString, which this test deliberately bypasses");
  REQUIRE_THROWS_AS(chain->handle("(\r)"), std::runtime_error);
  delete chain;
}
