#ifndef CREATE_PARSE_TABLE_H
#define CREATE_PARSE_TABLE_H

#include "../ParserActions/parser.actions.h"
#include <vector>
using std::vector;

// here is where we create the SLR parse table filled with our Actions
// Shift , reduce , error and accept

// Indexed using
// row : state Enum
// Col : tokenType Enum
auto createParseTable() {

  vector<vector<ParserAction *>> table;
  // --------------
  // add here
  // --------------

  return table;
}

// Goto , error
// Indexed using
// row : stateEnum
// Col : NonTerminal Enum
auto createGotoTable() {
  vector<vector<ParserAction *>> table;
  // --------------
  // add here
  // --------------

  return table;
}

#endif
