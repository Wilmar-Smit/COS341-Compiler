#ifndef CODE_GEN_H
#define CODE_GEN_H

#include <map>
#include <stdexcept>
#include <string>

#include "../xml/TreeBuilder.h"

class CodeGenUnsupported : public std::runtime_error {
public:
  explicit CodeGenUnsupported(const std::string &message);
};

std::string determineNumType(Node *root);

class CodeGen {
private:
  struct FunctionEntry {
    Node *node;
    std::string returnVar;
  };

  std::string numType;
  int varCounter;
  int labelCounter;
  std::map<std::string, FunctionEntry> functions;
  std::string hoistedDecls;

  std::string declare(const std::string &var) const;
  std::string genOutp(Node *node);
  std::string genAssign(Node *node);
  std::string genNegation(Node *node, const std::string &resultVar);
  std::string genBinary(Node *node, const std::string &resultVar);
  std::string genComparison(Node *node, const std::string &trueLabel);
  std::string genNot(Node *inner, const std::string &trueLabel);
  std::string genAnd(Node *node, const std::string &trueLabel,
                     const std::string &falseLabel);
  std::string genOr(Node *node, const std::string &trueLabel,
                    const std::string &falseLabel);
  std::string genBranch(Node *node);
  void registerFunctions(Node *node);
  FunctionEntry &lookupFunction(const std::string &name);
  std::string genP(Node *node);
  std::string genFDecl(Node *node);
  std::string genFunction(Node *node);
  std::string genVDecl(Node *node) const;
  std::string genLoop(Node *node);
  std::string genTestLoop(Node *boolNode, Node *algo, bool untilForm);
  std::string genRepeatLoop(Node *boolNode, Node *algo, bool whileForm);

public:
  explicit CodeGen(std::string numType);

  const std::string &getNumType() const;

  std::string newVar();
  std::string newLabel();

  std::string genAlgo(Node *node);
  std::string genInstr(Node *node);
  std::string genTerm(Node *node, const std::string &resultVar);
  std::string genBool(Node *node, const std::string &trueLabel,
                      const std::string &falseLabel);
  std::string genProgram(Node *root);
};

#endif // CODE_GEN_H
