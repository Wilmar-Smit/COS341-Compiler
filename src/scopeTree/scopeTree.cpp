#include "scopeTree.h"
#include <stdexcept>

ScopeTree::ScopeTree(TreeBuilder &tree) {
  Node *root = tree.getRoot();
  if (root) {
    buildScopeTree(root, 0);
  }
}

SymbolTable &ScopeTree::getSymbolTable() { return sym; }

const SymbolTable &ScopeTree::getSymbolTable() const { return sym; }

bool ScopeTree::validateScopes() { return true; }

void ScopeTree::processVDecl(Node *node, int currentScopeId) {
  if (!node || node->children.empty())
    return;

  Node *varNode = nullptr;
  DataType type = DataType::NUM;

  for (Node *child : node->children) {
    if (!child->symbol.empty() && child->symbol[0] == '#') {
      varNode = child;
    } else if (child->symbol == "num" || child->symbol == "n") {
      type = DataType::NUM;
    } else if (child->symbol == "string" || child->symbol == "s") {
      type = DataType::STRING;
    }
  }

  if (varNode) {
    VariableSymbol var{varNode->symbol, "", type, currentScopeId};
    if (!sym.addVariable(currentScopeId, var)) {
      throw std::runtime_error(
          "Semantic Error: Duplicate variable declaration '" + varNode->symbol +
          "'");
    }
    varNode->symbol = var.systemName;
  }

  for (Node *child : node->children) {
    if (child->symbol == "V_DECL") {
      processVDecl(child, currentScopeId);
    } else {
      buildScopeTree(child, currentScopeId);
    }
  }
}

void ScopeTree::processFDecl(Node *node, int currentScopeId) {
  if (!node)
    return;

  for (Node *child : node->children) {
    if (child->symbol == "F_TYPE") {
      processFType(child, currentScopeId);
    } else if (child->symbol == "F_DECL") {
      processFDecl(child, currentScopeId);
    }
  }
}
void ScopeTree::buildScopeTree(Node *node, int currentScopeId) {
  if (!node)
    return;

  // Handle variable declarations
  if (node->symbol == "V_DECL") {
    processVDecl(node, currentScopeId);
    return;
  }

  // Handle function definitions / scopes
  if (node->symbol == "F_TYPE") {
    processFType(node, currentScopeId);
    return;
  }

  if (node->symbol == "F_DECL") {
    processFDecl(node, currentScopeId);
    return;
  }

  int nextScopeId = currentScopeId;
  // Note: BRANCH and LOOP scope creation was removed in Bug 1 fix

  // Handle CALL nodes explicitly: the first child is a function name
  if (node->symbol == "CALL") {
    if (!node->children.empty() && node->children[0]) {
      Node *funcNode = node->children[0];
      if (!funcNode->symbol.empty() && funcNode->symbol[0] == '#') {
        FunctionSymbol func;
        if (sym.lookupFunction(funcNode->symbol, nextScopeId, func)) {
          funcNode->symbol = func.systemName;
        } else {
          if (sym.existsInAnyScope(funcNode->symbol)) {
            throw std::runtime_error(
                "Semantic Error: Out of scope reference to function '" +
                funcNode->symbol + "'");
          } else {
            throw std::runtime_error("Semantic Error: Undeclared function '" +
                                     funcNode->symbol + "'");
          }
        }
      }
    }
    // Traverse remaining children (e.g., arguments) normally
    for (size_t i = 1; i < node->children.size(); ++i) {
      buildScopeTree(node->children[i], nextScopeId);
    }
    return;
  }

  // For all other nodes starting with '#', perform a strict VARIABLE-ONLY
  // lookup
  if (!node->symbol.empty() && node->symbol[0] == '#') {
    VariableSymbol var;
    if (sym.lookupVariable(node->symbol, nextScopeId, var)) {
      node->symbol = var.systemName;
    } else {
      if (sym.existsInAnyScope(node->symbol)) {
        throw std::runtime_error(
            "Semantic Error: Out of scope reference to variable '" +
            node->symbol + "'");
      } else {
        throw std::runtime_error("Semantic Error: Undeclared variable '" +
                                 node->symbol + "'");
      }
    }
  }

  // Recursively traverse all children
  for (Node *child : node->children) {
    buildScopeTree(child, nextScopeId);
  }
}
void ScopeTree::processFType(Node *node, int currentScopeId) {
  if (!node)
    return;

  Node *funcNode = nullptr;
  DataType returnType = DataType::VOID;
  std::vector<DataType> paramTypes;

  for (Node *child : node->children) {
    if (!child->symbol.empty() && child->symbol[0] == '#') {
      funcNode = child;
    } else if (child->symbol == "num") {
      returnType = DataType::NUM;
    } else if (child->symbol == "void") {
      returnType = DataType::VOID;
    }
  }

  // FIX: Compute scope level dynamically as parent level + 1
  const Scope *parentScope = sym.getScope(currentScopeId);
  int newLevel = parentScope ? parentScope->level + 1 : 1;
  int funcScopeId = sym.createScope(newLevel, currentScopeId);

  // Process local variable declarations inside the function scope
  for (Node *child : node->children) {
    if (child->symbol == "V_DECL") {
      processVDecl(child, funcScopeId);
    }
  }

  Scope *funcScope = sym.getScope(funcScopeId);
  if (funcScope) {
    for (const auto &pair : funcScope->variables) {
      paramTypes.push_back(pair.second.type);
    }
  }

  if (funcNode) {
    FunctionSymbol func{funcNode->symbol, "", returnType, paramTypes,
                        funcScopeId};
    if (!sym.addFunction(currentScopeId, func)) {
      throw std::runtime_error(
          "Semantic Error: Duplicate function declaration '" +
          funcNode->symbol + "'");
    }
    funcNode->symbol = func.systemName;
  }

  // Traverse the rest of the function body with the function scope ID
  for (Node *child : node->children) {
    if (child->symbol != "V_DECL" && child != funcNode) {
      buildScopeTree(child, funcScopeId);
    }
  }
}
