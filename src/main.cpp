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
const std::string YELLOW = "\033[33m";
const std::string CYAN = "\033[36m";
const std::string RESET = "\033[0m";

int main(int argc, char *argv[]) {
  std::string filename = "input.txt";
  if (argc > 1) {
    filename = argv[1];
  }

  std::cout << "\n==============================================" << std::endl;
  std::cout << "          STARTING SPL COMPILATION            " << std::endl;
  std::cout << "==============================================" << std::endl;
  std::cout << "Target Source File: " << CYAN << filename << RESET << std::endl;

  // ------------ LEXICAL ANALYSIS ------------
  std::cout << "\n"
            << YELLOW << "[STAGE 1] Lexical Analysis (Scanning)..." << RESET
            << std::endl;

  TokenHandler *chain = createChain();
  std::vector<Token *> tokenList;
  std::string stream;

  try {
    stream = readFileToString(filename);
    std::cout << " -> File loaded successfully (" << stream.length()
              << " characters)." << std::endl;
  } catch (const std::exception &e) {
    std::cerr << RED << " -> Error reading file: " << e.what() << RESET
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
    std::cout << GREEN << " -> Lexical Analysis Complete. Tokens Generated: "
              << tokenList.size() << RESET << std::endl;
  } catch (const std::runtime_error &e) {
    std::cerr << RED << " -> " << e.what() << RESET << std::endl;
    std::cerr << RED << " -> COMPILING FAILED AT LEXER STAGE" << RESET
              << std::endl;
    lexingFailed = true;
  }

  // ------------ PARSING STAGE ------------
  if (!lexingFailed) {
    std::cout << "\n"
              << YELLOW << "[STAGE 2] Syntax Analysis (SLR Parsing)..." << RESET
              << std::endl;

    std::cout << " -> Loading SLR Action Table & Production Rules..."
              << std::endl;
    ActionTableReader *tableReader =
        new ActionTableReader("ProductionRules.txt");
    auto actionTable = tableReader->read("SLR_ACTION_Table.csv");

    tokenList.push_back(new Token("$", TokenType::DOLLAR_EOF));
    std::cout << " -> Appended EOF token ($)." << std::endl;

    std::cout << " -> Executing SLR Shift-Reduce Parsing..." << std::endl;
    parsingPassed = parse.ParseTokens(tokenList);

    if (parsingPassed) {
      std::cout << GREEN
                << " -> Syntax Analysis Complete. CST Successfully Constructed."
                << RESET << std::endl;
    } else {
      std::cerr
          << RED
          << " -> Syntax Analysis Failed. Input program violates SPL grammar."
          << RESET << std::endl;
    }

    for (auto &row : actionTable) {
      for (auto action : row) {
        delete action;
      }
    }
    delete tableReader;
  }

  // ------------ SEMANTIC ANALYSIS (PHASE 2a) ------------
  if (parsingPassed) {
    std::cout << "\n"
              << YELLOW
              << "[STAGE 3] Semantic Analysis & Scope Resolution (Phase 2a)..."
              << RESET << std::endl;

    TreeBuilder *tree = parse.getTree();
    if (tree && tree->getRoot()) {
      std::cout << " -> Concrete Syntax Tree Root Node ID: "
                << tree->getRoot()->id << std::endl;
      std::cout
          << " -> Traversing CST to build Symbol Table & resolve identifiers..."
          << std::endl;

      try {
        ScopeTree scopeTree(*tree);

        std::cout << " -> Writing transformed CST with unique internal "
                     "identifiers to XML..."
                  << std::endl;
        tree->writeXML(tree->getRoot());

        std::cout << GREEN
                  << " -> Scope Resolution & Unique Renaming Succeeded."
                  << RESET << std::endl;
        std::cout << GREEN
                  << " -> Transformed AST output written to 'tree.xml'."
                  << RESET << std::endl;
      } catch (const std::runtime_error &e) {
        std::cerr << RED << " -> Semantic Error Detected: " << e.what() << RESET
                  << std::endl;
        std::cerr << RED << " -> COMPILING FAILED AT SEMANTIC STAGE" << RESET
                  << std::endl;
      }
    } else {
      std::cerr << RED << " -> CST generation yielded an empty tree root!"
                << RESET << std::endl;
    }
  }

  std::cout << "\n==============================================" << std::endl;
  if (!lexingFailed && parsingPassed) {
    std::cout << GREEN << "         COMPILATION COMPLETED SUCCESSFULLY    "
              << RESET << std::endl;
  } else {
    std::cout << RED << "            COMPILATION FAILED                "
              << RESET << std::endl;
  }
  std::cout << "==============================================\n" << std::endl;

  // ------------ MEMORY MANAGEMENT ------------
  std::cout << " -> Cleaning up allocated heap memory..." << std::endl;
  if (parse.getTree()) {
    delete parse.getTree();
  }

  for (auto token : tokenList) {
    delete token;
  }
  delete chain;
  std::cout << " -> Memory cleanup finalized." << std::endl;

  return (lexingFailed || !parsingPassed) ? 1 : 0;
}
