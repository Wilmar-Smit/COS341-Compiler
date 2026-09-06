#include "token.h"

Token::Token() : pattern("") {}

Token &Token::setMatchString(string &match) {
  pattern = std::regex(match, std::regex::ECMAScript | std::regex::optimize);
  return *this;
}

Token &Token::setTokenType(TokenType &type) {
  this->type = type;
  return *this;
}
