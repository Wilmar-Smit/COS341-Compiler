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

void ScopeTree::buildScopeTree(Node *node, int currentScopeId) {
  if (!node)
    return;

  if (node->symbol == "P" || node->symbol == "SPL_PROG") {
    for (Node *child : node->children) {
      if (child->symbol == "V_DECL") {
        processVDecl(child, 0);
      } else if (child->symbol == "F_DECL") {
        processFDecl(child);
      } else if (child->symbol == "ALGO") {
        processAlgo(child, 0);
      }
    }
  } else if (node->symbol == "F_TYPE") {
    processFType(node);
  } else if (node->symbol == "BRANCH") {
    processBranch(node, currentScopeId);
  } else if (node->symbol == "LOOP") {
    processLoop(node, currentScopeId);
  } else {
    for (Node *child : node->children) {
      buildScopeTree(child, currentScopeId);
    }
  }
}

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
    }
  }
}

void ScopeTree::processFDecl(Node *node) {
  if (!node)
    return;

  for (Node *child : node->children) {
    if (child->symbol == "F_TYPE") {
      processFType(child);
    } else if (child->symbol == "F_DECL") {
      processFDecl(child);
    }
  }
}

void ScopeTree::processFType(Node *node) {
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

  int funcScopeId = sym.createScope(1, 0);

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
    if (!sym.addFunction(0, func)) {
      throw std::runtime_error(
          "Semantic Error: Duplicate function declaration '" +
          funcNode->symbol + "'");
    }
    funcNode->symbol = func.systemName;
  }

  for (Node *child : node->children) {
    if (child->symbol == "ALGO" || child->symbol == "P") {
      buildScopeTree(child, funcScopeId);
    }
  }
}

void ScopeTree::processAlgo(Node *node, int currentScopeId) {
  if (!node)
    return;

  for (Node *child : node->children) {
    if (child->symbol == "INSTR") {
      processInstr(child, currentScopeId);
    } else if (child->symbol == "ALGO") {
      processAlgo(child, currentScopeId);
    }
  }
}

void ScopeTree::processInstr(Node *node, int currentScopeId) {
  if (!node)
    return;

  for (Node *child : node->children) {
    if (child->symbol == "ASSIGN") {
      processAssign(child, currentScopeId);
    } else if (child->symbol == "CALL") {
      processCall(child, currentScopeId);
    } else if (child->symbol == "BRANCH") {
      processBranch(child, currentScopeId);
    } else if (child->symbol == "LOOP") {
      processLoop(child, currentScopeId);
    } else {
      buildScopeTree(child, currentScopeId);
    }
  }
}

void ScopeTree::processAssign(Node *node, int currentScopeId) {
  if (!node)
    return;

  for (Node *child : node->children) {
    if (!child->symbol.empty() && child->symbol[0] == '#') {
      VariableSymbol var;
      if (sym.lookupVariable(child->symbol, currentScopeId, var)) {
        child->symbol = var.systemName;
      } else {
        throw std::runtime_error("Semantic Error: Undeclared variable '" +
                                 child->symbol + "'");
      }
    } else {
      buildScopeTree(child, currentScopeId);
    }
  }
}

void ScopeTree::processCall(Node *node, int currentScopeId) {
  if (!node)
    return;

  for (Node *child : node->children) {
    if (!child->symbol.empty() && child->symbol[0] == '#') {
      FunctionSymbol func;
      if (sym.lookupFunction(child->symbol, func)) {
        child->symbol = func.systemName;
      } else {
        throw std::runtime_error("Semantic Error: Undeclared function '" +
                                 child->symbol + "'");
      }
    } else {
      buildScopeTree(child, currentScopeId);
    }
  }
}

void ScopeTree::processBranch(Node *node, int currentScopeId) {
  if (!node)
    return;

  const Scope *parentScope = sym.getScope(currentScopeId);
  int newLevel = parentScope ? parentScope->level + 1 : 2;
  int blockScopeId = sym.createScope(newLevel, currentScopeId);

  for (Node *child : node->children) {
    if (child->symbol == "ALGO") {
      processAlgo(child, blockScopeId);
    } else {
      buildScopeTree(child, blockScopeId);
    }
  }
}

void ScopeTree::processLoop(Node *node, int currentScopeId) {
  if (!node)
    return;

  const Scope *parentScope = sym.getScope(currentScopeId);
  int newLevel = parentScope ? parentScope->level + 1 : 2;
  int blockScopeId = sym.createScope(newLevel, currentScopeId);

  for (Node *child : node->children) {
    if (child->symbol == "ALGO") {
      processAlgo(child, blockScopeId);
    } else {
      buildScopeTree(child, blockScopeId);
    }
  }
}
