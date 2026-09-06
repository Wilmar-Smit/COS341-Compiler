#include "tokenHandler.h"

TokenHandler *TokenHandler::SetNext(TokenHandler *tokenHandler) {
  if (tokenHandler) {
    if (next) {
      next->SetNext(tokenHandler);
    } else {
      this->next = tokenHandler;
    }
  }
  return this;
}
