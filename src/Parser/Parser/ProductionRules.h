#ifndef PRODUCTION_RULES_H
#define PRODUCTION_RULES_H

#include "../../tokens/token.enum.h"
#include "../../tokens/token.h"

#include <variant>
#include <vector>
using std::variant;
using std::vector;

enum class NonTerminal {
  SPL_PROG,
  P,
  V_DECL,
  F_DECL,
  F_TYPE,
  ALGO,
  OUTP,
  INSTR,
  CALL,
  INPUT,
  ASSIGN,
  TERM,
  BRANCH,
  BOOL,
  LOOP,
  COND
};

using GrammarSymbol = variant<Token, NonTerminal>;

struct ProductionRule {
  NonTerminal nonTerminal;
  vector<GrammarSymbol> RHS;
  int numberToPop;
};

#endif
