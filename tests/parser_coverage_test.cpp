// Grammar coverage + tree.xml structure tests, run through the real
// pipeline (lex -> SLR parse -> tree.xml write) and asserted with pugixml.
//
// Token syntax matches input.txt: every token is its own space-separated
// word, e.g. "( #x )" not "(#x)".

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <string>
#include <vector>

#include "test_helpers.h"

namespace {

// Checks: unique non-empty ids, exactly one root, every parent resolves.
void assertWellFormedTree(const std::vector<XmlNodeInfo> &nodes) {
  REQUIRE_FALSE(nodes.empty());

  std::set<std::string> ids;
  int rootCount = 0;
  for (auto &n : nodes) {
    INFO("node id='" << n.id << "' content='" << n.content << "'");
    REQUIRE_FALSE(n.id.empty());
    REQUIRE_FALSE(n.content.empty());
    bool firstTimeSeen = ids.insert(n.id).second;
    REQUIRE(firstTimeSeen);  // fails loudly (via INFO above) on a duplicate id
    if (!n.hasParent) rootCount++;
  }
  INFO("expected exactly one node with no <parent> (the root)");
  REQUIRE(rootCount == 1);

  for (auto &n : nodes) {
    if (!n.hasParent) continue;
    INFO("node id='" << n.id << "' content='" << n.content
                      << "' has <parent>" << n.parent
                      << "</parent>, which should exist as some node's <id>");
    REQUIRE(ids.count(n.parent) == 1);
  }
}

// Runs `source`, asserts accept + well-formed tree.xml, returns nodes.
std::vector<XmlNodeInfo> acceptAndLoad(const std::string &source) {
  PipelineResult result = runPipeline(source);

  INFO("parser diagnostics:\n" << result.diagnostics);
  REQUIRE(result.accepted);
  REQUIRE(result.xmlWritten);

  pugi::xml_document doc = loadTreeXml();
  auto nodes = collectNodes(doc);
  assertWellFormedTree(nodes);
  return nodes;
}

// Asserts a node with the given nonterminal content exists and was
// produced by an epsilon reduction (present, no <children> tag - see
// TreeBuilder::xmlHelper, which omits <children> entirely when empty).
void assertEpsilonNodePresent(const std::vector<XmlNodeInfo> &nodes,
                               const std::string &nonterminal) {
  const XmlNodeInfo *node = nullptr;
  for (auto &n : nodes) {
    if (n.content == nonterminal && !n.hasChildrenTag) {
      node = &n;
      break;
    }
  }
  INFO("expected a '" << nonterminal
                       << "' node produced by an epsilon reduction "
                          "(present, with no <children> tag)");
  REQUIRE(node != nullptr);
}

}  // namespace

TEST_CASE("minimal empty program (all-epsilon V_DECL/F_DECL/ALGO)",
          "[coverage][epsilon]") {
  auto nodes = acceptAndLoad(": : $");

  assertEpsilonNodePresent(nodes, "V_DECL");
  assertEpsilonNodePresent(nodes, "F_DECL");
  assertEpsilonNodePresent(nodes, "ALGO");
}

TEST_CASE("multiple variable declarations (V_DECL recursion)",
          "[coverage][vdecl]") {
  auto nodes = acceptAndLoad("#a #b #c : : $");

  int vdeclCount = 0;
  for (auto &n : nodes)
    if (n.content == "V_DECL") vdeclCount++;
  // 3 names -> 3 recursive wraps + 1 epsilon base = 4.
  INFO("expected 4 V_DECL nodes");
  REQUIRE(vdeclCount == 4);

  assertEpsilonNodePresent(nodes, "V_DECL");

  for (auto &name : {"#a", "#b", "#c"}) {
    INFO("expected a leaf for NAME token '" << name << "'");
    REQUIRE(findByContent(nodes, name) != nullptr);
  }
}

TEST_CASE("void-type function declaration (F_TYPE -> void ...)",
          "[coverage][fdecl][void]") {
  auto nodes = acceptAndLoad(": void #f ( ) { : : return } : $");

  REQUIRE(findByContent(nodes, "F_TYPE") != nullptr);
  REQUIRE(findByContent(nodes, "void") != nullptr);
  REQUIRE(findByContent(nodes, "#f") != nullptr);
  REQUIRE(findByContent(nodes, "return") != nullptr);
  // The inner V_DECL (function's parameter list) is epsilon here.
  assertEpsilonNodePresent(nodes, "V_DECL");
}

TEST_CASE("num-type function declaration with return(TERM)",
          "[coverage][fdecl][num]") {
  auto nodes = acceptAndLoad(": num #g ( ) { : : return ( 0 ) } : $");

  REQUIRE(findByContent(nodes, "F_TYPE") != nullptr);
  REQUIRE(findByContent(nodes, "num") != nullptr);
  REQUIRE(findByContent(nodes, "#g") != nullptr);
  REQUIRE(findByContent(nodes, "return") != nullptr);

  const XmlNodeInfo *returnedTerm = findByContent(nodes, "0");
  REQUIRE(returnedTerm != nullptr);
  INFO("expected returned value wrapped in a TERM node");
  REQUIRE(countAncestorsWithContent(nodes, returnedTerm->id, "TERM") >= 1);
}

TEST_CASE("multiple function declarations (F_DECL recursion)",
          "[coverage][fdecl][recursion]") {
  auto nodes = acceptAndLoad(
      ": void #f ( ) { : : return } num #g ( ) { : : return ( 0 ) } : $");

  int fDeclCount = 0;
  int fTypeCount = 0;
  for (auto &n : nodes) {
    if (n.content == "F_DECL") fDeclCount++;
    if (n.content == "F_TYPE") fTypeCount++;
  }
  // Outer: 2 recursive wraps + 1 epsilon base = 3, plus one epsilon
  // F_DECL per function body's own inner P: 3 + 2 = 5 total.
  INFO("expected 2 F_TYPE nodes and 5 F_DECL nodes");
  REQUIRE(fTypeCount == 2);
  REQUIRE(fDeclCount == 5);
  assertEpsilonNodePresent(nodes, "F_DECL");

  REQUIRE(findByContent(nodes, "#f") != nullptr);
  REQUIRE(findByContent(nodes, "#g") != nullptr);
}

TEST_CASE("CALL with zero arguments (INPUT epsilon)",
          "[coverage][call][epsilon]") {
  auto nodes = acceptAndLoad(": : #f ( ) ; $");

  REQUIRE(findByContent(nodes, "CALL") != nullptr);
  REQUIRE(findByContent(nodes, "#f") != nullptr);
  assertEpsilonNodePresent(nodes, "INPUT");
}

TEST_CASE("CALL with 2+ arguments (INPUT recursion)",
          "[coverage][call][recursion]") {
  auto nodes = acceptAndLoad(": : #f ( #x 11 ) ; $");

  REQUIRE(findByContent(nodes, "CALL") != nullptr);
  REQUIRE(findByContent(nodes, "#x") != nullptr);
  REQUIRE(findByContent(nodes, "11") != nullptr);

  int inputCount = 0;
  for (auto &n : nodes)
    if (n.content == "INPUT") inputCount++;
  // 2 terms -> 2 recursive wraps + 1 epsilon base = 3.
  INFO("expected 3 INPUT nodes");
  REQUIRE(inputCount == 3);
}

TEST_CASE("nested arithmetic at least 2 levels deep",
          "[coverage][term][nested]") {
  auto nodes =
      acceptAndLoad(": : #result = mod ( add ( #x 11 ) sub ( #y 22 ) ) ; $");

  REQUIRE(findByContent(nodes, "mod") != nullptr);
  REQUIRE(findByContent(nodes, "add") != nullptr);
  REQUIRE(findByContent(nodes, "sub") != nullptr);

  const XmlNodeInfo *innerLeaf = findByContent(nodes, "11");
  REQUIRE(innerLeaf != nullptr);
  INFO("expected at least 2 nested TERM ancestors, not flattened");
  REQUIRE(countAncestorsWithContent(nodes, innerLeaf->id, "TERM") >= 2);
}

TEST_CASE("nested boolean at least 2 levels deep",
          "[coverage][bool][nested]") {
  auto nodes = acceptAndLoad(
      ": : if not ( and ( eq ( #x 1 ) or ( larger ( #y 2 ) lesser ( #z 3 ) ) "
      ") ) then { nop ; } else { nop ; } ; $");

  REQUIRE(findByContent(nodes, "not") != nullptr);
  REQUIRE(findByContent(nodes, "and") != nullptr);
  REQUIRE(findByContent(nodes, "or") != nullptr);
  REQUIRE(findByContent(nodes, "eq") != nullptr);
  REQUIRE(findByContent(nodes, "larger") != nullptr);
  REQUIRE(findByContent(nodes, "lesser") != nullptr);

  const XmlNodeInfo *deepLeaf = findByContent(nodes, "3");
  REQUIRE(deepLeaf != nullptr);
  INFO("expected at least 3 nested BOOL ancestors, not flattened");
  REQUIRE(countAncestorsWithContent(nodes, deepLeaf->id, "BOOL") >= 3);
}

TEST_CASE("BRANCH (if/then/else) with non-trivial ALGO in both branches",
          "[coverage][branch]") {
  auto nodes = acceptAndLoad(
      ": : if eq ( #x 0 ) then { #y = 1 ; print ( #y ) ; } else "
      "{ #y = 2 ; print ( #y ) ; } ; $");

  REQUIRE(findByContent(nodes, "BRANCH") != nullptr);
  REQUIRE(findByContent(nodes, "if") != nullptr);
  REQUIRE(findByContent(nodes, "then") != nullptr);
  REQUIRE(findByContent(nodes, "else") != nullptr);
  REQUIRE(findByContent(nodes, "print") != nullptr);

  int assignCount = 0;
  for (auto &n : nodes)
    if (n.content == "ASSIGN") assignCount++;
  INFO("expected one ASSIGN per branch (then + else)");
  REQUIRE(assignCount == 2);
}

TEST_CASE("LOOP with COND-first form (while/until BOOL do {...})",
          "[coverage][loop][cond-first]") {
  auto nodes = acceptAndLoad(": : while eq ( #x 0 ) do { nop ; } ; $");

  REQUIRE(findByContent(nodes, "LOOP") != nullptr);
  REQUIRE(findByContent(nodes, "COND") != nullptr);
  REQUIRE(findByContent(nodes, "while") != nullptr);
  REQUIRE(findByContent(nodes, "do") != nullptr);
  REQUIRE(findByContent(nodes, "nop") != nullptr);
}

TEST_CASE("LOOP with COND-last form (do {...} while/until BOOL)",
          "[coverage][loop][cond-last]") {
  auto nodes = acceptAndLoad(": : do { nop ; } until eq ( #x 0 ) ; $");

  REQUIRE(findByContent(nodes, "LOOP") != nullptr);
  REQUIRE(findByContent(nodes, "COND") != nullptr);
  REQUIRE(findByContent(nodes, "until") != nullptr);
  REQUIRE(findByContent(nodes, "do") != nullptr);
  REQUIRE(findByContent(nodes, "nop") != nullptr);
}
