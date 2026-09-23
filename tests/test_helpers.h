#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

// Shared plumbing: drives the real lexer + parser and reads back tree.xml.

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

// createChain() isn't `inline`, so only test_helpers.cpp includes
// generic.handler.h directly - avoids an ODR link error elsewhere.

// Drives ParseVisitor directly instead of Parser::ParseTokens, whose
// return value can't be trusted (always returns true - see PR writeup).

struct PipelineResult {
  bool accepted = false;
  std::string diagnostics;  // whatever ParseVisitor printed while parsing
  bool xmlWritten = false;
};

// Wraps generic.handler.h's createChain(). Defined in test_helpers.cpp.
TokenHandler *newHandlerChain();

// Mirrors main.cpp's tokenize step, including the trailing DOLLAR_EOF
// append. Defined in test_helpers.cpp.
std::vector<Token *> tokenizeAll(const std::string &source);

// Runs source through lex -> SLR parse -> (on accept) tree.xml write,
// like main.cpp, but returns the real accept/reject signal. Defined in
// test_helpers.cpp.
PipelineResult runPipeline(const std::string &source);

// Reading back tree.xml with pugixml.

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

// Counts ancestors (including startId) with the given content, to verify
// nesting depth rather than trusting a flat node count.
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
