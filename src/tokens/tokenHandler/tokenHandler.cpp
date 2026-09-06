#include "tokenHandler.h"
#include <stdexcept>

TokenHandler::TokenHandler() : next(nullptr) {}

TokenHandler &TokenHandler::setMatchString(string match) {
  this->pattern =
      std::regex(match, std::regex::ECMAScript | std::regex::optimize);
  return *this;
}

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

TokenResult TokenHandler::handle(string stream) {
  if (stream.empty()) {
    return {nullptr, ""};
  }

  size_t spacePos = stream.find(' ');

  string candidateTokenStr;
  string remaining;

  if (spacePos != string::npos) {
    candidateTokenStr = stream.substr(0, spacePos);
    remaining = stream.substr(spacePos);
  } else {
    candidateTokenStr = stream;
    remaining = ""; // can maybe make this $ for eof
  }

  if (std::regex_match(candidateTokenStr, this->pattern)) {
    return this->handleFunc(candidateTokenStr,
                            remaining); // i should return the my token
  }

  if (this->next) {
    return this->next->handle(stream); // my next should handle this
  }

  throw std::runtime_error("Unexpected token symbol: '" + candidateTokenStr +
                           "'");
}
