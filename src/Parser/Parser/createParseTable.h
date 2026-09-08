#ifndef CREATE_PARSE_TABLE_H
#define CREATE_PARSE_TABLE_H

#include "../ParserActions/parser.actions.h"
#include <vector>
using std::vector;

// here is where we create the SLR parse table filled with our Actions
// Shift , reduce , error and accept
auto createParseTable() {

  vector<vector<ParserAction *>> table;
  // --------------
  // add here
  // --------------

  return table;
}

#endif
