// Step 6 of phase 3b: the function registry, the code-mine text and the
// hoisted declarations. CALL is not implemented yet, so no program here
// calls anything.
//
// Token syntax matches input.txt: every token is its own space-separated
// word, e.g. "( #x )" not "(#x)".

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <stdexcept>
#include <string>
#include <vector>

#include "codegen/codeGen.h"
#include "test_helpers.h"

using Catch::Matchers::ContainsSubstring;

namespace {

void collectFTypes(Node *node, std::vector<Node *> &found) {
  if (node->symbol == "F_TYPE")
    found.push_back(node);
  for (Node *child : node->children)
    collectFTypes(child, found);
}

// pre-order, so an enclosing function comes before the ones nested in it
std::vector<Node *> fTypesOf(const CompiledProgram &program) {
  std::vector<Node *> found;
  collectFTypes(program.root, found);
  return found;
}

std::string functionName(Node *fType) { return fType->children[1]->symbol; }

std::vector<std::string> namesIn(const Node *decl) {
  std::vector<std::string> names;
  while (!decl->children.empty()) {
    names.push_back(decl->children[0]->symbol);
    decl = decl->children[1];
  }
  return names;
}

std::vector<std::string> mainVars(const CompiledProgram &program) {
  return namesIn(program.root->children[0]->children[0]);
}

std::vector<std::string> paramsOf(Node *fType) {
  return namesIn(fType->children[3]);
}

std::vector<std::string> localsOf(Node *fType) {
  return namesIn(fType->children[6]->children[0]);
}

std::string generate(const CompiledProgram &program,
                     const std::string &numType = "int") {
  CodeGen gen(numType);
  return gen.genProgram(program.root);
}

const std::string MARK_BEGIN =
    "/* begin of the non-executable code-mine for later inlining */\n";
const std::string MARK_END_AND_MAIN =
    "\n/* end of the non-executable code-mine */\n"
    "/* begin of executable C code */\n"
    "#include <iostream>\n"
    "int main()\n"
    "{\n";
const std::string TAIL = "\nreturn 0; } /* end of executable C */";

std::size_t count(const std::string &text, const std::string &part) {
  std::size_t total = 0;
  for (std::size_t at = text.find(part); at != std::string::npos;
       at = text.find(part, at + part.size()))
    total++;
  return total;
}

} // namespace

TEST_CASE("a void function with no parameters or locals",
          "[codegen][function]") {
  CompiledProgram program =
      compileSource("#x : void #p ( ) { : : nop ; return } : print \"hi\" ;");
  std::string x = mainVars(program)[0];
  std::string p = functionName(fTypesOf(program)[0]);

  REQUIRE(generate(program) ==
          MARK_BEGIN +
              "#if 0\n"
              "begin of code-mine for void-procedure " + p + "\n"
              "Parameters for inlining:\n"
              " \n"
              "Body of the procedure:\n"
              " \n"
              "/* nop */;\n"
              "/* end */\n"
              "end of code-mine for void-procedure " + p + "\n"
              "#endif\n" +
              MARK_END_AND_MAIN +
              "int " + x + ";\n"
              " \n"
              "std::cout << \"hi\";\n"
              "/* end */" + TAIL);
}

TEST_CASE("a num function with parameters, a local and a return term",
          "[codegen][function]") {
  CompiledProgram program = compileSource(
      "#x : num #f ( #a #b ) { #r : : #r = add ( #a #b ) ; "
      "return ( mul ( #r 2 ) ) } : print \"k\" ;");
  Node *f = fTypesOf(program)[0];
  std::string x = mainVars(program)[0];
  std::string name = functionName(f);
  std::string a = paramsOf(f)[0];
  std::string b = paramsOf(f)[1];
  std::string r = localsOf(f)[0];
  std::string result = name + "v1";

  REQUIRE(generate(program) ==
          MARK_BEGIN +
              "#if 0\n"
              "begin of code-mine for Function " + name + "\n"
              "Parameters for inlining:\n"
              "int " + a + ";\nint " + b + ";\n \n"
              "Body of the procedure:\n"
              "int " + r + ";\n \n"
              "int v2;\nint v3;\nint v4;\n"
              "v3 = " + a + ";\nv4 = " + b + ";\nv2 = v3 + v4;\n" +
              r + " = v2;\n"
              "/* end */;\n"
              "int v5;\nint v6;\nv5 = " + r + ";\nv6 = 2;\n" +
              result + " = v5 * v6;\n"
              "\n"
              "end of code-mine for Function " + name + "\n"
              "The result is found in the return value " + result + "\n"
              "#endif\n" +
              MARK_END_AND_MAIN +
              "int " + x + ";\n \n"
              "int " + a + ";\nint " + b + ";\nint " + r + ";\nint " +
              result + ";\n"
              "std::cout << \"k\";\n"
              "/* end */" + TAIL);
}

TEST_CASE("sibling functions keep program order", "[codegen][function]") {
  CompiledProgram program = compileSource(
      ": num #f ( #a ) { : : return ( #a ) } "
      "num #g ( #b ) { : : return ( #b ) } : nop ;");
  std::vector<Node *> fs = fTypesOf(program);
  std::string text = generate(program);
  std::string f = functionName(fs[0]);
  std::string g = functionName(fs[1]);

  REQUIRE(text.find("code-mine for Function " + f) <
          text.find("code-mine for Function " + g));
  REQUIRE_THAT(text, ContainsSubstring("{\n \nint " + paramsOf(fs[0])[0] +
                                       ";\nint " + f + "v1;\nint " +
                                       paramsOf(fs[1])[0] + ";\nint " + g +
                                       "v2;\n/* nop */"));
}

TEST_CASE("a nested function is rendered and declared before its parent",
          "[codegen][function]") {
  CompiledProgram program = compileSource(
      "#x : num #outer ( #a ) { #l : num #inner ( #b ) { : : return ( #b ) } "
      ": #l = 1 ; return ( #l ) } : nop ;");
  std::vector<Node *> fs = fTypesOf(program);
  Node *outer = fs[0];
  Node *inner = fs[1];
  std::string text = generate(program);
  std::string outerName = functionName(outer);
  std::string innerName = functionName(inner);

  REQUIRE(text.find("begin of code-mine for Function " + innerName) <
          text.find("begin of code-mine for Function " + outerName));
  REQUIRE_THAT(
      text, ContainsSubstring(
                "The result is found in the return value " + innerName + "v1"));
  REQUIRE_THAT(
      text, ContainsSubstring(
                "The result is found in the return value " + outerName + "v2"));
  REQUIRE_THAT(text, ContainsSubstring(
                         "int " + paramsOf(inner)[0] + ";\nint " + innerName +
                         "v1;\nint " + paramsOf(outer)[0] + ";\nint " +
                         localsOf(outer)[0] + ";\nint " + outerName +
                         "v2;\n/* nop */"));
}

TEST_CASE("a program without functions keeps the earlier format",
          "[codegen][function]") {
  CompiledProgram program =
      compileSource("#x #y : : nop ; print \"hi\" ;");
  std::string x = mainVars(program)[0];
  std::string y = mainVars(program)[1];

  REQUIRE(generate(program) ==
          MARK_BEGIN + " " + MARK_END_AND_MAIN + "int " + x + ";\nint " + y +
              ";\n \n/* nop */;\nstd::cout << \"hi\";\n/* end */" + TAIL);
}

TEST_CASE("a mine body with comments, a branch and a loop stays inert",
          "[codegen][function]") {
  CompiledProgram program = compileSource(
      "#x : void #p ( ) { #i : : comment \"c\" ; #i = 0 ; "
      "while lesser ( #i 2 ) do { #i = add ( #i 1 ) ; } ; "
      "if eq ( #i 2 ) then { nop ; } else { } ; return } : nop ;");
  std::string text = generate(program);

  std::size_t open = text.find("#if 0\n");
  std::size_t close = text.find("#endif\n");
  REQUIRE(count(text, "#if 0") == 1);
  REQUIRE(count(text, "#endif") == 1);
  REQUIRE(open < close);

  std::string mine = text.substr(open, close - open);
  REQUIRE_THAT(mine, ContainsSubstring("/* \"c\" */"));
  REQUIRE_THAT(mine, ContainsSubstring("/* else */"));
  REQUIRE_THAT(mine, ContainsSubstring("/* then */"));
  REQUIRE_THAT(mine, ContainsSubstring("/* exit */"));
  REQUIRE(text.find("/* else */", close) == std::string::npos);
}

TEST_CASE("float programs declare hoisted variables as float",
          "[codegen][function]") {
  CompiledProgram program = compileSource(
      "#x : num #h ( #a ) { : : return ( div ( #a 2 ) ) } : #x = 0.5 ;");
  Node *h = fTypesOf(program)[0];
  REQUIRE_THAT(generate(program, "float"),
               ContainsSubstring("float " + paramsOf(h)[0] + ";\nfloat " +
                                 functionName(h) + "v1;\n"));
}

TEST_CASE("a CALL in main or in a function body is still deferred",
          "[codegen][function]") {
  CompiledProgram inMain = compileSource(
      "#x : num #f ( ) { : : return ( 1 ) } : #x = #f ( ) ;");
  CompiledProgram inBody = compileSource(
      "#x : num #p ( ) { #r : num #g ( ) { : : return ( 1 ) } : "
      "#r = #g ( ) ; return ( #r ) } : nop ;");

  REQUIRE_THROWS_AS(generate(inMain), CodeGenUnsupported);
  REQUIRE_THROWS_AS(generate(inBody), CodeGenUnsupported);
}

TEST_CASE("malformed function nodes throw std::logic_error",
          "[codegen][function]") {
  const std::string source =
      ": num #f ( #a ) { : : return ( #a ) } "
      "void #p ( ) { : : nop ; return } : nop ;";

  SECTION("a punctuation leaf has the wrong symbol") {
    CompiledProgram program = compileSource(source);
    fTypesOf(program)[0]->children[2]->symbol = "x";
    REQUIRE_THROWS_AS(generate(program), std::logic_error);
  }
  SECTION("the wrong number of children") {
    CompiledProgram program = compileSource(source);
    fTypesOf(program)[1]->children.pop_back();
    REQUIRE_THROWS_AS(generate(program), std::logic_error);
  }
  SECTION("an unknown return kind") {
    CompiledProgram program = compileSource(source);
    fTypesOf(program)[0]->children[0]->symbol = "string";
    REQUIRE_THROWS_AS(generate(program), std::logic_error);
  }
  SECTION("the body is not a P") {
    CompiledProgram program = compileSource(source);
    fTypesOf(program)[0]->children[6]->symbol = "ALGO";
    REQUIRE_THROWS_AS(generate(program), std::logic_error);
  }
  SECTION("a name that is not a leaf") {
    CompiledProgram program = compileSource(source);
    Node *f = fTypesOf(program)[0];
    f->children[1]->children.push_back(f->children[2]);
    REQUIRE_THROWS_AS(generate(program), std::logic_error);
  }
  SECTION("two functions with the same name") {
    CompiledProgram program = compileSource(source);
    std::vector<Node *> fs = fTypesOf(program);
    fs[1]->children[1]->symbol = fs[0]->children[1]->symbol;
    REQUIRE_THROWS_AS(generate(program), std::logic_error);
  }
  SECTION("a broken F_DECL") {
    CompiledProgram program = compileSource(source);
    program.root->children[0]->children[2]->children.pop_back();
    REQUIRE_THROWS_AS(generate(program), std::logic_error);
  }
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

TEST_CASE("compiled programs with uncalled functions run unchanged",
          "[codegen][function][run]") {
  const std::string spl =
      "#x : void #p ( ) { #i : : comment \"hi\" ; #i = 0 ; "
      "while lesser ( #i 3 ) do { print ( #i ) ; #i = add ( #i 1 ) ; } ; "
      "if eq ( #i 3 ) then { print \"a\" ; } else { print \"b\" ; } ; return } "
      "num #f ( #a #b ) { #r : : #r = add ( #a #b ) ; "
      "return ( mul ( #r 2 ) ) } : "
      "#x = 5 ; print ( #x ) ; print \",\" ; print \"done\" ; ";

  REQUIRE(runProgram(spl) == "5,done");
}

TEST_CASE("compiled programs with nested uncalled functions run unchanged",
          "[codegen][function][run]") {
  const std::string spl =
      "#x : num #outer ( #a ) { #l : num #inner ( #b ) { : : return ( #b ) } "
      ": #l = 1 ; do { #l = add ( #l 1 ) ; } until larger ( #l 3 ) ; "
      "return ( #l ) } : #x = 7 ; print ( #x ) ; ";

  REQUIRE(runProgram(spl) == "7");
}

TEST_CASE("compiled float program with an uncalled function",
          "[codegen][function][run]") {
  const std::string spl =
      "#x : num #h ( #a ) { : : return ( div ( #a 2 ) ) } : "
      "#x = 0.5 ; print ( #x ) ; ";

  REQUIRE(runProgram(spl) == "0.5");
}
