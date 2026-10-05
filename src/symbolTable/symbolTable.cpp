#include "symbolTable.h"

SymbolTable::SymbolTable() : currentScopeId(0), nextScopeId(0) {
  createScope(0, -1);
}

int SymbolTable::createScope(int level, int parentId) {
  int id = nextScopeId++;
  Scope newScope;
  newScope.id = id;
  newScope.level = level;
  newScope.parentId = parentId;

  scopes[id].push_back(newScope);
  return id;
}

Scope *SymbolTable::getScope(int scopeId) {
  auto it = scopes.find(scopeId);
  if (it != scopes.end() && !it->second.empty()) {
    return &it->second.back();
  }
  return nullptr;
}

const Scope *SymbolTable::getScope(int scopeId) const {
  auto it = scopes.find(scopeId);
  if (it != scopes.end() && !it->second.empty()) {
    return &it->second.back();
  }
  return nullptr;
}

bool SymbolTable::addVariable(int scopeId, const VariableSymbol &symbol) {
  Scope *scope = getScope(scopeId);
  if (!scope) {
    return false;
  }
  if (scope->variables.find(symbol.name) != scope->variables.end()) {
    return false;
  }
  scope->variables[symbol.name] = symbol;
  return true;
}

bool SymbolTable::addFunction(int scopeId, const FunctionSymbol &symbol) {
  Scope *scope = getScope(scopeId);
  if (!scope) {
    return false;
  }
  if (scope->functions.find(symbol.name) != scope->functions.end()) {
    return false;
  }
  scope->functions[symbol.name] = symbol;
  return true;
}

bool SymbolTable::lookupVariable(const std::string &name, int startScopeId,
                                 VariableSymbol &outSymbol) const {
  int currId = startScopeId;
  while (currId != -1) {
    const Scope *scope = getScope(currId);
    if (!scope) {
      break;
    }

    auto it = scope->variables.find(name);
    if (it != scope->variables.end()) {
      outSymbol = it->second;
      return true;
    }

    currId = scope->parentId;
  }
  return false;
}

bool SymbolTable::lookupFunction(const std::string &name,
                                 FunctionSymbol &outSymbol) const {
  const Scope *globalScope = getScope(0);
  if (!globalScope) {
    return false;
  }

  auto it = globalScope->functions.find(name);
  if (it != globalScope->functions.end()) {
    outSymbol = it->second;
    return true;
  }

  return false;
}
