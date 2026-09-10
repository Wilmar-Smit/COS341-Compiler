#ifndef PRODUCTION_RULES_H
#define PRODUCTION_RULES_H

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
struct ProductionRule {
  NonTerminal nonTerminal;
  int numberToPop;
};

#endif
