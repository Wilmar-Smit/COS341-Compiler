#ifndef PARSER_H
#define PARSER_H

#include "../ParserActions/parser.actions.h"
#include "../StateEnums/state.enum.h"
#include "parser.visitor.h"
#include "token.h"
#include <stack>
#include <vector>
using std::stack;
using std::vector;
class Parser {
  /**
   * handles the delection of the SLR parse Table
   * Does not delete the tokens array !
   */

private:
  vector<vector<ParserAction *>> table; // gets filled in by the function
  stack<ParserStates> stack;            // looks up what the next state is

  /**
   * @brief The visitor goes through the parse table on command of the stack
   * which should be initialised with S0 Based on the state of the stack and the
   * current token idx that the parser or visitor will keep track of the visitor
   * will then visit the action and add onto the stack and visit the action add
   * onto the stack and so on
   */
  ParseVisitor *visitor = nullptr;

public:
  Parser();
  ~Parser();
  auto ParseTokens(vector<Token *> tokens);
};

#endif
