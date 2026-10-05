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

  // Handle scope blocks (BRANCH / LOOP)
  int nextScopeId = currentScopeId;
  if (node->symbol == "BRANCH" || node->symbol == "LOOP") {
    const Scope *parentScope = sym.getScope(currentScopeId);
    int newLevel = parentScope ? parentScope->level + 1 : 2;
    nextScopeId = sym.createScope(newLevel, currentScopeId);
  }

  // Universal check for identifier usages (nodes starting with '#')
  if (!node->symbol.empty() && node->symbol[0] == '#') {
    VariableSymbol var;
    FunctionSymbol func;
    if (sym.lookupVariable(node->symbol, nextScopeId, var)) {
      node->symbol = var.systemName;
    } else if (sym.lookupFunction(node->symbol, nextScopeId, func)) {
      node->symbol = func.systemName;
    } else {
      throw std::runtime_error("Semantic Error: Undeclared identifier '" +
                               node->symbol + "'");
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

  int funcScopeId = sym.createScope(1, currentScopeId);

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
