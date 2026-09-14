#include "parser.visitor.h"
#include "../../tokens/tokenTable.h"
#include "ProductionRules.h"
#include "parser.actions.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

ParseVisitor::ParseVisitor(vector<vector<ParserAction *>> &table,
                           vector<vector<ParserAction *>> &gotoTable,
                           std::stack<ParserStates> &stateStack,
                          TreeBuilder& tb)
    : table(table), stateStack(stateStack), gotoTable(gotoTable), xml(tb) {
  // keeps a reference to the same tables and stack as the parser
}

bool ParseVisitor::parseTokens(vector<Token *> tokens)
{
  this->tokens = tokens;
  try
  {
    while (this->tokenIndex < tokens.size())
    {

      if (tokens[this->tokenIndex]->getCode() == " "){
        this->tokenIndex++;
        continue;
      }

      this->StateIndex = static_cast<int>(stateStack.top());
      int tableTokenIndex =
          static_cast<int>(tokens[this->tokenIndex]->getType());

      auto action = table[StateIndex][tableTokenIndex];
      
      action->AcceptVisitor(this);
    }
  }
  catch (std::runtime_error e)
  {
    std::cout << e.what() << std::endl;
    return false;
  }
  return this->hitAcceptState;
}

void ParseVisitor::visit(ParserAction *action)
{
  throw std::runtime_error("Visit should not be called on this abstract class");
}

void ParseVisitor::visit(ShiftAction *action) {
  this->stateStack.push(action->state);
  xml.shiftNode(*tokens[this->tokenIndex]);
  this->tokenIndex++;
}

void ParseVisitor::visit(GotoAction *action) {
  this->stateStack.push(action->state);
}

void ParseVisitor::visit(AcceptAction *action) { this->hitAcceptState = true; }

void ParseVisitor::visit(ReduceAction *action)
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

void ParseVisitor::visit(ErrorAction *action) {
  int stateIndex = static_cast<int>(stateStack.top());
  int tableTokenIndex = static_cast<int>(tokens[tokenIndex]->getType());

  std::stringstream ss;
  ss << "Parsing failed: Unexpected token of type " << tableTokenIndex
     << " Being: " << patternFor(tokens[tokenIndex]->getType())
     << " encountered in parser state " << stateIndex << " at token index "
     << tokenIndex << ".";

  throw std::runtime_error(ss.str());
}
