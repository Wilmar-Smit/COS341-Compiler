#ifndef TOKEN_H
#define TOKEN_H
#include "token.enum.h"

#include <string>
using std::string;
class Token {

public:
  Token(string str) {
    this->code = str;
  } // the token not including the spaces " mod " == "mod"
  TokenType getType() { return this->type; }

protected:
  string code; // the code the token will store that its regex was matched with
  TokenType type;
};

#endif
