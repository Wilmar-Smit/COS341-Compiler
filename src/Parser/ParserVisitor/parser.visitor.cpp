#include "parser.visitor.h"
#include "../../tokens/tokenTable.h"
#include "ProductionRules.h"
#include "parser.actions.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;
const std::string GREEN = "\033[32m";
const std::string RED = "\033[31m";
const std::string RESET = "\033[0m";

ParseVisitor::ParseVisitor(vector<vector<ParserAction*>>& table,
  vector<vector<ParserAction*>>& gotoTable,
  std::stack<ParserStates>& stateStack,
  TreeBuilder& tb)
  : table(table), stateStack(stateStack), gotoTable(gotoTable), xml(tb) {
  // keeps a reference to the same tables and stack as the parser
}

Token* ParseVisitor::currentToken()
{
  if (this->tokenIndex < static_cast<int>(this->tokens.size())) {
    return this->tokens[this->tokenIndex];
  }
  return &this->endOfInputToken;
}

bool ParseVisitor::parseTokens(vector<Token*> tokens)
{
  this->tokens = tokens;
  try
  {
    while (!this->hitAcceptState)
    {
      Token* lookahead = currentToken();

      if (lookahead->getCode() == " ") {
        this->tokenIndex++;
        continue;
      }

      this->StateIndex = static_cast<int>(stateStack.top());
      int tableTokenIndex = static_cast<int>(lookahead->getType());

      auto action = table[StateIndex][tableTokenIndex];

      action->AcceptVisitor(this);
    }
  }
  catch (std::runtime_error e)
  {
    std::cout << e.what() << std::endl;
    return false;
  }

  if (hitAcceptState) {
    xml.writeXML(xml.getRoot());
  }

  return this->hitAcceptState;
}

void ParseVisitor::visit(ParserAction* action)
{
  throw std::runtime_error("Visit should not be called on this abstract class");
}

void ParseVisitor::visit(ShiftAction* action) {

  this->stateStack.push(action->state);
  xml.shiftNode(*currentToken());
  this->tokenIndex++;
}

void ParseVisitor::visit(GotoAction* action) {
  this->stateStack.push(action->state);
}

void ParseVisitor::visit(AcceptAction* action) {
  this->hitAcceptState = true;
}

void ParseVisitor::visit(ReduceAction* action)
{
  int numToPop = action->rule.numberToPop;
  NonTerminal NT = action->rule.nonTerminal;

  xml.reduceNode(NT, numToPop);

  for (auto i = 0; i < numToPop; i++) {
    this->stateStack.pop();
  }

  int exposedStateIndex = static_cast<int>(stateStack.top());

  auto nonTerminalIndex = static_cast<int>(NT);
  auto gotoAction = gotoTable[exposedStateIndex][nonTerminalIndex];

  gotoAction->AcceptVisitor(this);
}

void ParseVisitor::visit(ErrorAction* action) {
  int stateIndex = static_cast<int>(stateStack.top());
  Token* lookahead = currentToken();
  int tableTokenIndex = static_cast<int>(lookahead->getType());
  std::stringstream possibleTokens;
  possibleTokens << GREEN << "\nPossible expressions";
  for (int rowTokenIdx = 0; rowTokenIdx < table[stateIndex].size();
       rowTokenIdx++) {

    auto action = table[stateIndex][rowTokenIdx];

    if (!action || action->type == actionType::ERROR) {
      continue;
    }

    auto expectedTokenType = static_cast<TokenType>(rowTokenIdx);

    switch (action->type) {
    case actionType::SHIFT:
    case actionType::REDUCE:
    case actionType::ACCEPT:
      possibleTokens << "[ " << patternFor(expectedTokenType) << " ]";
      break;
    default:
      break;
    }
  }

  std::stringstream ss;
  ss << RED << "Parsing failed: Unexpected token of type " << tableTokenIndex
     << " Being: " << patternFor(lookahead->getType())
     << " encountered in parser state " << stateIndex << " at token index "
     << tokenIndex << "." << RESET;

  ss << possibleTokens.str() << RESET;

  throw std::runtime_error(ss.str());
}
