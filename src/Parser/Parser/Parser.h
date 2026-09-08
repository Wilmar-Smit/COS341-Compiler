#ifndef PARSER_H
#define PARSER_H

#include "../ParserActions/parser.actions.h"
#include "parser.visitor.h"
#include "token.h"
#include <vector>
using std::vector;
class Parser {
  /**
   * handles the delection of the SLR parse Table
   * Does not delete the tokens array !
   */

private:
  vector<vector<ParserAction *>> table; // gets filled in by the function
  ParseVisitor *visitor = nullptr;

public:
  Parser();
  ~Parser();
  auto ParseTokens(vector<Token *> tokens);
};

#endif
