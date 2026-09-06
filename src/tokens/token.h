#ifndef TOKEN_H
#define TOKEN_H
#include "token.enum.h"
#include <regex>
#include <string>
using std::string;

class Token {

public:
  Token();
  TokenType getTokenType() { return this->type; }

protected:
  std::regex pattern;
  TokenType type;

  Token &setMatchString(string &match);
  Token &setTokenType(TokenType &type);
};

#endif
