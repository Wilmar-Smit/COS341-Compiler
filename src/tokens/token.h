#ifndef TOKEN_H
#define TOKEN_H
#include "token.enum.h"

#include <string>
using std::string;
class Token {

public:
  Token(string str, TokenType type) { this->code = str; }
  TokenType getType() { return this->type; }
  string getCode() { return this->code; }

protected:
  string code; // the code the token will store that its regex was matched with
  TokenType type;
};

#endif
