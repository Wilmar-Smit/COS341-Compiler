// Step 5 of phase 3a: the program wrapper. Part A asserts the exact text of
// genProgram, part B compiles the complete returned text as a program and
// runs it against output worked out by hand from the SPL semantics.
//
// Token syntax matches input.txt: every token is its own space-separated
// word, e.g. "( #x )" not "(#x)".

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <stdexcept>
#include <string>

#include "codegen/codeGen.h"
#include "test_helpers.h"

using Catch::Matchers::ContainsSubstring;

namespace {

Node *pOf(const CompiledProgram &program) { return program.root->children[0]; }

std::string declaredName(const CompiledProgram &program, int index) {
  const Node *decl = pOf(program)->children[0];
  for (int i = 0; i < index; i++)
    decl = decl->children[1];
  return decl->children[0]->symbol;
}

const std::string HEAD =
    "/* begin of the non-executable code-mine for later inlining */\n"
    " \n"
    "/* end of the non-executable code-mine */\n"
    "/* begin of executable C code */\n"
    "#include <iostream>\n"
    "int main()\n"
    "{\n";

const std::string TAIL = "\nreturn 0; } /* end of executable C */";

} // namespace

TEST_CASE("genProgram wraps variables and instructions", "[codegen][program]") {
  CompiledProgram program =
      compileSource("#x #y : : nop ; print \"hi\" ;");
  std::string x = declaredName(program, 0);
  std::string y = declaredName(program, 1);
  CodeGen gen("int");
  REQUIRE(gen.genProgram(program.root) ==
          HEAD + "int " + x + ";\nint " + y + ";\n \n"
          "/* nop */;\nstd::cout << \"hi\";\n/* end */" + TAIL);
}

TEST_CASE("genProgram writes one blank space for an empty V_DECL",
          "[codegen][program]") {
  CompiledProgram program = compileSource(": : nop ;");
  CodeGen gen("int");
  REQUIRE(gen.genProgram(program.root) ==
          HEAD + " \n/* nop */;\n/* end */" + TAIL);
}

TEST_CASE("genProgram uses the numeric type for declarations",
          "[codegen][program]") {
  CompiledProgram program = compileSource("#x : : #x = 1.5 ;");
  CodeGen gen(determineNumType(program.root));
  REQUIRE_THAT(gen.genProgram(program.root),
               ContainsSubstring("{\nfloat " + declaredName(program, 0) +
                                 ";\n \n"));
}

TEST_CASE("return 0 closes the program on its own line", "[codegen][program]") {
  CompiledProgram program = compileSource(": : nop ;");
  CodeGen gen("int");
  std::string text = gen.genProgram(program.root);
  REQUIRE_THAT(text, ContainsSubstring("/* end */\nreturn 0; } /* end of "
                                       "executable C */"));
  REQUIRE(text.back() != '\n');
}

TEST_CASE("a non-empty F_DECL is deferred to phase 3b", "[codegen][program]") {
  CodeGen gen("int");

  CompiledProgram withFunction =
      compileSource("#x : num #f ( ) { : : return ( 1 ) } : nop ;");
  REQUIRE_THROWS_WITH(gen.genProgram(withFunction.root),
                      ContainsSubstring("Phase 3b"));

  CompiledProgram withCall = compileSource(
      ": void #p ( ) { : : nop ; return } : #p ( ) ;");
  REQUIRE_THROWS_AS(gen.genProgram(withCall.root), CodeGenUnsupported);
}

TEST_CASE("malformed program trees throw std::logic_error",
          "[codegen][program]") {
  CodeGen gen("int");
  CompiledProgram program = compileSource("#x : : nop ;");
  Node *validP = pOf(program);
  Node dummy{1, "x", {}};

  REQUIRE_THROWS_AS(gen.genProgram(nullptr), std::logic_error);
  REQUIRE_THROWS_AS(gen.genProgram(validP), std::logic_error);

  Node noDollar{2, "SPL_PROG", {validP, &dummy}};
  REQUIRE_THROWS_AS(gen.genProgram(&noDollar), std::logic_error);

  Node shortProg{3, "SPL_PROG", {validP}};
  REQUIRE_THROWS_AS(gen.genProgram(&shortProg), std::logic_error);

  Node badColon = *validP;
  badColon.children[1] = &dummy;
  Node badColonProg{4, "SPL_PROG", {&badColon, program.root->children[1]}};
  REQUIRE_THROWS_AS(gen.genProgram(&badColonProg), std::logic_error);

  Node nestedName{5, "nm", {&dummy}};
  Node emptyDecl{6, "V_DECL", {}};
  Node badDecl{7, "V_DECL", {&nestedName, &emptyDecl}};
  Node badVDeclP = *validP;
  badVDeclP.children[0] = &badDecl;
  Node badVDeclProg{8, "SPL_PROG", {&badVDeclP, program.root->children[1]}};
  REQUIRE_THROWS_AS(gen.genProgram(&badVDeclProg), std::logic_error);
}

namespace {

std::string runProgram(const std::string &source) {
  if (findCxxCompiler().empty())
    SKIP("no C++ compiler (CXX, g++, clang++ or c++) available");

  CompiledProgram program = compileSource(source);
  CodeGen gen(determineNumType(program.root));
  return compileAndRun(gen.genProgram(program.root));
}

} // namespace

TEST_CASE("compiled program prints computed values", "[codegen][program][run]") {
  const std::string spl =
      "#x #y : : #x = 6 ; #y = mul ( #x 7 ) ; "
      "print ( #y ) ; print \",\" ; print ( sub ( #y #x ) ) ; print \",\" ; "
      "print ( neg ( add ( #x 1 ) ) ) ; print \",\" ; "
      "print ( mod ( #y 5 ) ) ; ";

  REQUIRE(runProgram(spl) == "42,36,-7,2");
}

TEST_CASE("compiled program prints strings and ignores comments",
          "[codegen][program][run]") {
  const std::string spl =
      ": : print \"hello\" ; comment \"ignored\" ; nop ; print \"a.b\" ; ";

  REQUIRE(runProgram(spl) == "helloa.b");
}

TEST_CASE("compiled program takes the right branch arms",
          "[codegen][program][run]") {
  const std::string spl =
      "#x : : #x = 4 ; "
      "if larger ( #x 3 ) then { print \"big\" ; } "
      "else { print \"small\" ; } ; "
      "if and ( lesser ( #x 3 ) eq ( #x 4 ) ) then { print \"y\" ; } "
      "else { print \"n\" ; } ; ";

  REQUIRE(runProgram(spl) == "bign");
}

TEST_CASE("compiled program runs all four loop forms",
          "[codegen][program][run]") {
  const std::string step = "{ print ( #x ) ; #x = add ( #x 1 ) ; } ";
  const std::string spl =
      "#x : : "
      "#x = 0 ; while lesser ( #x 3 ) do " + step + "; print \",\" ; "
      "#x = 0 ; until eq ( #x 3 ) do " + step + "; print \",\" ; "
      "#x = 0 ; do " + step + "while lesser ( #x 3 ) ; print \",\" ; "
      "#x = 0 ; do " + step + "until eq ( #x 3 ) ; print \",\" ; ";

  REQUIRE(runProgram(spl) == "012,012,012,012,");
}

TEST_CASE("compiled float program keeps fractions", "[codegen][program][run]") {
  const std::string spl =
      "#x #y : : #x = div ( 7 2 ) ; #y = add ( #x 0.25 ) ; "
      "print ( #x ) ; print \",\" ; print ( #y ) ; ";

  REQUIRE(runProgram(spl) == "3.5,3.75");
}

TEST_CASE("compiled program with nested constructs", "[codegen][program][run]") {
  const std::string spl =
      "#x #y : : "
      "#x = 0 ; while lesser ( #x 3 ) do { #y = 0 ; "
      "while lesser ( #y #x ) do { print \"s\" ; #y = add ( #y 1 ) ; } ; "
      "print \",\" ; #x = add ( #x 1 ) ; } ; "
      "#x = 0 ; while lesser ( #x 4 ) do { "
      "if eq ( mod ( #x 2 ) 0 ) then { print \"e\" ; } else { "
      "if larger ( #x 2 ) then { print \"p\" ; } else { print \"o\" ; } ; } ; "
      "#x = add ( #x 1 ) ; } ; ";

  // x=0 e, x=1 o, x=2 e, x=3 p (strings are lower case only)
  REQUIRE(runProgram(spl) == ",s,ss,eoep");
}
