#ifndef SCOPE_TREE_H
#define SCOPE_TREE_H

#include "../symbolTable/symbolTable.h"
#include "../xml/TreeBuilder.h"

class ScopeTree {
private:
  SymbolTable sym;

  void buildScopeTree(Node *node, int currentScopeId);
  void processVDecl(Node *node, int currentScopeId);
  void processFDecl(Node *node);
  void processFType(Node *node);
  void processAlgo(Node *node, int currentScopeId);
  void processInstr(Node *node, int currentScopeId);
  void processAssign(Node *node, int currentScopeId);
  void processCall(Node *node, int currentScopeId);
  void processBranch(Node *node, int currentScopeId);
  void processLoop(Node *node, int currentScopeId);

public:
  ScopeTree(TreeBuilder &tree);

  SymbolTable &getSymbolTable();
  const SymbolTable &getSymbolTable() const;

  bool validateScopes();
};

#endif
