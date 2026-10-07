#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "../symbolTable/symbolTable.h"
#include "../xml/TreeBuilder.h"

#include <string>
#include <unordered_map>

// Phase 2b: crawls the renamed syntax tree, records a type for every node
// and stores the types of all names in the symbol table. Throws
// std::runtime_error on the first violated rule.
class TypeChecker {
private:
  SymbolTable &sym;
  std::unordered_map<int, SemType> nodeTypes; // node id -> type_of(node)

  void checkFloatIntegerConflict(Node *root);

  SemType checkProg(Node *node);
  SemType checkP(Node *node);
  SemType checkVDecl(Node *node);
  SemType checkFDecl(Node *node);
  SemType checkFType(Node *node);
  SemType checkAlgo(Node *node);
  SemType checkInstr(Node *node);
  SemType checkOutp(Node *node);
  SemType checkCall(Node *node);
  SemType checkInput(Node *node);
  SemType checkAssign(Node *node);
  SemType checkTerm(Node *node);
  SemType checkBranch(Node *node);
  SemType checkBool(Node *node);
  SemType checkLoop(Node *node);
  SemType checkCond(Node *node);

  SemType record(Node *node, SemType type);
  void expect(Node *node, const std::string &nonTerminal);

public:
  TypeChecker(Node *root, SymbolTable &sym);

  SemType typeOf(const Node *node) const;
};

std::string semTypeToString(SemType type);

#endif // TYPE_CHECKER_H
