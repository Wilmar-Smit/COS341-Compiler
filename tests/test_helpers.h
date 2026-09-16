#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

// Shared plumbing for the coverage/table tests: driving the real lexer +
// SLR parser end to end, and reading back the tree.xml it writes with
// pugixml so assertions run against actual output structure, not just
// "did it accept".

#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include <pugixml.hpp>

#include "token.h"
#include "tokenHandler.h"

// NOTE: generic.handler.h's createChain() is a free function defined
// (not declared) in the header, with no `inline` keyword - fine for
// main.cpp, which is the header's only #include site in production code,
// but it means any second .cpp that includes it triggers an ODR
// "multiple definition" link error. Rather than add `inline` to
// production code, tests/test_helpers.cpp is the single translation unit
// that includes generic.handler.h; everything here only sees the
// declarations below and links against that one definition.

// ---------------------------------------------------------------------
// Running the pipeline
// ---------------------------------------------------------------------
//
// NOTE: this deliberately drives ParseVisitor directly instead of going
// through Parser::ParseTokens. Parser::ParseTokens (src/Parser/Parser/
// Parser.cpp) reads:
//
//   auto accept = visitor->parseTokens(tokens);
//   if (accept) { /* return visitor . get composite tree */ }
//   else        { /* throw exeption ? idk */ }
//   ...
//   return true;
//
// Both branches are no-ops and the function unconditionally returns
// `true` unless something throws all the way out of the try block - which
// never happens, because ParseVisitor::parseTokens already catches its own
// runtime_error internally and returns false. So Parser::ParseTokens's
// return value cannot currently be trusted to reflect whether parsing
// actually succeeded. This is a real bug in application code, but every
// test below can be written without touching it by asking ParseVisitor
// directly, so it is reported (see the accompanying writeup) rather than
// patched.

struct PipelineResult {
  bool accepted = false;
  std::string diagnostics;  // whatever ParseVisitor printed (e.g. the
                             // ErrorAction message) while parsing
  bool xmlWritten = false;
};

// Thin wrapper around generic.handler.h's createChain(), so test files
// never need to include that header directly (see the ODR note above).
// Defined once, in test_helpers.cpp.
TokenHandler *newHandlerChain();

// Mirrors main.cpp's tokenize step exactly (including appending a
// trailing DOLLAR_EOF token unconditionally, whether or not the source
// already ended in a literal "$"). Defined once, in test_helpers.cpp.
std::vector<Token *> tokenizeAll(const std::string &source);

// Runs source through lex -> SLR parse -> (on accept) tree.xml write,
// exactly like main.cpp does, but returns the *real* accept/reject
// signal from ParseVisitor and captures whatever it printed. Defined
// once, in test_helpers.cpp.
//
// Assumes the current working directory contains ProductionRules.txt,
// SLR_ACTION_Table.csv and SLR_GOTO_Table.csv (the CMake test target
// points ctest at a scratch copy of these - see CMakeLists.txt - so a
// fresh tree.xml written here never touches the repo's checked-in one).
PipelineResult runPipeline(const std::string &source);

// ---------------------------------------------------------------------
// Reading back tree.xml with pugixml
// ---------------------------------------------------------------------

struct XmlNodeInfo {
  std::string id;
  std::string content;
  bool hasParent = false;
  std::string parent;
  bool hasChildrenTag = false;
  std::vector<std::string> children;
};

inline pugi::xml_document loadTreeXml(const std::string &path = "tree.xml") {
  pugi::xml_document doc;
  pugi::xml_parse_result parseResult = doc.load_file(path.c_str());
  if (!parseResult) {
    throw std::runtime_error(std::string("failed to parse ") + path +
                              " as XML: " + parseResult.description());
  }
  return doc;
}

inline std::vector<XmlNodeInfo> collectNodes(const pugi::xml_document &doc) {
  std::vector<XmlNodeInfo> nodes;
  pugi::xml_node tree = doc.child("tree");
  for (pugi::xml_node el : tree.children()) {
    XmlNodeInfo info;
    info.id = el.child("id").child_value();
    info.content = el.child("content").child_value();

    pugi::xml_node parentEl = el.child("parent");
    if (parentEl) {
      info.hasParent = true;
      info.parent = parentEl.child_value();
    }

    pugi::xml_node childrenEl = el.child("children");
    if (childrenEl) {
      info.hasChildrenTag = true;
      for (pugi::xml_node c : childrenEl.children("child")) {
        info.children.push_back(c.child_value());
      }
    }

    nodes.push_back(std::move(info));
  }
  return nodes;
}

inline const XmlNodeInfo *findByContent(const std::vector<XmlNodeInfo> &nodes,
                                         const std::string &content) {
  for (auto &n : nodes) {
    if (n.content == content) return &n;
  }
  return nullptr;
}

inline std::map<std::string, XmlNodeInfo> indexById(
    const std::vector<XmlNodeInfo> &nodes) {
  std::map<std::string, XmlNodeInfo> byId;
  for (auto &n : nodes) byId[n.id] = n;
  return byId;
}

// Counts how many ancestors of (and including) the node with id `startId`
// have the given content, walking parent links up to the root. Used to
// verify actual nesting depth (e.g. TERM-inside-TERM) rather than trusting
// a flat node count.
inline int countAncestorsWithContent(const std::vector<XmlNodeInfo> &nodes,
                                      const std::string &startId,
                                      const std::string &content) {
  auto byId = indexById(nodes);
  int count = 0;
  std::string cur = startId;
  while (true) {
    auto it = byId.find(cur);
    if (it == byId.end()) break;
    if (it->second.content == content) count++;
    if (!it->second.hasParent) break;
    cur = it->second.parent;
  }
  return count;
}

#endif
