// The numeric type of the generated C++ comes from the Phase 2b Type Analyser:
// mod makes an integer program, anything else a float program. The run tests
// compile the complete genProgram text.
//
// Token syntax matches input.txt: every token is its own space-separated
// word, e.g. "( #x )" not "(#x)".

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>

#include "codegen/codeGen.h"
#include "test_helpers.h"

using Catch::Matchers::ContainsSubstring;

namespace {

std::string declaredName(const CompiledProgram &program) {
  return program.root->children[0]->children[0]->children[0]->symbol;
}

std::string generate(const CompiledProgram &program) {
  CodeGen gen(numTypeFromMode(program.numberMode));
  return gen.genProgram(program.root);
}

std::string runSpl(const std::string &source) {
  if (findCxxCompiler().empty())
    SKIP("no C++ compiler (CXX, g++, clang++ or c++) available");

  CompiledProgram program = compileSource(source);
  return compileAndRun(generate(program));
}

} // namespace

TEST_CASE("a program with mod declares int", "[codegen][numtype]") {
  CompiledProgram program =
      compileSource("#x : : #x = mod ( 7 2 ) ; print ( #x ) ;");
  std::string text = generate(program);

  REQUIRE(program.numberMode == NumberMode::INTEGER);
  REQUIRE_THAT(text, ContainsSubstring("{\nint " + declaredName(program) +
                                       ";\n \n"));
  REQUIRE_THAT(text, ContainsSubstring("int v1;\n"));
  REQUIRE(text.find("float") == std::string::npos);
}

TEST_CASE("a program without mod, div or decimals declares float",
          "[codegen][numtype]") {
  CompiledProgram program =
      compileSource("#x : : #x = add ( 2 3 ) ; print ( #x ) ;");
  std::string text = generate(program);

  REQUIRE(program.numberMode == NumberMode::FLOAT);
  REQUIRE_THAT(text, ContainsSubstring("{\nfloat " + declaredName(program) +
                                       ";\n \n"));
  REQUIRE_THAT(text, ContainsSubstring("float v1;\n"));
  REQUIRE(text.find("int v") == std::string::npos);
}

TEST_CASE("a no-mod program compiles and runs correctly",
          "[codegen][numtype][run]") {
  const std::string spl =
      "#x #y : : #x = add ( 2 3 ) ; #y = mul ( #x neg ( 4 ) ) ; "
      "print ( #x ) ; print \",\" ; print ( #y ) ; "
      "while lesser ( #x 7 ) do { #x = add ( #x 1 ) ; } ; "
      "print \",\" ; print ( #x ) ; ";

  REQUIRE(runSpl(spl) == "5,-20,7");
}

TEST_CASE("a program with div declares float and prints fractions",
          "[codegen][numtype][run]") {
  CompiledProgram program =
      compileSource("#x : : #x = div ( 7 2 ) ; print ( #x ) ;");
  REQUIRE_THAT(generate(program),
               ContainsSubstring("{\nfloat " + declaredName(program) + ";"));

  REQUIRE(runSpl("#x #y : : #x = div ( 7 2 ) ; #y = div ( #x 4 ) ; "
                 "print ( #x ) ; print \",\" ; print ( #y ) ;") == "3.5,0.875");
}

// Reference program, compiled with g++ and clang++ and run:
//   float a = 4000; float b = 4000; float p = a * b; std::cout << p;
// printed "1.6e+07" (std::cout default format, six significant digits).
TEST_CASE("a large product prints in the float format",
          "[codegen][numtype][run]") {
  REQUIRE(runSpl(": : print ( mul ( 4000 4000 ) ) ;") == "1.6e+07");
}

TEST_CASE("a large product in a mod program stays an integer",
          "[codegen][numtype][run]") {
  REQUIRE(runSpl("#x : : #x = mul ( 4000 4000 ) ; "
                 "print ( #x ) ; print \",\" ; print ( mod ( #x 7 ) ) ;") ==
          "16000000,2");
}
