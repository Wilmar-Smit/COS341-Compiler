#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>

enum class DataType { NUM, STRING, VOID };

// Phase 2b type attributes (spec: every node starts as "unknown")
enum class SemType { UNKNOWN, NUMERIC, BOOLEAN, PROCEDURE, OK };

struct VariableSymbol {
  std::string name;
  std::string systemName;
  DataType type;
  int scopeId;
};

struct FunctionSymbol {
  std::string name;
  std::string systemName;
  DataType returnType;
  std::vector<DataType> paramTypes;
  int scopeId;
};

struct Scope {
  int id;
  int level;
  int parentId;
  std::unordered_map<std::string, VariableSymbol> variables;
  std::unordered_map<std::string, FunctionSymbol> functions;
};

class SymbolTable {
private:
  int currentScopeId;
  int nextScopeId;
  std::unordered_map<int, std::vector<Scope>> scopes;
  // keyed by system-generated name, which is unique after phase 2a
  std::unordered_map<std::string, SemType> nameTypes;

public:
  SymbolTable();
bool existsInAnyScope(const std::string &name) const;
  int createScope(int level, int parentId);
  Scope *getScope(int scopeId);
  const Scope *getScope(int scopeId) const;

  bool addVariable(int scopeId, VariableSymbol &symbol);
  bool addFunction(int scopeId, FunctionSymbol &symbol);

  bool lookupVariable(const std::string &name, int startScopeId,
                      VariableSymbol &outSymbol) const;

  bool lookupFunction(const std::string &name, int startScopeId,
                      FunctionSymbol &outSymbol) const;

  void setNameType(const std::string &systemName, SemType type);
  SemType getNameType(const std::string &systemName) const;
};

#endif // SYMBOL_TABLE_H
