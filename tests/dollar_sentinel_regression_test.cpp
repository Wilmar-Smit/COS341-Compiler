// Regression test: main.cpp's unconditional appended DOLLAR_EOF is
// load-bearing, not redundant - see PR description.

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

// EXPECTED-TO-FAIL-ACCEPT: failing/not accepting here is CORRECT, not a
// bug - today's driver loop never reaches AcceptAction with only one
// DOLLAR_EOF (see PR description). If this starts passing, investigate;
// it is not evidence the double-append is safe to remove.
TEST_CASE("single trailing DOLLAR_EOF does not reach AcceptAction",
          "[regression][dollar-sentinel][known-gap]") {
  std::vector<Token *> tokens = tokenizeAll(kMinimalFixture);
  REQUIRE(countTrailingDollarEof(tokens) == 1);

  PipelineResult result = runPipelineFromTokens(tokens);

  INFO("parser diagnostics:\n" << result.diagnostics);
  REQUIRE_FALSE(result.accepted);
  REQUIRE_FALSE(result.xmlWritten);
}
