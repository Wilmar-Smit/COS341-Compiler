#include "Parser.h"
#include "createParseTable.h"
#include "parser.visitor.h"
#include "state.enum.h"
#include <iostream>
#include <stdexcept>
// adds the parser
Parser::Parser() {
  this->table = createParseTable();
  this->gotoTable = createGotoTable();
  this->stack.push(ParserStates::S0); // S0 pushed on

  this->visitor = new ParseVisitor(
      table, gotoTable,
      stack); // does not manage memory these are just references
}

bool Parser::ParseTokens(vector<Token *> tokens) {

  try {
    auto accept = visitor->parseTokens(tokens);

    if (accept) {
      // return visitor . get composite tree
    } else {
      // throw exeption ? idk
    }
  } catch (runtime_error e) {
    std::cout << e.what() << std::endl;
    return false;
  }

  return true;
}

// memory management
Parser::~Parser() {
  for (auto row : table) {
    for (auto action : row) {
      if (action)
        delete action;
    }
  }
  for (auto row : gotoTable) {
    for (auto action : row) {
      if (action)
        delete action;
    }
  }
  if (visitor) {
    delete visitor;
  }
}
