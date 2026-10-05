#include "scopeTree.h"

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

  std::string varName = "";
  DataType type = DataType::NUM;

  for (Node *child : node->children) {
    if (!child->symbol.empty() && child->symbol[0] == '#') {
      varName = child->symbol;
    } else if (child->symbol == "num" || child->symbol == "n") {
      type = DataType::NUM;
    } else if (child->symbol == "string" || child->symbol == "s") {
      type = DataType::STRING;
    }
  }

  if (!varName.empty()) {
    VariableSymbol var{varName, type, currentScopeId};
    sym.addVariable(currentScopeId, var);
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

  std::string funcName = "";
  DataType returnType = DataType::VOID;
  std::vector<DataType> paramTypes;

  for (Node *child : node->children) {
    if (!child->symbol.empty() && child->symbol[0] == '#') {
      funcName = child->symbol;
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

  FunctionSymbol func{funcName, returnType, paramTypes, funcScopeId};
  sym.addFunction(0, func);

  for (Node *child : node->children) {
    if (child->symbol == "ALGO") {
      processAlgo(child, funcScopeId);
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
    }
  }
}

void ScopeTree::processAssign(Node *node, int currentScopeId) {
  if (!node)
    return;

  for (Node *child : node->children) {
    if (!child->symbol.empty() && child->symbol[0] == '#') {
      VariableSymbol var;
      sym.lookupVariable(child->symbol, currentScopeId, var);
    }
  }
}

void ScopeTree::processCall(Node *node, int currentScopeId) {
  if (!node)
    return;

  for (Node *child : node->children) {
    if (!child->symbol.empty() && child->symbol[0] == '#') {
      FunctionSymbol func;
      sym.lookupFunction(child->symbol, func);
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
