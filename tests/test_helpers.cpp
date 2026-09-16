#include "test_helpers.h"

#include <iostream>

#include "tokens/genericTokenHandler/generic.handler.h"
#include "fileReader/parseTableReader.h"
#include "state.enum.h"
#include "parser.actions.h"
#include "parser.visitor.h"
#include "TreeBuilder.h"

// This is the single translation unit that includes generic.handler.h -
// see the note in test_helpers.h for why (its createChain() isn't marked
// `inline`, so a second #include site anywhere in the test binary would
// be an ODR violation at link time).

TokenHandler *newHandlerChain() { return createChain(); }

std::vector<Token *> tokenizeAll(const std::string &source) {
  TokenHandler *chain = createChain();
  std::vector<Token *> tokens;
  std::string stream = source;
  while (stream != "") {
    auto res = chain->handle(stream);
    tokens.push_back(res.token);
    stream = res.remainingStream;
  }
  delete chain;
  return tokens;
}

PipelineResult runPipeline(const std::string &source) {
  PipelineResult result;

  std::vector<Token *> tokenList = tokenizeAll(source);
  tokenList.push_back(new Token("$", TokenType::DOLLAR_EOF));

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
