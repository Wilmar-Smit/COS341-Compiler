#ifndef TOKEN_H
#define TOKEN_H
#include "token.enum.h"
#include <regex>
#include <string>
using std::string;
// uses chain of responsibility
// uses builder
// uses template method

class TokenHandler {

public:
  TokenHandler();
  TokenHandler *SetNext(TokenHandler *handler);

protected:
  std::regex pattern;
  TokenHandler *next;

  TokenHandler &setMatchString(string &match);
};

#endif
