#ifndef TOKEN_HANDLER_H
#define TOKEN_HANDLER_H
#include "../token.h"
#include <regex>
#include <string>
using std::string;
// uses chain of responsibility
// uses builder
// uses template method
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
  virtual TokenResult handleFunc(string stream) = 0; // the template method

protected:
  std::regex pattern;
  TokenHandler *next = nullptr;

  TokenHandler &setMatchString(string match);
};

class ExampleHandler : public TokenHandler {
public:
  ExampleHandler() : TokenHandler() { this->setMatchString(R"(^mod$)"); }

  TokenResult handleFunc(std::string matchedString) override {
    return {new Token(matchedString), ""};
  }
};

#endif
