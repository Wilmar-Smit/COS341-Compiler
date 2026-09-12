#ifndef PRODUCTION_RULES_H
#define PRODUCTION_RULES_H

#include "../../tokens/token.enum.h"
#include "../../tokens/token.h"

#include <cstddef>
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

// Number of NonTerminal enumerators. Assumes they are contiguous (0..N-1)
// and that COND is the last one - keep COND last if you add more.
inline constexpr std::size_t NONTERMINAL_COUNT =
    static_cast<std::size_t>(NonTerminal::COND) + 1;

using GrammarSymbol = variant<Token, NonTerminal>;

struct ProductionRule {
  NonTerminal nonTerminal;
  vector<GrammarSymbol> RHS;
  int numberToPop;
};

#endif
