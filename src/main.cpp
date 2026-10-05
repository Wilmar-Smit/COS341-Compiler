#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Parser.h"
#include "fileReader/filereader.h"
#include "fileReader/parseTableReader.h"
#include "scopeTree/scopeTree.h"
#include "token.h"
#include "tokenHandler.h"
#include "tokens/genericTokenHandler/generic.handler.h"

const std::string GREEN = "\033[32m";
const std::string RED = "\033[31m";
const std::string RESET = "\033[0m";

int main(int argc, char *argv[]) {
  std::string filename = "input.txt";
  if (argc > 1) {
    filename = argv[1];
  }

  std::cout << "\n------------ STARTING COMPILATION ------------\n"
            << std::endl;
  std::cout << "Target File: " << filename << std::endl;

  // ------------ LEXICAL ANALYSIS ------------

  TokenHandler *chain = createChain();
  std::vector<Token *> tokenList;
  std::string stream;

  try {
    stream = readFileToString(filename);
  } catch (const std::exception &e) {
    std::cerr << RED << "Error reading file: " << e.what() << RESET
              << std::endl;
    delete chain;
    return 1;
  }

  bool lexingFailed = false;
  bool parsingPassed = false;
  Parser parse;

  try {
    while (!stream.empty()) {
      auto res = chain->handle(stream);
      tokenList.push_back(res.token);
      stream = res.remainingStream;
    }
  } catch (const std::runtime_error &e) {
    std::cerr << RED << e.what() << RESET << std::endl;
    std::cerr << RED << "COMPILING FAILED AT LEXER STAGE" << RESET << std::endl;
    lexingFailed = true;
  }

  // ------------ PARSING STAGE ------------

  if (!lexingFailed) {
    ActionTableReader *tableReader =
        new ActionTableReader("ProductionRules.txt");
    auto actionTable = tableReader->read("SLR_ACTION_Table.csv");

    tokenList.push_back(new Token("$", TokenType::DOLLAR_EOF));

    parsingPassed = parse.ParseTokens(tokenList);

    for (auto &row : actionTable) {
      for (auto action : row) {
        delete action;
      }
    }
    delete tableReader;
  }

  // ------------ SEMANTIC ANALYSIS (PHASE 2a) ------------

  if (parsingPassed) {
    TreeBuilder *tree = parse.getTree();
    if (tree) {
      ScopeTree scopeTree(*tree);
      bool scopeValid = scopeTree.validateScopes();

      if (scopeValid) {
        std::cout << GREEN << "SCOPE RESOLUTION & SEMANTIC ANALYSIS PASSED"
                  << RESET << std::endl;
      } else {
        std::cout << RED << "SCOPE RESOLUTION FAILED" << RESET << std::endl;
      }
    }
  }

  std::cout << "\n------------ ENDING COMPILATION ------------\n" << std::endl;

  // ------------ MEMORY MANAGEMENT ------------

  if (parse.getTree()) {
    delete parse.getTree();
  }

  for (auto token : tokenList) {
    delete token;
  }
  delete chain;

  return (lexingFailed || !parsingPassed) ? 1 : 0;
}
