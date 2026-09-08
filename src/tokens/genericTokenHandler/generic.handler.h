#ifndef GENERIC_HANDLER_H
#define GENERIC_HANDLER_H

#include "token.enum.h"
#include "tokenHandler.h"
#include "tokenTable.h"
#include <iostream>
using std::cout;
using std::endl;
class GenericHandler : public TokenHandler {

public:
  GenericHandler(TokenType token) : TokenHandler() {
    cout << "Created a handler for " << patternFor(token) << endl;

    this->setMatchString(patternFor(token)).setTokenType(token);
  }

private:
  TokenResult handleFunc(string matchedString, string restOfStream) override {
    return {.token = new Token(matchedString, this->type),
            .remainingStream = restOfStream};
  }
};

TokenHandler *createChain() {

  TokenHandler *start = nullptr;

  for (std::size_t i = 0; i < TOKEN_TYPE_COUNT; ++i) {

    TokenType type =
        static_cast<TokenType>(i); // gets us the token type by enum

    if (start == nullptr) {
      start = new GenericHandler(type);
    } else {
      start->SetNext(new GenericHandler(type));
    }
  }
  return start;
}

#endif
