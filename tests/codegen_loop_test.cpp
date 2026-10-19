// Step 4 of phase 3a: the four LOOP forms. Part A asserts the exact text,
// part B compiles and runs the generated code against output worked out by
// hand from the SPL semantics.
//
// Token syntax matches input.txt: every token is its own space-separated
// word, e.g. "( #x )" not "(#x)".

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <stdexcept>
#include <string>

#include "codegen/codeGen.h"
#include "test_helpers.h"

namespace {

Node *algoOf(const CompiledProgram &program) {
  return program.root->children[0]->children[4];
}

Node *firstInstrOf(const CompiledProgram &program) {
  return algoOf(program)->children[0];
}

std::string declaredName(const CompiledProgram &program, int index) {
  const Node *decl = program.root->children[0]->children[0];
  for (int i = 0; i < index; i++)
    decl = decl->children[1];
  return decl->children[0]->symbol;
}

// Declares two variables so every source can name #x.
std::string genInstrFor(const std::string &instr, std::string &x) {
  CompiledProgram program = compileSource("#x #y : : " + instr);
  x = declaredName(program, 0);
  CodeGen gen("int");
  return gen.genInstr(firstInstrOf(program));
}

std::string genAlgoFor(const std::string &algo, std::string &x) {
  CompiledProgram program = compileSource("#x #y : : " + algo);
  x = declaredName(program, 0);
  CodeGen gen("int");
  return gen.genAlgo(algoOf(program));
}

} // namespace

TEST_CASE("until loop matches the worked example", "[codegen][loop]") {
  std::string x;
  std::string text = genInstrFor("until eq ( #x 3 ) do { nop ; } ;", x);
  REQUIRE(text ==
          "L1 : /* until */\n"
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = 3;\n"
          "if (v1 == v2) goto L2;\n"
          "L3 : /* do */\n"
          "/* nop */;\n"
          "/* end */;\n"
          "goto L1;\n"
          "L2 :\n"
          "/* exit */");
}

TEST_CASE("do-while loop matches the worked example", "[codegen][loop]") {
  std::string x;
  std::string text = genInstrFor("do { nop ; } while eq ( #x 3 ) ;", x);
  REQUIRE(text ==
          "L1 : /* do */\n"
          "/* nop */;\n"
          "/* end */;\n"
          "/* while */\n"
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = 3;\n"
          "if (v1 == v2) goto L1;\n"
          "L2 : /* exit */");
}

// while = until with the condition negated, so genNot(B, exit) is used.
// Labels: start L1, exit L2, body L3, then genNot takes L4 (inner true) and
// L5 (inner false). Equal means "stay in the loop": L4 falls into the body at
// L3, and not-equal falls into L5, which jumps to the exit L2.
TEST_CASE("while loop negates the condition", "[codegen][loop]") {
  std::string x;
  std::string text = genInstrFor("while eq ( #x 3 ) do { nop ; } ;", x);
  REQUIRE(text ==
          "L1 : /* until */\n"
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = 3;\n"
          "if (v1 == v2) goto L4;\n"
          "L5 :\n"
          "goto L2;\n"
          "L4 :\n"
          "L3 : /* do */\n"
          "/* nop */;\n"
          "/* end */;\n"
          "goto L1;\n"
          "L2 :\n"
          "/* exit */");
}

// do-until = do-while with the condition negated, so genNot(B, body).
// Labels: body L1, exit L2, then genNot takes L3 (inner true) and L4 (inner
// false). Equal means "stop": L3 falls into the exit L2, and not-equal falls
// into L4, which jumps back to the body L1.
TEST_CASE("do-until loop negates the condition", "[codegen][loop]") {
  std::string x;
  std::string text = genInstrFor("do { nop ; } until eq ( #x 3 ) ;", x);
  REQUIRE(text ==
          "L1 : /* do */\n"
          "/* nop */;\n"
          "/* end */;\n"
          "/* while */\n"
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = 3;\n"
          "if (v1 == v2) goto L3;\n"
          "L4 :\n"
          "goto L1;\n"
          "L3 :\n"
          "L2 : /* exit */");
}

TEST_CASE("labels number in order across nested loops", "[codegen][loop]") {
  std::string x;
  std::string text = genInstrFor(
      "while eq ( #x 1 ) do { until eq ( #x 2 ) do { nop ; } ; } ;", x);
  std::string inner =
      "L6 : /* until */\n"
      "int v3;\nint v4;\nv3 = " + x + ";\nv4 = 2;\n"
      "if (v3 == v4) goto L7;\n"
      "L8 : /* do */\n"
      "/* nop */;\n"
      "/* end */;\n"
      "goto L6;\n"
      "L7 :\n"
      "/* exit */";
  REQUIRE(text ==
          "L1 : /* until */\n"
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = 1;\n"
          "if (v1 == v2) goto L4;\n"
          "L5 :\n"
          "goto L2;\n"
          "L4 :\n"
          "L3 : /* do */\n" +
              inner + ";\n/* end */;\n"
          "goto L1;\n"
          "L2 :\n"
          "/* exit */");
}

TEST_CASE("a loop inside a BRANCH", "[codegen][loop]") {
  std::string x;
  std::string text = genInstrFor(
      "if eq ( #x 1 ) then { do { nop ; } while eq ( #x 2 ) ; } else { } ;", x);
  REQUIRE(text ==
          "/* if */\n"
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = 1;\n"
          "if (v1 == v2) goto L1;\n"
          "L2 : /* else */\n"
          "/* end */;\n"
          "goto L3;\n"
          "L1 : /* then */\n"
          "L4 : /* do */\n"
          "/* nop */;\n"
          "/* end */;\n"
          "/* while */\n"
          "int v3;\nint v4;\nv3 = " + x + ";\nv4 = 2;\n"
          "if (v3 == v4) goto L4;\n"
          "L5 : /* exit */;\n"
          "/* end */;\n"
          "L3 : /* continue */");
}

TEST_CASE("a BRANCH inside a loop", "[codegen][loop]") {
  std::string x;
  std::string text = genInstrFor(
      "do { if eq ( #x 1 ) then { } else { } ; } until eq ( #x 2 ) ;", x);
  REQUIRE(text ==
          "L1 : /* do */\n"
          "/* if */\n"
          "int v3;\nint v4;\nv3 = " + x + ";\nv4 = 1;\n"
          "if (v3 == v4) goto L5;\n"
          "L6 : /* else */\n"
          "/* end */;\n"
          "goto L7;\n"
          "L5 : /* then */\n"
          "/* end */;\n"
          "L7 : /* continue */;\n"
          "/* end */;\n"
          "/* while */\n"
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = 2;\n"
          "if (v1 == v2) goto L3;\n"
          "L4 :\n"
          "goto L1;\n"
          "L3 :\n"
          "L2 : /* exit */");
}

TEST_CASE("loops in a sequence continue the numbering", "[codegen][loop]") {
  std::string x;
  std::string text = genAlgoFor(
      "do { } while eq ( #x 1 ) ; until eq ( #x 2 ) do { } ;", x);
  REQUIRE(text ==
          "L1 : /* do */\n"
          "/* end */;\n"
          "/* while */\n"
          "int v1;\nint v2;\nv1 = " + x + ";\nv2 = 1;\n"
          "if (v1 == v2) goto L1;\n"
          "L2 : /* exit */;\n"
          "L3 : /* until */\n"
          "int v3;\nint v4;\nv3 = " + x + ";\nv4 = 2;\n"
          "if (v3 == v4) goto L4;\n"
          "L5 : /* do */\n"
          "/* end */;\n"
          "goto L3;\n"
          "L4 :\n"
          "/* exit */;\n"
          "/* end */");
}

TEST_CASE("malformed LOOP nodes throw std::logic_error", "[codegen][loop]") {
  CodeGen gen("int");

  Node emptyLoop{0, "LOOP", {}};
  Node emptyInstr{1, "INSTR", {&emptyLoop}};
  REQUIRE_THROWS_AS(gen.genInstr(&emptyInstr), std::logic_error);

  Node dummy{2, "x", {}};
  Node noDo{3, "LOOP", {&dummy, &dummy, &dummy, &dummy, &dummy, &dummy}};
  REQUIRE_THROWS_AS(gen.genInstr(&noDo), std::logic_error);

  Node unknownKind{4, "foo", {}};
  Node badCond{5, "COND", {&unknownKind}};
  Node doLeaf{6, "do", {}};
  Node open{7, "{", {}};
  Node close{8, "}", {}};
  Node condFirst{
      9, "LOOP", {&badCond, &dummy, &doLeaf, &open, &dummy, &close}};
  REQUIRE_THROWS_AS(gen.genInstr(&condFirst), std::logic_error);

  Node condLast{
      10, "LOOP", {&doLeaf, &open, &dummy, &close, &badCond, &dummy}};
  REQUIRE_THROWS_AS(gen.genInstr(&condLast), std::logic_error);

  REQUIRE_THROWS_AS(gen.genInstr(nullptr), std::logic_error);
}

TEST_CASE("a CALL inside a loop condition or body is inlined",
          "[codegen][loop]") {
  const std::string fn = "num #f ( ) { : : return ( 1 ) } : ";
  const std::string proc = "void #p ( ) { : : nop ; return } : ";
  CodeGen gen("int");

  CompiledProgram inCondition = compileSource(
      "#x : " + fn + "while eq ( #f ( ) 1 ) do { } ;");
  CompiledProgram inBody = compileSource(
      "#x : " + proc + "do { #p ( ) ; } until eq ( #x 1 ) ;");

  CodeGen condGen("int");
  CodeGen bodyGen("int");
  REQUIRE(condGen.genProgram(inCondition.root).find("/* until */") !=
          std::string::npos);
  REQUIRE(bodyGen.genProgram(inBody.root).find("/* do */") !=
          std::string::npos);
  REQUIRE_THROWS_AS(gen.genInstr(firstInstrOf(inBody)), std::logic_error);
}

namespace {

std::string runSpl(const std::string &source) {
  if (findCxxCompiler().empty())
    SKIP("no C++ compiler (CXX, g++, clang++ or c++) available");

  CompiledProgram program = compileSource(source);
  CodeGen gen(numTypeFromMode(program.numberMode));
  std::string fragment = gen.genAlgo(algoOf(program));
  return compileAndRun(wrapInMain(program, gen.getNumType(), fragment));
}

} // namespace

TEST_CASE("compiled counting loops in all four forms", "[codegen][run]") {
  const std::string body = "{ print ( #x ) ; #x = add ( #x 1 ) ; } ; ";
  const std::string spl =
      "#x : : "
      "#x = 0 ; while lesser ( #x 3 ) do " + body + "print \",\" ; "
      "#x = 0 ; until eq ( #x 3 ) do " + body + "print \",\" ; "
      "#x = 0 ; do { print ( #x ) ; #x = add ( #x 1 ) ; } while lesser ( #x 3 ) ; "
      "print \",\" ; "
      "#x = 0 ; do { print ( #x ) ; #x = add ( #x 1 ) ; } until eq ( #x 3 ) ; "
      "print \",\" ; ";

  REQUIRE(runSpl(spl) == "012,012,012,012,");
}

TEST_CASE("compiled loops with conditions that start false or true",
          "[codegen][run]") {
  const std::string spl =
      "#x : : "
      "#x = 5 ; while lesser ( #x 3 ) do { print \"a\" ; } ; print \",\" ; "
      "#x = 5 ; until larger ( #x 3 ) do { print \"a\" ; } ; print \",\" ; "
      "#x = 5 ; do { print \"b\" ; #x = add ( #x 1 ) ; } "
      "while lesser ( #x 3 ) ; print \",\" ; "
      "#x = 5 ; do { print \"c\" ; } until larger ( #x 3 ) ; print \",\" ; ";

  REQUIRE(runSpl(spl) == ",,b,c,");
}

TEST_CASE("compiled loop conditions using and, or and not",
          "[codegen][run]") {
  const std::string step = "print ( #x ) ; #x = add ( #x 1 ) ; ";
  const std::string spl =
      "#x #y : : "
      "#x = 0 ; #y = 10 ; "
      "while and ( lesser ( #x 3 ) larger ( #y 8 ) ) do "
      "{ print ( #x ) ; #x = add ( #x 1 ) ; #y = sub ( #y 1 ) ; } ; "
      "print \",\" ; "
      "#x = 0 ; until or ( eq ( #x 2 ) eq ( #x 4 ) ) do { " + step + "} ; "
      "print \",\" ; "
      "#x = 0 ; while not ( eq ( #x 2 ) ) do { " + step + "} ; "
      "print \",\" ; "
      "#x = 0 ; do { " + step + "} "
      "while or ( and ( lesser ( #x 2 ) larger ( #x -1 ) ) eq ( #x 3 ) ) ; "
      "print \",\" ; "
      "#x = 0 ; do { " + step + "} "
      "until and ( larger ( #x 1 ) not ( eq ( #x 5 ) ) ) ; print \",\" ; ";

  REQUIRE(runSpl(spl) == "01,01,01,01,01,");
}

TEST_CASE("compiled nested loops and loops mixed with branches",
          "[codegen][run]") {
  const std::string spl =
      "#x #y : : "
      // x=0: no s, x=1: one, x=2: two
      "#x = 0 ; while lesser ( #x 3 ) do { #y = 0 ; "
      "while lesser ( #y #x ) do { print \"s\" ; #y = add ( #y 1 ) ; } ; "
      "print \",\" ; #x = add ( #x 1 ) ; } ; "
      // a branch in the body of a loop: even/odd for x = 0..5
      "#x = 0 ; while lesser ( #x 6 ) do { "
      "if eq ( mod ( #x 2 ) 0 ) then { print \"e\" ; } else { print \"o\" ; } ; "
      "#x = add ( #x 1 ) ; } ; "
      // a loop in a branch: taken for x = 3 (counts down), skipped for x = 0
      "#x = 3 ; if larger ( #x 2 ) then { while larger ( #x 0 ) do "
      "{ print ( #x ) ; #x = sub ( #x 1 ) ; } ; } else { print \"n\" ; } ; "
      "#x = 0 ; if larger ( #x 2 ) then { while larger ( #x 0 ) do "
      "{ print ( #x ) ; #x = sub ( #x 1 ) ; } ; } else { print \"n\" ; } ; "
      // nested do loops: two outer rounds of two inner steps
      "#x = 0 ; do { #y = 0 ; do { print \"i\" ; #y = add ( #y 1 ) ; } "
      "until eq ( #y 2 ) ; print \":\" ; #x = add ( #x 1 ) ; } "
      "while lesser ( #x 2 ) ; ";

  REQUIRE(runSpl(spl) == ",s,ss,eoeoeo321nii:ii:");
}

TEST_CASE("an infinite generated loop fails instead of hanging",
          "[codegen][run][timeout]") {
  if (findCxxCompiler().empty())
    SKIP("no C++ compiler (CXX, g++, clang++ or c++) available");

  CompiledProgram program = compileSource(": : while eq ( 1 1 ) do { } ;");
  CodeGen gen("int");
  std::string source =
      wrapInMain(program, "int", gen.genAlgo(algoOf(program)));
  REQUIRE_THROWS_WITH(compileAndRun(source),
                      Catch::Matchers::ContainsSubstring("did not finish"));
}
