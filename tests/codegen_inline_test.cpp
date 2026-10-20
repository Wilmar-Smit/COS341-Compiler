// Step 7 of phase 3b: CALL inlining. Part A asserts the exact text and the
// structure of the inlined code, part B compiles the complete genProgram text
// and runs it against output worked out by hand from the SPL semantics.
//
// Token syntax matches input.txt: every token is its own space-separated
// word, e.g. "( #x )" not "(#x)". Programs without mod are float programs,
// so Part A expects float declarations.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "codegen/codeGen.h"
#include "test_helpers.h"

using Catch::Matchers::ContainsSubstring;

namespace {

void collect(Node *node, const std::string &symbol, std::vector<Node *> &found) {
  if (node->symbol == symbol)
    found.push_back(node);
  for (Node *child : node->children)
    collect(child, symbol, found);
}

// pre-order, so an enclosing function or an earlier call comes first
std::vector<Node *> nodesNamed(const CompiledProgram &program,
                               const std::string &symbol) {
  std::vector<Node *> found;
  collect(program.root, symbol, found);
  return found;
}

std::vector<std::string> namesIn(const Node *decl) {
  std::vector<std::string> names;
  while (!decl->children.empty()) {
    names.push_back(decl->children[0]->symbol);
    decl = decl->children[1];
  }
  return names;
}

std::string functionName(const CompiledProgram &program, std::size_t index) {
  return nodesNamed(program, "F_TYPE")[index]->children[1]->symbol;
}

std::string mainVar(const CompiledProgram &program, std::size_t index) {
  return namesIn(program.root->children[0]->children[0])[index];
}

std::string generate(const CompiledProgram &program) {
  CodeGen gen(numTypeFromMode(program.numberMode));
  return gen.genProgram(program.root);
}

const std::string MAIN_OPEN = "int main()\n{\n";
const std::string TAIL = "\nreturn 0; } /* end of executable C */";

// everything generated for main(), starting at its first declaration
std::string mainPart(const std::string &text) {
  return text.substr(text.find(MAIN_OPEN) + MAIN_OPEN.size());
}

// the mine text of one function, from its begin line to its end line
std::string mineOf(const std::string &text, const std::string &name) {
  std::size_t begin = text.find("begin of code-mine for ");
  while (begin != std::string::npos) {
    std::size_t lineEnd = text.find('\n', begin);
    if (text.substr(begin, lineEnd - begin).ends_with(" " + name)) {
      std::string endLine = "end of code-mine for ";
      std::size_t end = text.find(endLine, begin);
      return text.substr(begin, end - begin);
    }
    begin = text.find("begin of code-mine for ", lineEnd);
  }
  return "";
}

// every line of the form "<label> :" or "<label> : /* ... */"
std::vector<std::string> labelsIn(const std::string &text) {
  std::vector<std::string> labels;
  std::size_t start = 0;
  while (start < text.size()) {
    std::size_t end = text.find('\n', start);
    if (end == std::string::npos)
      end = text.size();
    std::string line = text.substr(start, end - start);
    std::size_t space = line.find(" :");
    if (line.size() > 1 && line[0] == 'L' && space != std::string::npos &&
        line.find_first_not_of("0123456789", 1) == space)
      labels.push_back(line.substr(0, space));
    start = end + 1;
  }
  return labels;
}

std::size_t count(const std::string &text, const std::string &part) {
  std::size_t total = 0;
  for (std::size_t at = text.find(part); at != std::string::npos;
       at = text.find(part, at + part.size()))
    total++;
  return total;
}

void requireUniqueLabels(const std::string &text, std::size_t expected) {
  std::vector<std::string> labels = labelsIn(text);
  std::set<std::string> distinct(labels.begin(), labels.end());
  REQUIRE(labels.size() == expected);
  REQUIRE(distinct.size() == labels.size());
}

} // namespace

TEST_CASE("a void procedure without arguments is inlined into main",
          "[codegen][inline]") {
  CompiledProgram program = compileSource(
      "#x : void #p ( ) { : : print \"hi\" ; return } : #p ( ) ; print \"k\" ;");
  std::string x = mainVar(program, 0);

  REQUIRE(mainPart(generate(program)) ==
          "float " + x + ";\n"
          " \n"
          "std::cout << \"hi\";\n"
          "/* end */;\n"
          "std::cout << \"k\";\n"
          "/* end */" + TAIL);
}

TEST_CASE("a num function with two arguments assigned in main",
          "[codegen][inline]") {
  CompiledProgram program = compileSource(
      "#x : num #f ( #a #b ) { #r : : #r = add ( #a #b ) ; "
      "return ( mul ( #r 2 ) ) } : #x = #f ( 3 4 ) ; print ( #x ) ;");
  Node *f = nodesNamed(program, "F_TYPE")[0];
  std::string x = mainVar(program, 0);
  std::string name = functionName(program, 0);
  std::string a = namesIn(f->children[3])[0];
  std::string b = namesIn(f->children[3])[1];
  std::string r = namesIn(f->children[6]->children[0])[0];
  std::string result = name + "v1";

  // v1 is the result variable and v2..v6 belong to the mine
  REQUIRE(mainPart(generate(program)) ==
          "float " + x + ";\n \n"
          "float " + a + ";\nfloat " + b + ";\nfloat " + r + ";\nfloat " +
              result + ";\n"
          "float v7;\nfloat v8;\nfloat v9;\n"
          "v8 = 3;\nv9 = 4;\n" +
          a + " = v8;\n" + b + " = v9;\n"
          "float v10;\nfloat v11;\nfloat v12;\n"
          "v11 = " + a + ";\nv12 = " + b + ";\nv10 = v11 + v12;\n" + r +
              " = v10;\n"
          "/* end */;\n"
          "float v13;\nfloat v14;\nv13 = " + r + ";\nv14 = 2;\n" + result +
              " = v13 * v14;\n"
          "v7 = " + result + ";\n" +
          x + " = v7;\n"
          "float v15;\nv15 = " + x + ";\nstd::cout << v15;\n"
          "/* end */" + TAIL);
}

TEST_CASE("arguments are assigned to the parameters in declaration order",
          "[codegen][inline]") {
  CompiledProgram program = compileSource(
      "#x : num #d ( #a #b ) { : : return ( sub ( #a #b ) ) } : "
      "#x = #d ( 5 3 ) ;");
  Node *d = nodesNamed(program, "F_TYPE")[0];
  std::string a = namesIn(d->children[3])[0];
  std::string b = namesIn(d->children[3])[1];

  REQUIRE_THAT(generate(program),
               ContainsSubstring("v5 = 5;\nv6 = 3;\n" + a + " = v5;\n" + b +
                                 " = v6;\n"));
}

TEST_CASE("a call as the operand of print", "[codegen][inline]") {
  CompiledProgram program = compileSource(
      "#x : num #f ( #a ) { : : return ( add ( #a 1 ) ) } : "
      "print ( #f ( 5 ) ) ;");
  std::string x = mainVar(program, 0);
  std::string a = namesIn(nodesNamed(program, "F_TYPE")[0]->children[3])[0];
  std::string result = functionName(program, 0) + "v1";

  REQUIRE(mainPart(generate(program)) ==
          "float " + x + ";\n \n"
          "float " + a + ";\nfloat " + result + ";\n"
          "float v4;\nfloat v5;\nv5 = 5;\n" + a + " = v5;\n"
          "/* end */;\n"
          "float v6;\nfloat v7;\nv6 = " + a + ";\nv7 = 1;\n" + result +
              " = v6 + v7;\n"
          "v4 = " + result + ";\n"
          "std::cout << v4;\n"
          "/* end */" + TAIL);
}

// The type checker rejects a num call as an instruction, so the tree is
// rewritten after checking: the INSTR is made to hold the CALL directly.
TEST_CASE("a num call as an instruction discards the value",
          "[codegen][inline]") {
  CompiledProgram program = compileSource(
      "#x : num #f ( #a ) { : : return ( #a ) } : #x = #f ( 5 ) ;");
  Node *instr = program.root->children[0]->children[4]->children[0];
  instr->children = {nodesNamed(program, "CALL")[0]};
  std::string x = mainVar(program, 0);
  std::string a = namesIn(nodesNamed(program, "F_TYPE")[0]->children[3])[0];
  std::string result = functionName(program, 0) + "v1";

  REQUIRE(mainPart(generate(program)) ==
          "float " + x + ";\n \n"
          "float " + a + ";\nfloat " + result + ";\n"
          "float v2;\nv2 = 5;\n" + a + " = v2;\n"
          "/* end */;\n" + result + " = " + a + ";\n"
          "/* end */" + TAIL);
}

TEST_CASE("a call inside the argument of a call", "[codegen][inline]") {
  CompiledProgram program = compileSource(
      "#x : num #g ( #b ) { : : return ( mul ( #b 2 ) ) } "
      "num #f ( #a ) { : : return ( add ( #a 1 ) ) } : #x = #f ( #g ( 3 ) ) ;");
  std::string x = mainVar(program, 0);
  std::string b = namesIn(nodesNamed(program, "F_TYPE")[0]->children[3])[0];
  std::string a = namesIn(nodesNamed(program, "F_TYPE")[1]->children[3])[0];
  std::string gr = functionName(program, 0) + "v1";
  std::string fr = functionName(program, 1) + "v4";

  // g is rendered first (v1..v3), then f (v4..v6); main starts at v7
  REQUIRE(mainPart(generate(program)) ==
          "float " + x + ";\n \n"
          "float " + b + ";\nfloat " + gr + ";\nfloat " + a + ";\nfloat " +
              fr + ";\n"
          "float v7;\nfloat v8;\nfloat v9;\n"
          "v9 = 3;\n" + b + " = v9;\n"
          "/* end */;\n"
          "float v10;\nfloat v11;\nv10 = " + b + ";\nv11 = 2;\n" + gr +
              " = v10 * v11;\n"
          "v8 = " + gr + ";\n" + a + " = v8;\n"
          "/* end */;\n"
          "float v12;\nfloat v13;\nv12 = " + a + ";\nv13 = 1;\n" + fr +
              " = v12 + v13;\n"
          "v7 = " + fr + ";\n" + x + " = v7;\n"
          "/* end */" + TAIL);
}

TEST_CASE("calls in both operands of a binary term", "[codegen][inline]") {
  CompiledProgram program = compileSource(
      "#x : num #f ( #a ) { : : return ( #a ) } : "
      "#x = add ( #f ( 1 ) #f ( 2 ) ) ;");
  std::string x = mainVar(program, 0);
  std::string a = namesIn(nodesNamed(program, "F_TYPE")[0]->children[3])[0];
  std::string fr = functionName(program, 0) + "v1";

  REQUIRE(mainPart(generate(program)) ==
          "float " + x + ";\n \n"
          "float " + a + ";\nfloat " + fr + ";\n"
          "float v2;\nfloat v3;\nfloat v4;\n"
          "float v5;\nv5 = 1;\n" + a + " = v5;\n"
          "/* end */;\n" + fr + " = " + a + ";\n"
          "v3 = " + fr + ";\n"
          "float v6;\nv6 = 2;\n" + a + " = v6;\n"
          "/* end */;\n" + fr + " = " + a + ";\n"
          "v4 = " + fr + ";\n"
          "v2 = v3 + v4;\n" + x + " = v2;\n"
          "/* end */" + TAIL);
}

TEST_CASE("a branch function called twice gets fresh labels each time",
          "[codegen][inline]") {
  CompiledProgram program = compileSource(
      "#x : num #f ( #a ) { : : if eq ( #a 0 ) then { } else { } ; "
      "return ( #a ) } : #x = #f ( 0 ) ; #x = #f ( 1 ) ;");
  std::string text = generate(program);
  std::string x = mainVar(program, 0);
  std::string a = namesIn(nodesNamed(program, "F_TYPE")[0]->children[3])[0];
  std::string fr = functionName(program, 0) + "v1";

  requireUniqueLabels(text, 9);
  REQUIRE(mainPart(text) ==
          "float " + x + ";\n \n"
          "float " + a + ";\nfloat " + fr + ";\n"
          "float v4;\nfloat v5;\nv5 = 0;\n" + a + " = v5;\n"
          "/* if */\n"
          "float v6;\nfloat v7;\nv6 = " + a + ";\nv7 = 0;\n"
          "if (v6 == v7) goto L4;\n"
          "L5 : /* else */\n"
          "/* end */;\n"
          "goto L6;\n"
          "L4 : /* then */\n"
          "/* end */;\n"
          "L6 : /* continue */;\n"
          "/* end */;\n" + fr + " = " + a + ";\n"
          "v4 = " + fr + ";\n" + x + " = v4;\n"
          "float v8;\nfloat v9;\nv9 = 1;\n" + a + " = v9;\n"
          "/* if */\n"
          "float v10;\nfloat v11;\nv10 = " + a + ";\nv11 = 0;\n"
          "if (v10 == v11) goto L7;\n"
          "L8 : /* else */\n"
          "/* end */;\n"
          "goto L9;\n"
          "L7 : /* then */\n"
          "/* end */;\n"
          "L9 : /* continue */;\n"
          "/* end */;\n" + fr + " = " + a + ";\n"
          "v8 = " + fr + ";\n" + x + " = v8;\n"
          "/* end */" + TAIL);
}

TEST_CASE("a loop function called three times never repeats a label",
          "[codegen][inline]") {
  CompiledProgram program = compileSource(
      "#x : num #w ( #n ) { #i : : #i = #n ; "
      "while lesser ( #i 3 ) do { if eq ( #i 1 ) then { nop ; } else { nop ; } ; "
      "#i = add ( #i 1 ) ; } ; return ( #i ) } : "
      "#x = #w ( 0 ) ; #x = #w ( 1 ) ; print ( #w ( 2 ) ) ;");
  std::string text = generate(program);

  // a while loop defines 5 labels and a branch 3, in the mine and in each call
  requireUniqueLabels(text, 4 * 8);
  REQUIRE(count(text, "/* exit */") == 4);
}

TEST_CASE("the mine of a function shows its sub-function inlined",
          "[codegen][inline]") {
  CompiledProgram program = compileSource(
      "#x : num #f ( #a ) { #l : num #g ( #b ) { #i : "
      ": #i = #b ; while lesser ( #i 3 ) do { #i = add ( #i 1 ) ; } ; "
      "return ( add ( #i #b ) ) } : #l = #g ( #a ) ; return ( #l ) } : "
      "#x = #f ( 0 ) ; #x = #f ( 1 ) ;");
  Node *g = nodesNamed(program, "F_TYPE")[1];
  std::string gParam = namesIn(g->children[3])[0];
  std::string text = generate(program);
  std::string mineG = mineOf(text, functionName(program, 1));
  std::string mineF = mineOf(text, functionName(program, 0));

  REQUIRE(count(mineG, "/* until */") == 1);
  REQUIRE(count(mineF, "/* until */") == 1);
  REQUIRE_THAT(mineF, ContainsSubstring(gParam + " = v"));
  REQUIRE(count(text, "/* until */") == 4);
  requireUniqueLabels(text, 4 * 5);
}

TEST_CASE("an unknown function name is a std::logic_error",
          "[codegen][inline]") {
  CompiledProgram program = compileSource(
      "#x : void #p ( ) { : : nop ; return } : #p ( ) ;");
  nodesNamed(program, "CALL")[0]->children[0]->symbol = "sys999";

  REQUIRE_THROWS_WITH(generate(program), ContainsSubstring("unknown function"));
  REQUIRE_THROWS_AS(generate(program), std::logic_error);
}

TEST_CASE("the wrong number of arguments is a std::logic_error",
          "[codegen][inline]") {
  CompiledProgram tooMany = compileSource(
      "#x : num #f ( #a ) { : : return ( #a ) } : #x = #f ( 1 2 ) ;");
  CompiledProgram tooFew = compileSource(
      "#x : num #f ( #a #b ) { : : return ( #a ) } : #x = #f ( 1 ) ;");
  CompiledProgram none = compileSource(
      "#x : num #f ( #a ) { : : return ( #a ) } : #x = #f ( ) ;");

  REQUIRE_THROWS_AS(generate(tooMany), std::logic_error);
  REQUIRE_THROWS_AS(generate(tooFew), std::logic_error);
  REQUIRE_THROWS_AS(generate(none), std::logic_error);
}

TEST_CASE("a void procedure used as a TERM is a std::logic_error",
          "[codegen][inline]") {
  CompiledProgram program = compileSource(
      "#x : void #p ( ) { : : nop ; return } "
      "num #f ( ) { : : return ( 1 ) } : #p ( ) ; #x = #f ( ) ;");
  std::string p = functionName(program, 0);
  nodesNamed(program, "CALL")[1]->children[0]->symbol = p;

  REQUIRE_THROWS_AS(generate(program), std::logic_error);
}

TEST_CASE("a malformed CALL is a std::logic_error", "[codegen][inline]") {
  const std::string source =
      "#x : void #p ( ) { : : nop ; return } : #p ( ) ;";

  SECTION("the wrong number of children") {
    CompiledProgram program = compileSource(source);
    nodesNamed(program, "CALL")[0]->children.pop_back();
    REQUIRE_THROWS_AS(generate(program), std::logic_error);
  }
  SECTION("a punctuation leaf has the wrong symbol") {
    CompiledProgram program = compileSource(source);
    nodesNamed(program, "CALL")[0]->children[1]->symbol = "x";
    REQUIRE_THROWS_AS(generate(program), std::logic_error);
  }
  SECTION("the arguments are not an INPUT") {
    CompiledProgram program = compileSource(source);
    nodesNamed(program, "CALL")[0]->children[2]->symbol = "ALGO";
    REQUIRE_THROWS_AS(generate(program), std::logic_error);
  }
}

TEST_CASE("direct and indirect recursion are CodeGenUnsupported",
          "[codegen][inline]") {
  SECTION("direct") {
    CompiledProgram program = compileSource(
        "#x : void #p ( ) { : void #q ( ) { : : nop ; return } : "
        "#q ( ) ; return } : #p ( ) ;");
    nodesNamed(program, "CALL")[0]->children[0]->symbol =
        functionName(program, 0);

    REQUIRE_THROWS_AS(generate(program), CodeGenUnsupported);
    REQUIRE_THROWS_WITH(generate(program), ContainsSubstring("recursion"));
  }
  SECTION("indirect") {
    CompiledProgram program = compileSource(
        "#x : void #p ( ) { : void #q ( ) { : void #r ( ) { : : nop ; "
        "return } : #r ( ) ; return } : #q ( ) ; return } : #p ( ) ;");
    nodesNamed(program, "CALL")[0]->children[0]->symbol =
        functionName(program, 0);

    REQUIRE_THROWS_AS(generate(program), CodeGenUnsupported);
  }
  SECTION("the generator is usable again afterwards") {
    CompiledProgram bad = compileSource(
        "#x : void #p ( ) { : void #q ( ) { : : nop ; return } : "
        "#q ( ) ; return } : #p ( ) ;");
    nodesNamed(bad, "CALL")[0]->children[0]->symbol = functionName(bad, 0);
    CompiledProgram good = compileSource(
        "#x : void #p ( ) { : : nop ; return } : #p ( ) ;");
    CodeGen gen("float");

    REQUIRE_THROWS_AS(gen.genProgram(bad.root), CodeGenUnsupported);
    REQUIRE_THAT(gen.genProgram(good.root), ContainsSubstring("/* nop */"));
  }
}

namespace {

std::string runProgram(const std::string &source) {
  if (findCxxCompiler().empty())
    SKIP("no C++ compiler (CXX, g++, clang++ or c++) available");

  CompiledProgram program = compileSource(source);
  return compileAndRun(generate(program));
}

} // namespace

TEST_CASE("compiled void procedure that prints", "[codegen][inline][run]") {
  REQUIRE(runProgram(": void #p ( ) { : : print \"hi\" ; return } : "
                     "#p ( ) ; print \"-\" ; #p ( ) ;") == "hi-hi");
}

TEST_CASE("compiled void procedure with arguments", "[codegen][inline][run]") {
  REQUIRE(runProgram(": void #show ( #a #b ) { : : print ( #a ) ; "
                     "print ( #b ) ; return } : "
                     "#show ( 1 2 ) ; #show ( 3 4 ) ;") == "1234");
}

TEST_CASE("compiled num function with arguments and a local",
          "[codegen][inline][run]") {
  // r = 3 + 4, result = r * 2
  REQUIRE(runProgram("#x : num #f ( #a #b ) { #r : : #r = add ( #a #b ) ; "
                     "return ( mul ( #r 2 ) ) } : "
                     "#x = #f ( 3 4 ) ; print ( #x ) ;") == "14");
}

TEST_CASE("compiled parameters keep their order", "[codegen][inline][run]") {
  // d(5, 3) = 2 and d(d(9, 4), 2) = d(5, 2) = 3
  REQUIRE(runProgram(": num #d ( #a #b ) { : : return ( sub ( #a #b ) ) } : "
                     "print ( #d ( 5 3 ) ) ; print \",\" ; "
                     "print ( #d ( #d ( 9 4 ) 2 ) ) ;") == "2,3");
}

TEST_CASE("compiled call in the condition of a while loop",
          "[codegen][inline][run]") {
  REQUIRE(runProgram("#i : num #lim ( ) { : : return ( 3 ) } : #i = 0 ; "
                     "while lesser ( #i #lim ( ) ) do { print ( #i ) ; "
                     "#i = add ( #i 1 ) ; } ;") == "012");
}

TEST_CASE("compiled f(f(x))", "[codegen][inline][run]") {
  REQUIRE(runProgram("#x : num #f ( #a ) { : : return ( add ( #a 1 ) ) } : "
                     "#x = #f ( #f ( 1 ) ) ; print ( #x ) ;") == "3");
}

TEST_CASE("compiled add(f(a), f(b))", "[codegen][inline][run]") {
  REQUIRE(runProgram("#x : num #sq ( #a ) { : : return ( mul ( #a #a ) ) } : "
                     "#x = add ( #sq ( 3 ) #sq ( 4 ) ) ; print ( #x ) ;") ==
          "25");
}

TEST_CASE("compiled function that reads and writes a variable of main",
          "[codegen][inline][run]") {
  REQUIRE(runProgram("#g : void #inc ( ) { : : #g = add ( #g 1 ) ; return } : "
                     "#g = 5 ; #inc ( ) ; #inc ( ) ; print ( #g ) ;") == "7");
}

TEST_CASE("compiled function with locals called twice",
          "[codegen][inline][run]") {
  // t = 2a, u = t + 1: f(3) = 7 and f(10) = 21
  REQUIRE(runProgram("#x : num #f ( #a ) { #t #u : : #t = mul ( #a 2 ) ; "
                     "#u = add ( #t 1 ) ; return ( #u ) } : "
                     "print ( #f ( 3 ) ) ; print \",\" ; print ( #f ( 10 ) ) ;") ==
          "7,21");
}

TEST_CASE("compiled loop and branch function called with two arguments",
          "[codegen][inline][run]") {
  // adds the numbers 1..n that are larger than 2: s(3) = 3, s(5) = 3+4+5 = 12
  REQUIRE(runProgram(
              "#x : num #s ( #n ) { #i #t : : #i = 1 ; #t = 0 ; "
              "while lesser ( #i add ( #n 1 ) ) do { "
              "if larger ( #i 2 ) then { #t = add ( #t #i ) ; } "
              "else { nop ; } ; #i = add ( #i 1 ) ; } ; return ( #t ) } : "
              "print ( #s ( 3 ) ) ; print \",\" ; print ( #s ( 5 ) ) ;") ==
          "3,12");
}

TEST_CASE("compiled function that calls its sub-function, called twice",
          "[codegen][inline][run]") {
  // g(b) = (first i >= 3 counting up from b) + b: g(0) = 3, g(1) = 4
  // f(a) = g(a) * 10: 30 and 40
  REQUIRE(runProgram(
              "#x : num #f ( #a ) { #l : num #g ( #b ) { #i : "
              ": #i = #b ; while lesser ( #i 3 ) do { #i = add ( #i 1 ) ; } ; "
              "return ( add ( #i #b ) ) } : #l = #g ( #a ) ; "
              "return ( mul ( #l 10 ) ) } : "
              "print ( #f ( 0 ) ) ; print \",\" ; print ( #f ( 1 ) ) ;") ==
          "30,40");
}

TEST_CASE("compiled integer program with mod and calls",
          "[codegen][inline][run]") {
  const std::string spl =
      "#x : num #m ( #a #b ) { : : return ( mod ( #a #b ) ) } : "
      "print ( #m ( 17 5 ) ) ; print \",\" ; #x = #m ( 9 4 ) ; print ( #x ) ;";
  CompiledProgram program = compileSource(spl);

  REQUIRE(program.numberMode == NumberMode::INTEGER);
  REQUIRE_THAT(generate(program), ContainsSubstring("int v"));
  REQUIRE(runProgram(spl) == "2,1");
}

TEST_CASE("compiled float program with div and calls",
          "[codegen][inline][run]") {
  const std::string spl =
      "#x : num #h ( #a ) { : : return ( div ( #a 2 ) ) } : "
      "print ( #h ( 5 ) ) ; print \",\" ; print ( #h ( #h ( 5 ) ) ) ;";

  REQUIRE(runProgram(spl) == "2.5,1.25");
}
