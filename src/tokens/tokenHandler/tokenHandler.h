#ifndef TOKEN_HANDLER_H
#define TOKEN_HANDLER_H

#include "../token.h"
#include "token.enum.h"
#include <regex>
#include <string>

using std::string;

struct TokenResult {
  Token *token = nullptr;
  std::string remainingStream;
};

class TokenHandler {
public:
  TokenHandler();
  virtual ~TokenHandler() {
    if (next) {
      delete next;
    }
  }

  virtual TokenHandler *SetNext(TokenHandler *handler);

  virtual TokenResult handle(string stream);

  // Template method receiving matched lexeme ("mod") and unconsumed stream ("
  // (")
  virtual TokenResult handleFunc(string matchedString, string restOfStream) = 0;

  TokenHandler &setMatchString(string match);
  TokenHandler &setTokenType(TokenType type);

protected:
  std::regex pattern;
  TokenHandler *next = nullptr;
  TokenType type = TokenType::MOD;
};

class ExampleHandler : public TokenHandler {
public:
  ExampleHandler() : TokenHandler() {
    this->setMatchString(R"(^mod$)").setTokenType(TokenType::MOD);
  }

  TokenResult handleFunc(string matchedString, string restOfStream) override {
    return {new Token(matchedString, this->type), restOfStream};
  }
};

#endif
