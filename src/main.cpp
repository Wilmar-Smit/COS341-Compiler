
#include "fileReader/filereader.h"
#include "token.h"
#include "tokenHandler.h"
#include "tokens/genericTokenHandler/generic.handler.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
const std::string GREEN = "\033[32m";
const std::string RED = "\033[31m";
const std::string RESET = "\033[0m";

int main() {

  TokenHandler *chain = createChain();

  std::vector<Token *> tokenList;

  string stream = readFileToString("input.txt");
  cout << stream << endl;

  try {
    while (stream != "") {
      auto res = chain->handle(stream);
      tokenList.push_back(res.token);
      stream = res.remainingStream;
      cout << "Remaining stream : [" << stream << "]\n" << endl;
    }
  } catch (std::runtime_error e) {
    cout << RED << e.what() << RESET << endl;
    cout << RED << "COMPILING FAILED LEXER STAGE:" << RESET << endl;
  }

  // ------------ MEMORY MANAGEMENT ------------

  for (auto token : tokenList) {
    cout << GREEN << "Token code :[" << token->getCode() << "]" << RESET
         << endl;
    delete token;
  }

  delete chain;
  return 0;
}
