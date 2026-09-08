#include "Parser.h"
#include "createParseTable.h"
#include "parser.visitor.h"
// adds the parser
Parser::Parser() {
  this->table = createParseTable();
  this->visitor = new ParseVisitor();
}

auto Parser::ParseTokens(vector<Token *> tokens) {}

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
