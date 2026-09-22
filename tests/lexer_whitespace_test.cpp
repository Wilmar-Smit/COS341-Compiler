// Lexer whitespace tests - TokenHandler driven directly, bypassing the
// parser. Only TokenHandler's own SPACE handling (ASCII 32 literal) is in
// effect here; filereader.h's \n/\r normalization happens upstream.

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "test_helpers.h"

namespace {

struct ExpectedToken {
  TokenType type;
  std::string code;
};

// Tokenizes `input` and strips SPACE tokens, like ParseVisitor does.
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
  // GAP: TokenType::SPACE only matches ASCII 32; no tab handling exists.
  TokenHandler *chain = newHandlerChain();
  INFO("expected: throws, no TokenType pattern matches a raw tab");
  REQUIRE_THROWS_AS(chain->handle("(\t)"), std::runtime_error);
  delete chain;
}

TEST_CASE(
    "a raw carriage return is NOT recognised as blank_space by "
    "TokenHandler itself (only ASCII 32 is)",
    "[lexer][whitespace][gap]") {
  // GAP: \r is normalized upstream in filereader.h, not by TokenHandler.
  TokenHandler *chain = newHandlerChain();
  INFO("expected: throws, TokenHandler has no ASCII-13 handling");
  REQUIRE_THROWS_AS(chain->handle("(\r)"), std::runtime_error);
  delete chain;
}
