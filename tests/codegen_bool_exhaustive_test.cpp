// Every BOOL over three 0/1 variables up to a sampled depth 3, compiled into
// one program and compared against plain C++ bool logic for all 8 variable
// assignments.
//
// Token syntax matches input.txt: every token is its own space-separated
// word, e.g. "( #x )" not "(#x)".

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "codegen/codeGen.h"
#include "test_helpers.h"

namespace {

constexpr int ASSIGNMENTS = 8;
constexpr std::size_t SAMPLE_STRIDE = 8;

using Table = std::array<bool, ASSIGNMENTS>;

// index = p * 4 + q * 2 + r, the order the generated loops visit them in
struct Expr {
  std::string spl;
  Table table;
};

Expr atom(const std::string &name, int bit) {
  Expr expr{"eq ( #" + name + " 1 )", {}};
  for (int i = 0; i < ASSIGNMENTS; i++)
    expr.table[i] = ((i >> bit) & 1) != 0;
  return expr;
}

Expr negate(const Expr &inner) {
  Expr expr{"not ( " + inner.spl + " )", {}};
  for (int i = 0; i < ASSIGNMENTS; i++)
    expr.table[i] = !inner.table[i];
  return expr;
}

Expr conjunction(const Expr &left, const Expr &right) {
  Expr expr{"and ( " + left.spl + " " + right.spl + " )", {}};
  for (int i = 0; i < ASSIGNMENTS; i++)
    expr.table[i] = left.table[i] && right.table[i];
  return expr;
}

Expr disjunction(const Expr &left, const Expr &right) {
  Expr expr{"or ( " + left.spl + " " + right.spl + " )", {}};
  for (int i = 0; i < ASSIGNMENTS; i++)
    expr.table[i] = left.table[i] || right.table[i];
  return expr;
}

std::vector<Expr> levelOne() {
  const std::vector<Expr> atoms = {atom("p", 2), atom("q", 1), atom("r", 0)};
  std::vector<Expr> all = atoms;
  for (const Expr &a : atoms)
    all.push_back(negate(a));
  for (const Expr &a : atoms) {
    for (const Expr &b : atoms) {
      all.push_back(conjunction(a, b));
      all.push_back(disjunction(a, b));
    }
  }
  return all;
}

// Fixed stride over every not/and/or applied to level-one expressions.
std::vector<Expr> levelThreeSample(const std::vector<Expr> &base) {
  std::vector<Expr> pool;
  for (const Expr &a : base)
    pool.push_back(negate(a));
  for (const Expr &a : base) {
    for (const Expr &b : base) {
      pool.push_back(conjunction(a, b));
      pool.push_back(disjunction(a, b));
    }
  }

  std::vector<Expr> sample;
  for (std::size_t i = 0; i < pool.size(); i += SAMPLE_STRIDE)
    sample.push_back(pool[i]);
  return sample;
}

double secondsSince(std::chrono::steady_clock::time_point start) {
  return std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                       start)
      .count();
}

} // namespace

TEST_CASE("compiled BOOL matches C++ logic for every assignment",
          "[codegen][run][exhaustive]") {
  if (findCxxCompiler().empty())
    SKIP("no C++ compiler (CXX, g++, clang++ or c++) available");

  std::vector<Expr> exprs = levelOne();
  const std::size_t levelOneCount = exprs.size();
  for (const Expr &expr : levelThreeSample(exprs))
    exprs.push_back(expr);
  REQUIRE(levelOneCount == 24);

  // The program loops p, q, r over 0 and 1 (outermost first) instead of
  // repeating every expression eight times: the lexer re-copies the rest of
  // the input per token, so a source that large would be quadratic.
  std::string body;
  for (const Expr &expr : exprs)
    body += "if " + expr.spl +
            " then { print ( 1 ) ; } else { print ( 0 ) ; } ; ";
  const std::string spl =
      "#p #q #r : : "
      "#p = 0 ; while lesser ( #p 2 ) do { "
      "#q = 0 ; while lesser ( #q 2 ) do { "
      "#r = 0 ; while lesser ( #r 2 ) do { " + body +
      "#r = add ( #r 1 ) ; } ; "
      "#q = add ( #q 1 ) ; } ; "
      "#p = add ( #p 1 ) ; } ; ";

  std::string expected;
  for (int assignment = 0; assignment < ASSIGNMENTS; assignment++) {
    for (const Expr &expr : exprs)
      expected += expr.table[assignment] ? '1' : '0';
  }

  auto start = std::chrono::steady_clock::now();
  CompiledProgram program = compileSource(spl);
  double parseSeconds = secondsSince(start);

  start = std::chrono::steady_clock::now();
  CodeGen gen(determineNumType(program.root));
  std::string fragment =
      gen.genAlgo(program.root->children[0]->children[4]);
  double genSeconds = secondsSince(start);

  start = std::chrono::steady_clock::now();
  std::string actual =
      compileAndRun(wrapInMain(program, gen.getNumType(), fragment));
  double runSeconds = secondsSince(start);

  WARN("expressions: " << exprs.size() << ", generated lines: "
       << std::count(fragment.begin(), fragment.end(), '\n')
       << ", parse+analyse " << parseSeconds << "s, codegen " << genSeconds
       << "s, C++ compile+run " << runSeconds << "s");

  REQUIRE(actual.size() == expected.size());
  for (std::size_t i = 0; i < expected.size(); i++) {
    if (actual[i] != expected[i]) {
      std::size_t exprIndex = (i % exprs.size());
      INFO("assignment " << i / exprs.size() << ", expression "
                         << exprs[exprIndex].spl);
      REQUIRE(actual[i] == expected[i]);
    }
  }
}
