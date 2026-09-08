#include "parser.visitor.h"
#include "ProductionRules.h"
#include "parser.actions.h"
#include <sstream>
#include <stdexcept>

ParseVisitor::ParseVisitor(vector<vector<ParserAction *>> &table,
                           vector<vector<ParserAction *>> &gotoTable,
                           std::stack<ParserStates> &stack)
    : table(table), stack(stack), gotoTable(gotoTable) {
  // keeps a reference to the same tables and stack as the parser
}

bool ParseVisitor::parseTokens(vector<Token *> tokens) {

  this->tokens = tokens;
  try {
    while (this->tokenIndex < tokens.size()) {

      this->StateIndex = static_cast<int>(stack.top());
      int tableTokenIndex =
          static_cast<int>(tokens[this->tokenIndex]->getType());

      auto action = table[StateIndex][tableTokenIndex];

      action->AcceptVisitor(this);
    }
  } catch (std::runtime_error e) {
  }

  return this->hitAcceptState;
}

void ParseVisitor::visit(ParserAction *action) {
  throw std::runtime_error("Visit should not be called on this abstract class");
}

void ParseVisitor::visit(ShiftAction *action) {
  this->stack.push(action->state);
  this->tokenIndex++;
}

void ParseVisitor::visit(GotoAction *action) {
  this->stack.push(action->state);
}

void ParseVisitor::visit(AcceptAction *action) { this->hitAcceptState = true; }

void ParseVisitor::visit(ReduceAction *action) {
  int numToPop = action->rule.numberToPop;
  NonTerminal NT = action->rule.nonTerminal;

  for (auto i = 0; i < numToPop; i++) {
    this->stack.pop();
  }

  int exposedStateIndex = static_cast<int>(stack.top());

  auto nonTerminalIndex = static_cast<int>(NT);
  auto gotoAction = gotoTable[exposedStateIndex][nonTerminalIndex];

  gotoAction->AcceptVisitor(this);
}

void ParseVisitor::visit(ErrorAction *action) {
  int stateIndex = static_cast<int>(stack.top());
  int tableTokenIndex = static_cast<int>(tokens[tokenIndex]->getType());

  std::stringstream ss;
  ss << "Parsing failed: Unexpected token of type " << tableTokenIndex
     << " encountered in parser state " << stateIndex << " at token index "
     << tokenIndex << ".";

  throw std::runtime_error(ss.str());
}
