#ifndef PARSER_LOOP
#define PARSER_LOOP

#include "states/parseState.h"
#include "states/tableState.h"
#include "token.h"
#include <vector>
struct Action {
  ParseState* type;
  TableStates state;
};

struct Rule {
public:
  // Attached non terminal enum
  // Number of tokens to pop -> for the stack
};

class Parser {
private:
  std::vector<Rule> rules{};

public:
  auto parse(std::vector<Token *>) {}
};

#endif
