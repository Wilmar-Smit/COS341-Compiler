#include "Parser.h"
#include "createParseTable.h"
#include "parser.visitor.h"
#include "state.enum.h"
#include <stack>

// adds the parser
Parser::Parser() {
  this->table = createParseTable();
  this->stack.push(ParserStates::S0); // S0 pushed on

  this->visitor = new ParseVisitor(
      table, stack); // does not manage memory these are just stacks
}

auto Parser::ParseTokens(vector<Token *> tokens) {

  for (auto token : tokens) {
  }
}

// memory management
Parser::~Parser() {
  for (auto row : table) {
    for (auto action : row) {
      if (action)
        delete action;
    }
  }
  if (visitor) {
    delete visitor;
  }
}
