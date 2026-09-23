// Regression tests for the driver loop's end-of-input handling: once the
// real token stream is exhausted, lookahead must keep sticking at
// DOLLAR_EOF for any further reduce/accept decisions instead of the loop
// exiting early - see PR description.

#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <sstream>
#include <stack>
#include <vector>

#include "fileReader/parseTableReader.h"
#include "parser.actions.h"
#include "parser.visitor.h"
#include "state.enum.h"
#include "test_helpers.h"
#include "TreeBuilder.h"

namespace {

// Same minimal all-epsilon fixture as parser_coverage_test.cpp.
const std::string kMinimalFixture = ": : $";

// runPipeline() always appends a DOLLAR_EOF, so this takes a pre-built
// token vector instead to produce the "only one, nothing appended" case.
PipelineResult runPipelineFromTokens(std::vector<Token *> tokenList) {
  PipelineResult result;

  ActionTableReader actionReader;
  auto table = actionReader.read("SLR_ACTION_Table.csv");
  GotoTableReader gotoReader;
  auto gotoTable = gotoReader.read("SLR_GOTO_Table.csv");

  std::stack<ParserStates> stateStack;
  stateStack.push(ParserStates::S0);
  TreeBuilder xml;
  ParseVisitor visitor(table, gotoTable, stateStack, xml);

  std::error_code ec;
  std::filesystem::remove("tree.xml", ec);

  std::ostringstream capture;
  std::streambuf *oldBuf = std::cout.rdbuf(capture.rdbuf());
  result.accepted = visitor.parseTokens(tokenList);
  std::cout.rdbuf(oldBuf);
  result.diagnostics = capture.str();

  result.xmlWritten = std::filesystem::exists("tree.xml");

  for (auto &row : table)
    for (auto *action : row) delete action;
  for (auto &row : gotoTable)
    for (auto *action : row) delete action;
  for (auto *token : tokenList) delete token;

  return result;
}

// Skips trailing SPACE tokens, like ParseVisitor does.
int countTrailingDollarEof(const std::vector<Token *> &tokens) {
  int count = 0;
  for (auto it = tokens.rbegin(); it != tokens.rend(); ++it) {
    if ((*it)->getType() == TokenType::DOLLAR_EOF) {
      count++;
    } else if ((*it)->getType() == TokenType::SPACE) {
      continue;
    } else {
      break;
    }
  }
  return count;
}

}  // namespace

TEST_CASE(
    "double DOLLAR_EOF (current main.cpp behavior) reaches AcceptAction",
    "[regression][dollar-sentinel]") {
  std::vector<Token *> tokens = tokenizeAll(kMinimalFixture);
  REQUIRE(countTrailingDollarEof(tokens) == 1);

  // Mirrors main.cpp's unconditional append.
  tokens.push_back(new Token("$", TokenType::DOLLAR_EOF));
  REQUIRE(countTrailingDollarEof(tokens) == 2);

  PipelineResult result = runPipelineFromTokens(tokens);

  INFO("parser diagnostics:\n" << result.diagnostics);
  REQUIRE(result.accepted);
  REQUIRE(result.xmlWritten);
}

// Was "single trailing DOLLAR_EOF does not reach AcceptAction" and
// documented the bug (loop exited on tokenIndex >= size before the
// trailing reduce-reduce-accept chain could run on the final lookahead).
// The driver now sticks at DOLLAR_EOF once real input is exhausted
// instead of stopping, so a single real $ is sufficient - this is the
// deliberate fix, flip the expectation rather than let it go stale.
TEST_CASE("single trailing DOLLAR_EOF reaches AcceptAction",
          "[regression][dollar-sentinel]") {
  std::vector<Token *> tokens = tokenizeAll(kMinimalFixture);
  REQUIRE(countTrailingDollarEof(tokens) == 1);

  PipelineResult result = runPipelineFromTokens(tokens);

  INFO("parser diagnostics:\n" << result.diagnostics);
  REQUIRE(result.accepted);
  REQUIRE(result.xmlWritten);
}

// Invariant: real tutor SPL files never contain a literal $ (course
// Announcement #23) - only main.cpp's appended DOLLAR_EOF supplies the
// one $ the grammar's SPL_PROG -> P $ rule requires.
TEST_CASE("zero-$ source accepts via main.cpp's single appended DOLLAR_EOF",
          "[regression][dollar-sentinel]") {
  std::vector<Token *> tokens = tokenizeAll(": :");
  REQUIRE(countTrailingDollarEof(tokens) == 0);

  tokens.push_back(new Token("$", TokenType::DOLLAR_EOF));

  PipelineResult result = runPipelineFromTokens(tokens);

  INFO("parser diagnostics:\n" << result.diagnostics);
  REQUIRE(result.accepted);
  REQUIRE(result.xmlWritten);
}

// Stress test: nested empty V_DECL/F_DECL/ALGO (two levels of function
// body, each all-epsilon) must still resolve down to AcceptAction, not
// just the fixed two-step chain the minimal fixture exercises.
TEST_CASE("deeply nested trailing epsilon reduces reach AcceptAction",
          "[regression][dollar-sentinel]") {
  std::string source =
      ": void #f ( ) { : void #g ( ) { : : return } : return } : $";
  std::vector<Token *> tokens = tokenizeAll(source);
  REQUIRE(countTrailingDollarEof(tokens) == 1);

  PipelineResult result = runPipelineFromTokens(tokens);

  INFO("parser diagnostics:\n" << result.diagnostics);
  REQUIRE(result.accepted);
  REQUIRE(result.xmlWritten);
}

// A genuine syntax error (unbalanced brace) must still hit ErrorAction,
// not hang or silently accept, once lookahead sticks at DOLLAR_EOF.
TEST_CASE("malformed program still triggers ErrorAction, not a hang",
          "[regression][dollar-sentinel]") {
  std::vector<Token *> tokens = tokenizeAll(": void #f ( ) { : : return $");

  PipelineResult result = runPipelineFromTokens(tokens);

  INFO("parser diagnostics:\n" << result.diagnostics);
  REQUIRE_FALSE(result.accepted);
  REQUIRE_FALSE(result.xmlWritten);
  REQUIRE_FALSE(result.diagnostics.empty());
}
