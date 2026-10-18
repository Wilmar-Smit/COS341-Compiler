// Step 3 of phase 3a: BOOL and BRANCH. Part A asserts the exact generated
// text; part B compiles and runs the generated code, with expected output
// worked out by hand from the SPL semantics.
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

// INSTR -> BRANCH, BRANCH -> if BOOL then { ALGO } else { ALGO }
Node *branchBoolOf(const CompiledProgram &program) {
  return firstInstrOf(program)->children[0]->children[1];
}

std::string declaredName(const CompiledProgram &program, int index) {
  const Node *decl = program.root->children[0]->children[0];
  for (int i = 0; i < index; i++)
    decl = decl->children[1];
  return decl->children[0]->symbol;
}

} // namespace

TEST_CASE("comparisons map to their C operators", "[codegen][bool]") {
  const std::array<std::pair<std::string, std::string>, 3> ops = {
      {{"eq", "=="}, {"larger", ">"}, {"lesser", "<"}}};
  for (const auto &op : ops) {
    INFO("comparison " << op.first);
    CompiledProgram program = compileSource(
        "#x #y : : if " + op.first + " ( #x #y ) then { } else { } ;");
    std::string x = declaredName(program, 0);
    std::string y = declaredName(program, 1);
    CodeGen gen("int");
    REQUIRE(gen.genBool(branchBoolOf(program), "T", "F") ==
            "int v1;\nint v2;\nv1 = " + x + ";\nv2 = " + y +
                ";\nif (v1 " + op.second + " v2) goto T;\n");
  }
}

TEST_CASE("or jumps to the true label from either operand",
          "[codegen][bool]") {
  CompiledProgram program = compileSource(
      "#x #y : : if or ( eq ( #x #y ) lesser ( #x 1 ) ) then { } else { } ;");
  std::string x = declaredName(program, 0);
  std::string y = declaredName(program, 1);
  CodeGen gen("int");
  REQUIRE(gen.genBool(branchBoolOf(program), "T", "F") ==
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = " + y +
              ";\nif (v1 == v2) goto T;\n"
              "int v3;\nint v4;\nv3 = " + x +
              ";\nv4 = 1;\nif (v3 < v4) goto T;\n");
}

TEST_CASE("not uses fresh labels and falls through when the inner is false",
          "[codegen][bool]") {
  CompiledProgram program = compileSource(
      "#x #y : : if not ( eq ( #x #y ) ) then { } else { } ;");
  std::string x = declaredName(program, 0);
  std::string y = declaredName(program, 1);
  CodeGen gen("int");
  REQUIRE(gen.genBool(branchBoolOf(program), "T", "F") ==
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = " + y +
              ";\nif (v1 == v2) goto L1;\nL2 :\ngoto T;\nL1 :\n");
}

TEST_CASE("and skips to the false label when the first operand fails",
          "[codegen][bool]") {
  CompiledProgram program = compileSource(
      "#x #y : : if and ( eq ( #x #y ) eq ( #x 1 ) ) then { } else { } ;");
  std::string x = declaredName(program, 0);
  std::string y = declaredName(program, 1);
  CodeGen gen("int");
  REQUIRE(gen.genBool(branchBoolOf(program), "T", "F") ==
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = " + y +
              ";\nif (v1 == v2) goto L1;\nL2 :\ngoto F;\nL1 :\n"
              "int v3;\nint v4;\nv3 = " + x +
              ";\nv4 = 1;\nif (v3 == v4) goto T;\n");
}

TEST_CASE("nested and, or, not number variables and labels in order",
          "[codegen][bool]") {
  CompiledProgram program = compileSource(
      "#x #y : : if and ( or ( eq ( #x #y ) not ( lesser ( #x 1 ) ) ) "
      "larger ( #y 2 ) ) then { } else { } ;");
  std::string x = declaredName(program, 0);
  std::string y = declaredName(program, 1);
  CodeGen gen("int");
  REQUIRE(gen.genBool(branchBoolOf(program), "T", "F") ==
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = " + y +
              ";\nif (v1 == v2) goto L1;\n"
              "int v3;\nint v4;\nv3 = " + x +
              ";\nv4 = 1;\nif (v3 < v4) goto L3;\nL4 :\ngoto L1;\nL3 :\n"
              "L2 :\ngoto F;\nL1 :\n"
              "int v5;\nint v6;\nv5 = " + y +
              ";\nv6 = 2;\nif (v5 > v6) goto T;\n");
}

TEST_CASE("or gets its own label when its first operand contains an and",
          "[codegen][bool]") {
  CompiledProgram program = compileSource(
      "#x #y : : if or ( and ( eq ( #x #y ) eq ( #x 1 ) ) lesser ( #x 2 ) ) "
      "then { } else { } ;");
  std::string x = declaredName(program, 0);
  std::string y = declaredName(program, 1);
  CodeGen gen("int");
  REQUIRE(gen.genBool(branchBoolOf(program), "T", "F") ==
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = " + y +
              ";\nif (v1 == v2) goto L2;\nL3 :\ngoto L1;\nL2 :\n"
              "int v3;\nint v4;\nv3 = " + x +
              ";\nv4 = 1;\nif (v3 == v4) goto T;\nL1 :\n"
              "int v5;\nint v6;\nv5 = " + x +
              ";\nv6 = 2;\nif (v5 < v6) goto T;\n");
}

TEST_CASE("BRANCH matches the worked example", "[codegen][branch]") {
  CompiledProgram program =
      compileSource("#x : : if eq ( #x 3 ) then { nop ; } else { } ;");
  std::string x = declaredName(program, 0);
  CodeGen gen("int");
  REQUIRE(gen.genInstr(firstInstrOf(program)) ==
          "/* if */\n"
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = 3;\n"
          "if (v1 == v2) goto L1;\n"
          "L2 : /* else */\n"
          "/* end */;\n"
          "goto L3;\n"
          "L1 : /* then */\n"
          "/* nop */;\n"
          "/* end */;\n"
          "L3 : /* continue */");
}

TEST_CASE("a BRANCH inside a BRANCH body", "[codegen][branch]") {
  CompiledProgram program = compileSource(
      "#x : : if eq ( #x 1 ) then { if eq ( #x 2 ) then { nop ; } else { } ; "
      "} else { nop ; } ;");
  std::string x = declaredName(program, 0);
  CodeGen gen("int");
  REQUIRE(gen.genInstr(firstInstrOf(program)) ==
          "/* if */\n"
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = 1;\n"
          "if (v1 == v2) goto L1;\n"
          "L2 : /* else */\n"
          "/* nop */;\n"
          "/* end */;\n"
          "goto L3;\n"
          "L1 : /* then */\n"
          "/* if */\n"
          "int v3;\nint v4;\nv3 = " + x + ";\nv4 = 2;\n"
          "if (v3 == v4) goto L4;\n"
          "L5 : /* else */\n"
          "/* end */;\n"
          "goto L6;\n"
          "L4 : /* then */\n"
          "/* nop */;\n"
          "/* end */;\n"
          "L6 : /* continue */;\n"
          "/* end */;\n"
          "L3 : /* continue */");
}

TEST_CASE("temporaries and labels continue across instructions",
          "[codegen][branch]") {
  CompiledProgram program = compileSource(
      "#x : : if eq ( #x 1 ) then { } else { } ; "
      "if eq ( #x 2 ) then { } else { } ;");
  std::string x = declaredName(program, 0);
  CodeGen gen("int");
  REQUIRE(gen.genAlgo(algoOf(program)) ==
          "/* if */\n"
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = 1;\n"
          "if (v1 == v2) goto L1;\n"
          "L2 : /* else */\n/* end */;\ngoto L3;\n"
          "L1 : /* then */\n/* end */;\n"
          "L3 : /* continue */;\n"
          "/* if */\n"
          "int v3;\nint v4;\nv3 = " + x + ";\nv4 = 2;\n"
          "if (v3 == v4) goto L4;\n"
          "L5 : /* else */\n/* end */;\ngoto L6;\n"
          "L4 : /* then */\n/* end */;\n"
          "L6 : /* continue */;\n"
          "/* end */");
}

TEST_CASE("malformed BOOL and BRANCH nodes throw std::logic_error",
          "[codegen][bool]") {
  CodeGen gen("int");
  CompiledProgram program = compileSource("#x : : nop ;");

  REQUIRE_THROWS_AS(gen.genBool(nullptr, "T", "F"), std::logic_error);
  REQUIRE_THROWS_AS(gen.genBool(algoOf(program), "T", "F"), std::logic_error);

  Node emptyBool{0, "BOOL", {}};
  REQUIRE_THROWS_AS(gen.genBool(&emptyBool, "T", "F"), std::logic_error);

  Node dummy{1, "x", {}};
  Node unknownOp{2, "BOOL", {&dummy, &dummy, &dummy, &dummy, &dummy}};
  REQUIRE_THROWS_AS(gen.genBool(&unknownOp, "T", "F"), std::logic_error);

  Node badNot{3, "BOOL", {&dummy, &dummy, &dummy, &dummy}};
  REQUIRE_THROWS_AS(gen.genBool(&badNot, "T", "F"), std::logic_error);

  Node emptyBranch{4, "BRANCH", {}};
  Node instr{5, "INSTR", {&emptyBranch}};
  REQUIRE_THROWS_AS(gen.genInstr(&instr), std::logic_error);
}

TEST_CASE("a CALL inside a BOOL or a BRANCH body is deferred",
          "[codegen][bool]") {
  const std::string fn = "num #f ( ) { : : return ( 1 ) } : ";
  const std::string proc = "void #p ( ) { : : nop ; return } : ";
  CodeGen gen("int");

  CompiledProgram inBool = compileSource(
      "#x : " + fn + "if eq ( #f ( ) 1 ) then { } else { } ;");
  CompiledProgram inBody = compileSource(
      "#x : " + proc + "if eq ( #x 1 ) then { #p ( ) ; } else { } ;");
  CompiledProgram inElse = compileSource(
      "#x : " + proc + "if eq ( #x 1 ) then { } else { #p ( ) ; } ;");

  REQUIRE_THROWS_AS(gen.genInstr(firstInstrOf(inBool)), CodeGenUnsupported);
  REQUIRE_THROWS_AS(gen.genInstr(firstInstrOf(inBody)), CodeGenUnsupported);
  REQUIRE_THROWS_AS(gen.genInstr(firstInstrOf(inElse)), CodeGenUnsupported);
}

namespace {

// Wraps if/else around `condition`: prints `letter` when true, "-" when not.
std::string probe(const std::string &condition, const std::string &letter) {
  return "if " + condition + " then { print \"" + letter +
         "\" ; } else { print \"-\" ; } ; ";
}

std::string runSpl(const std::string &source) {
  if (findCxxCompiler().empty())
    SKIP("no C++ compiler (CXX, g++, clang++ or c++) available");

  CompiledProgram program = compileSource(source);
  CodeGen gen(determineNumType(program.root));
  std::string fragment = gen.genAlgo(algoOf(program));
  return compileAndRun(wrapInMain(program, gen.getNumType(), fragment));
}

const std::string T = "eq ( 1 1 )";
const std::string F = "eq ( 1 2 )";

} // namespace

TEST_CASE("compiled and/or/not follow their truth tables", "[codegen][run]") {
  std::string spl = ": : ";
  spl += probe("and ( " + T + " " + T + " )", "a");
  spl += probe("and ( " + T + " " + F + " )", "b");
  spl += probe("and ( " + F + " " + T + " )", "c");
  spl += probe("and ( " + F + " " + F + " )", "d");
  spl += probe("or ( " + T + " " + T + " )", "e");
  spl += probe("or ( " + T + " " + F + " )", "f");
  spl += probe("or ( " + F + " " + T + " )", "g");
  spl += probe("or ( " + F + " " + F + " )", "h");
  spl += probe("not ( " + T + " )", "i");
  spl += probe("not ( " + F + " )", "j");
  // or whose first operand is an and: the case a literal translation breaks
  spl += probe("or ( and ( " + F + " " + T + " ) " + T + " )", "k");
  spl += probe("and ( or ( " + F + " " + T + " ) not ( " + F + " ) )", "l");
  spl += probe("and ( or ( " + F + " " + F + " ) not ( " + F + " ) )", "m");
  spl += probe("not ( and ( " + T + " " + F + " ) )", "n");
  spl += probe("not ( or ( " + T + " " + F + " ) )", "o");
  spl += probe("and ( not ( " + T + " ) " + T + " )", "p");
  spl += probe("or ( not ( " + T + " ) not ( " + F + " ) )", "q");
  spl += probe("or ( or ( " + F + " and ( " + T + " " + T + " ) ) " + F + " )",
               "r");
  spl += probe("and ( and ( " + T + " " + T + " ) and ( " + T + " " + F + " ) )",
               "s");
  spl += probe("and ( and ( " + T + " " + T + " ) and ( " + T + " " + T + " ) )",
               "t");
  spl += probe("or ( and ( " + F + " " + F + " ) and ( " + T + " " + T + " ) )",
               "u");

  REQUIRE(runSpl(spl) == "a---efg--jkl-n--qr-tu");
}

TEST_CASE("compiled comparisons and arithmetic operands", "[codegen][run]") {
  std::string spl = "#x #y : : #x = 5 ; #y = 7 ; ";
  spl += probe("eq ( #x 5 )", "a");
  spl += probe("eq ( #x #y )", "b");
  spl += probe("larger ( #y #x )", "c");
  spl += probe("larger ( #x #y )", "d");
  spl += probe("larger ( #x 5 )", "e");
  spl += probe("lesser ( #x #y )", "f");
  spl += probe("lesser ( #y #x )", "g");
  spl += probe("lesser ( #x 5 )", "h");
  spl += probe("eq ( add ( #x 2 ) #y )", "i");
  spl += probe("larger ( sub ( #y 1 ) #x )", "j");
  spl += probe("lesser ( neg ( #x ) 0 )", "k");
  spl += probe("larger ( #x -3 )", "l");

  REQUIRE(runSpl(spl) == "a-c--f--ijkl");
}

TEST_CASE("compiled nested branches take the right arms", "[codegen][run]") {
  const std::string spl =
      "#x : : #x = 1 ; "
      "if eq ( #x 1 ) then { print \"a\" ; "
      "if larger ( #x 5 ) then { print \"b\" ; } else { print \"c\" ; "
      "if lesser ( #x 2 ) then { print \"d\" ; } else { print \"e\" ; } ; } ; "
      "print \"f\" ; } else { print \"g\" ; } ; "
      "if eq ( #x 2 ) then { print \"h\" ; } else { #x = 2 ; "
      "if eq ( #x 2 ) then { print \"i\" ; } else { print \"j\" ; } ; } ; "
      "print \"k\" ; "
      "if larger ( #x 0 ) then { print \"m\" ; } else { } ; ";

  REQUIRE(runSpl(spl) == "acdfikm");
}
