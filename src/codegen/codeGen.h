#ifndef CODE_GEN_H
#define CODE_GEN_H

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
  std::string numType;
  int varCounter;
  int labelCounter;

public:
  explicit CodeGen(std::string numType);

  const std::string &getNumType() const;

  std::string newVar();
  std::string newLabel();

  std::string genAlgo(Node *node);
  std::string genInstr(Node *node);
};

#endif // CODE_GEN_H
