#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>

enum class DataType { NUM, STRING, VOID, UNKNOWN };

struct VariableSymbol {
  std::string name;
  DataType type;
  int scopeId;
};

struct FunctionSymbol {
  std::string name;
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
public:
  std::unordered_map<int, std::vector<Scope>> scopes;
  int currentScopeId;
  int nextScopeId;

  SymbolTable();

  int createScope(int level, int parentId);

  Scope *getScope(int scopeId);
  const Scope *getScope(int scopeId) const;

  bool addVariable(int scopeId, const VariableSymbol &symbol);
  bool addFunction(int scopeId, const FunctionSymbol &symbol);

  bool lookupVariable(const std::string &name, int startScopeId,
                      VariableSymbol &outSymbol) const;
  bool lookupFunction(const std::string &name, FunctionSymbol &outSymbol) const;
};

#endif // SYMBOL_TABLE_H
