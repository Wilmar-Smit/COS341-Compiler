
#include "fileReader/filereader.h"
#include "token.h"
#include "tokenHandler.h"
#include "tokens/genericTokenHandler/generic.handler.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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
    cout << e.what() << endl;
  }

  for (auto token : tokenList) {
    cout << "Token code :[" << token->getCode() << "]" << endl;
  }

  delete chain;
  return 0;
}
