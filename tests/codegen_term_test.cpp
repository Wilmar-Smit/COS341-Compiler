// Step 2 of phase 3a: TERM, ASSIGN and print ( TERM ). Assertions are on the
// exact generated text, so temporary numbering and declaration order are
// pinned down.
//
// Token syntax matches input.txt: every token is its own space-separated
// word, e.g. "( #x )" not "(#x)".

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <array>
#include <stdexcept>
#include <string>

#include "codegen/codeGen.h"
#include "test_helpers.h"

using Catch::Matchers::ContainsSubstring;

namespace {

Node *algoOf(const CompiledProgram &program) {
  return program.root->children[0]->children[4];
}

Node *firstInstrOf(const CompiledProgram &program) {
  return algoOf(program)->children[0];
}

// INSTR -> ASSIGN, ASSIGN -> NAME = TERM
Node *assignTermOf(const CompiledProgram &program) {
  return firstInstrOf(program)->children[0]->children[2];
}

// V_DECL -> NAME V_DECL, so the nth declared variable is nth in the chain.
std::string declaredName(const CompiledProgram &program, int index) {
  const Node *decl = program.root->children[0]->children[0];
  for (int i = 0; i < index; i++)
    decl = decl->children[1];
  return decl->children[0]->symbol;
}

struct Names {
  std::string x;
  std::string y;
};

Names namesOf(const CompiledProgram &program) {
  return {declaredName(program, 0), declaredName(program, 1)};
}

} // namespace

TEST_CASE("genTerm copies a NAME", "[codegen][term]") {
  CompiledProgram program = compileSource("#x #y : : #x = #y ;");
  Names n = namesOf(program);
  CodeGen gen("int");
  REQUIRE(gen.genTerm(assignTermOf(program), "r") == "r = " + n.y + ";\n");
  REQUIRE(gen.newVar() == "v1");
}

TEST_CASE("genTerm copies a NUM verbatim", "[codegen][term]") {
  CompiledProgram program = compileSource("#x #y : : #x = 42 ;");
  CodeGen gen("int");
  REQUIRE(gen.genTerm(assignTermOf(program), "r") == "r = 42;\n");
}

TEST_CASE("genTerm keeps the sign and decimals of a literal",
          "[codegen][term]") {
  CompiledProgram negative = compileSource("#x #y : : #x = -7 ;");
  CompiledProgram decimal = compileSource("#x #y : : #x = -0.25 ;");
  CodeGen gen("float");
  REQUIRE(gen.genTerm(assignTermOf(negative), "r") == "r = -7;\n");
  REQUIRE(gen.genTerm(assignTermOf(decimal), "r") == "r = -0.25;\n");
}

TEST_CASE("neg of a name", "[codegen][term]") {
  CompiledProgram program = compileSource("#x #y : : #x = neg ( #y ) ;");
  Names n = namesOf(program);
  CodeGen gen("int");
  REQUIRE(gen.genTerm(assignTermOf(program), "r") ==
          "int v1;\nv1 = " + n.y + ";\nr = -v1;\n");
}

TEST_CASE("each binary operator maps to its C operator", "[codegen][term]") {
  struct Case {
    std::string spl;
    std::string c;
    std::string type;
  };
  const std::array<Case, 5> cases = {{{"mod", "%", "int"},   {"div", "/", "float"},
                        {"add", "+", "int"},   {"sub", "-", "int"},
                        {"mul", "*", "int"}}};

  for (const Case &c : cases) {
    INFO("operator " << c.spl);
    CompiledProgram program =
        compileSource("#x #y : : #x = " + c.spl + " ( #y 3 ) ;");
    Names n = namesOf(program);
    CodeGen gen(determineNumType(program.root));
    REQUIRE(gen.genInstr(firstInstrOf(program)) ==
            c.type + " v1;\n" + c.type + " v2;\n" + c.type + " v3;\nv2 = " +
                n.y + ";\nv3 = 3;\nv1 = v2 " + c.c + " v3;\n" + n.x + " = v1");
  }
}

TEST_CASE("operands that are names or numbers still get temporaries",
          "[codegen][term]") {
  CompiledProgram program = compileSource("#x #y : : #x = add ( 1 2 ) ;");
  Names n = namesOf(program);
  CodeGen gen("int");
  REQUIRE(gen.genInstr(firstInstrOf(program)) ==
          "int v1;\nint v2;\nint v3;\nv2 = 1;\nv3 = 2;\nv1 = v2 + v3;\n" +
              n.x + " = v1");
}

TEST_CASE("nested terms number and declare temporaries parent-first",
          "[codegen][term]") {
  CompiledProgram program =
      compileSource("#x #y : : #x = add ( mul ( #x 2 ) neg ( #y ) ) ;");
  Names n = namesOf(program);
  CodeGen gen("int");
  REQUIRE(gen.genInstr(firstInstrOf(program)) ==
          "int v1;\n"
          "int v2;\n"
          "int v3;\n"
          "int v4;\n"
          "int v5;\n"
          "v4 = " + n.x + ";\n"
          "v5 = 2;\n"
          "v2 = v4 * v5;\n"
          "int v6;\n"
          "v6 = " + n.y + ";\n"
          "v3 = -v6;\n"
          "v1 = v2 + v3;\n" +
              n.x + " = v1");
}

TEST_CASE("ASSIGN with a plain NAME", "[codegen][assign]") {
  CompiledProgram program = compileSource("#x #y : : #x = #y ;");
  Names n = namesOf(program);
  CodeGen gen("int");
  REQUIRE(gen.genInstr(firstInstrOf(program)) ==
          "int v1;\nv1 = " + n.y + ";\n" + n.x + " = v1");
}

TEST_CASE("ASSIGN output has no trailing semicolon or newline",
          "[codegen][assign]") {
  CompiledProgram program = compileSource("#x #y : : #x = 5 ;");
  CodeGen gen("int");
  std::string text = gen.genInstr(firstInstrOf(program));
  REQUIRE(text.back() != ';');
  REQUIRE(text.back() != '\n');
}

TEST_CASE("print ( NAME )", "[codegen][print]") {
  CompiledProgram program = compileSource("#x #y : : print ( #x ) ;");
  Names n = namesOf(program);
  CodeGen gen("int");
  REQUIRE(gen.genInstr(firstInstrOf(program)) ==
          "int v1;\nv1 = " + n.x + ";\nstd::cout << v1");
}

TEST_CASE("print ( nested term )", "[codegen][print]") {
  CompiledProgram program = compileSource("#x #y : : print ( sub ( #x 1 ) ) ;");
  Names n = namesOf(program);
  CodeGen gen("int");
  REQUIRE(gen.genInstr(firstInstrOf(program)) ==
          "int v1;\nint v2;\nint v3;\nv2 = " + n.x +
              ";\nv3 = 1;\nv1 = v2 - v3;\nstd::cout << v1");
}

TEST_CASE("a float program declares float temporaries", "[codegen][term]") {
  CompiledProgram withDecimal = compileSource("#x #y : : #x = 2.5 ;");
  CompiledProgram withDiv = compileSource("#x #y : : #x = div ( 1 2 ) ;");
  Names n = namesOf(withDecimal);
  CodeGen decimalGen(determineNumType(withDecimal.root));
  CodeGen divGen(determineNumType(withDiv.root));

  REQUIRE(decimalGen.genInstr(firstInstrOf(withDecimal)) ==
          "float v1;\nv1 = 2.5;\n" + n.x + " = v1");
  REQUIRE_THAT(divGen.genInstr(firstInstrOf(withDiv)),
               ContainsSubstring("float v1;\nfloat v2;\nfloat v3;\n"));
}

TEST_CASE("genAlgo continues temporary numbering across instructions",
          "[codegen][term]") {
  CompiledProgram program =
      compileSource("#x #y : : #x = add ( #x 3 ) ; print ( #x ) ;");
  Names n = namesOf(program);
  CodeGen gen("int");
  REQUIRE(gen.genAlgo(algoOf(program)) ==
          "int v1;\nint v2;\nint v3;\nv2 = " + n.x + ";\nv3 = 3;\n"
          "v1 = v2 + v3;\n" + n.x + " = v1;\n"
          "int v4;\nv4 = " + n.x + ";\nstd::cout << v4;\n/* end */");
}

TEST_CASE("a CALL inside a TERM is deferred to phase 3b", "[codegen][term]") {
  const std::string fn = "num #f ( ) { : : return ( 1 ) } : ";
  CompiledProgram direct = compileSource("#x : " + fn + "#x = #f ( ) ;");
  CompiledProgram nested =
      compileSource("#x : " + fn + "#x = add ( #f ( ) 1 ) ;");
  CodeGen gen("int");

  REQUIRE_THROWS_AS(gen.genInstr(firstInstrOf(direct)), CodeGenUnsupported);
  REQUIRE_THROWS_WITH(gen.genInstr(firstInstrOf(nested)),
                      ContainsSubstring("Phase 3b"));
}

TEST_CASE("CALL instructions are still deferred",
          "[codegen]") {
  CodeGen gen("int");

  CompiledProgram call =
      compileSource(": void #p ( ) { : : nop ; return } : #p ( ) ;");

  REQUIRE_THROWS_WITH(gen.genInstr(firstInstrOf(call)),
                      ContainsSubstring("Phase 3b"));
}

TEST_CASE("genTerm rejects malformed nodes", "[codegen][term]") {
  CompiledProgram program = compileSource("#x #y : : #x = 1 ;");
  CodeGen gen("int");
  REQUIRE_THROWS_AS(gen.genTerm(nullptr, "r"), std::logic_error);
  REQUIRE_THROWS_AS(gen.genTerm(algoOf(program), "r"), std::logic_error);
}
