// Step 1 of phase 3a: counters, numeric type selection and the ALGO
// instructions that need no term/boolean support yet. Programs go through
// the same lex -> parse -> ScopeTree -> TypeChecker run as main.cpp.
//
// Token syntax matches input.txt: every token is its own space-separated
// word, e.g. "( #x )" not "(#x)".

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <string>

#include "codegen/codeGen.h"
#include "test_helpers.h"

namespace {

// P -> V_DECL : F_DECL : ALGO, and SPL_PROG -> P $
Node *algoOf(const CompiledProgram &program) {
  return program.root->children[0]->children[4];
}

Node *firstInstrOf(const CompiledProgram &program) {
  return algoOf(program)->children[0];
}

std::string numTypeOf(const std::string &source) {
  CompiledProgram program = compileSource(source);
  return determineNumType(program.root);
}

void requireUnsupported(const std::string &source) {
  CompiledProgram program = compileSource(source);
  CodeGen gen("int");
  REQUIRE_THROWS_AS(gen.genInstr(firstInstrOf(program)), CodeGenUnsupported);
}

} // namespace

TEST_CASE("newVar and newLabel count from 1 independently", "[codegen]") {
  CodeGen gen("int");
  REQUIRE(gen.newVar() == "v1");
  REQUIRE(gen.newVar() == "v2");
  REQUIRE(gen.newLabel() == "L1");
  REQUIRE(gen.newVar() == "v3");
  REQUIRE(gen.newLabel() == "L2");
}

TEST_CASE("fresh CodeGen instances generate identical sequences",
          "[codegen]") {
  CodeGen first("int");
  CodeGen second("float");
  for (int i = 0; i < 3; i++) {
    REQUIRE(first.newVar() == second.newVar());
    REQUIRE(first.newLabel() == second.newLabel());
  }
}

TEST_CASE("CodeGen remembers its numeric type", "[codegen]") {
  REQUIRE(CodeGen("float").getNumType() == "float");
  REQUIRE(CodeGen("int").getNumType() == "int");
}

TEST_CASE("determineNumType picks int for a plain program", "[codegen]") {
  REQUIRE(numTypeOf(": : nop ;") == "int");
}

TEST_CASE("determineNumType picks float when div is used", "[codegen]") {
  REQUIRE(numTypeOf("#x : : #x = div ( 7 2 ) ;") == "float");
}

TEST_CASE("determineNumType picks float for a decimal literal", "[codegen]") {
  REQUIRE(numTypeOf("#x : : #x = 1.5 ;") == "float");
  REQUIRE(numTypeOf("#x : : #x = -0.25 ;") == "float");
}

TEST_CASE("determineNumType picks int when only mod is used", "[codegen]") {
  REQUIRE(numTypeOf("#x : : #x = mod ( 7 2 ) ;") == "int");
}

TEST_CASE("determineNumType looks inside function bodies", "[codegen]") {
  REQUIRE(numTypeOf("#x : num #f ( ) { : : return ( 2.5 ) } : #x = #f ( ) ;") ==
          "float");
  REQUIRE(numTypeOf("#x : num #f ( #a #b ) { : : return ( div ( #a #b ) ) } : "
                    "#x = #f ( 4 2 ) ;") == "float");
}

TEST_CASE("determineNumType ignores dots inside strings", "[codegen]") {
  REQUIRE(numTypeOf(": : print \"a.b\" ;") == "int");
}

TEST_CASE("determineNumType rejects a null tree", "[codegen]") {
  REQUIRE_THROWS_AS(determineNumType(nullptr), std::logic_error);
}

TEST_CASE("genAlgo turns an empty ALGO into the end marker", "[codegen]") {
  CompiledProgram program = compileSource(": :");
  CodeGen gen("int");
  REQUIRE(gen.genAlgo(algoOf(program)) == "/* end */");
}

TEST_CASE("genAlgo handles nop", "[codegen]") {
  CompiledProgram program = compileSource(": : nop ;");
  CodeGen gen("int");
  REQUIRE(gen.genAlgo(algoOf(program)) == "/* nop */;\n/* end */");
}

TEST_CASE("genAlgo handles comment", "[codegen]") {
  CompiledProgram program = compileSource(": : comment \"hi\" ;");
  CodeGen gen("int");
  REQUIRE(gen.genAlgo(algoOf(program)) == "/* \"hi\" */;\n/* end */");
}

TEST_CASE("genAlgo handles print STRING", "[codegen]") {
  CompiledProgram program = compileSource(": : print \"hi\" ;");
  CodeGen gen("int");
  REQUIRE(gen.genAlgo(algoOf(program)) ==
          "std::cout << \"hi\";\n/* end */");
}

TEST_CASE("genAlgo puts a newline after every semicolon", "[codegen]") {
  CompiledProgram program = compileSource(": : nop ; print \"a\" ;");
  CodeGen gen("int");
  REQUIRE(gen.genAlgo(algoOf(program)) ==
          "/* nop */;\nstd::cout << \"a\";\n/* end */");
}

TEST_CASE("genAlgo and genInstr reject malformed nodes", "[codegen]") {
  CompiledProgram program = compileSource(": : nop ;");
  CodeGen gen("int");
  REQUIRE_THROWS_AS(gen.genAlgo(nullptr), std::logic_error);
  REQUIRE_THROWS_AS(gen.genAlgo(firstInstrOf(program)), std::logic_error);
  REQUIRE_THROWS_AS(gen.genInstr(algoOf(program)), std::logic_error);
}

TEST_CASE("constructs from later steps throw CodeGenUnsupported",
          "[codegen]") {
  SECTION("BRANCH") {
    requireUnsupported(
        "#x : : if eq ( #x 1 ) then { nop ; } else { nop ; } ;");
  }
  SECTION("LOOP") {
    requireUnsupported("#x : : while eq ( #x 1 ) do { nop ; } ;");
  }
  SECTION("CALL") {
    requireUnsupported(": void #p ( ) { : : nop ; return } : #p ( ) ;");
  }
}

TEST_CASE("genAlgo propagates CodeGenUnsupported from a later instruction",
          "[codegen]") {
  CompiledProgram program = compileSource("#x : : nop ; while eq ( #x 1 ) do { nop ; } ;");
  CodeGen gen("int");
  REQUIRE_THROWS_AS(gen.genAlgo(algoOf(program)), CodeGenUnsupported);
}
