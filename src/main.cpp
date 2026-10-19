#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Parser.h"
#include "codegen/codeGen.h"
#include "fileReader/filereader.h"
#include "fileReader/parseTableReader.h"
#include "scopeTree/scopeTree.h"
#include "typeChecker/typeChecker.h"
#include "token.h"
#include "tokenHandler.h"
#include "tokens/genericTokenHandler/generic.handler.h"

const std::string GREEN = "\033[32m";
const std::string RED = "\033[31m";
const std::string YELLOW = "\033[33m";
const std::string CYAN = "\033[36m";
const std::string RESET = "\033[0m";

enum class CodeGenResult { Generated, Skipped, Failed };

// Skipping on CodeGenUnsupported is temporary: until Phase 3b, valid programs
// with functions must keep compiling with exit code 0.
CodeGenResult runCodeGeneration(Node *root, const std::string &numType,
                                const std::string &outputName) {
  try {
    CodeGen codeGen(numType);
    std::string generated = codeGen.genProgram(root);

    std::ofstream outputFile(outputName);
    if (!outputFile.is_open()) {
      throw std::runtime_error("cannot open output file: " + outputName);
    }
    outputFile << generated << "\n";
    outputFile.close();
    std::cout << GREEN << "[STAGE 5] Code Generation Passed. Output written to "
              << outputName << "." << RESET << std::endl;
    return CodeGenResult::Generated;
  } catch (const CodeGenUnsupported &e) {
    std::cout << YELLOW << "[STAGE 5] Code generation skipped: " << e.what()
              << RESET << std::endl;
    return CodeGenResult::Skipped;
  } catch (const std::exception &e) {
    std::cerr << RED << "[STAGE 5] Code Generation Failed: " << e.what()
              << RESET << std::endl;
    return CodeGenResult::Failed;
  }
}

int main(int argc, char* argv[]) {
  std::string filename = argc > 1 ? argv[1] : "input.txt";

  std::cout << "\n==============================================" << std::endl;
  std::cout << "          STARTING SPL COMPILATION          " << std::endl;
  std::cout << "==============================================" << std::endl;
  std::cout << "Target Source File: " << CYAN << filename << RESET << std::endl;

  // ------------ STAGE 1: LEXICAL ANALYSIS ------------
  TokenHandler* chain = createChain();
  std::vector<Token*> tokenList;
  std::string stream;

  try {
    stream = readFileToString(filename);
    while (!stream.empty()) {
      auto res = chain->handle(stream);
      tokenList.push_back(res.token);
      stream = res.remainingStream;
    }
    std::cout << GREEN << "[STAGE 1] Lexical Analysis Passed ("
      << tokenList.size() << " tokens)." << RESET << std::endl;
  }
  catch (const std::exception& e) {
    std::cerr << RED << "[STAGE 1] Lexical Analysis Failed: " << e.what()
      << RESET << std::endl;
    delete chain;
    return 1;
  }

  // ------------ STAGE 2: SYNTAX ANALYSIS ------------
  ActionTableReader* tableReader = new ActionTableReader("ProductionRules.txt");
  auto actionTable = tableReader->read("SLR_ACTION_Table.csv");
  tokenList.push_back(new Token("$", TokenType::DOLLAR_EOF));

  Parser parse;
  bool parsingPassed = parse.ParseTokens(tokenList);

  for (auto& row : actionTable) {
    for (auto action : row) {
      delete action;
    }
  }
  delete tableReader;

  if (!parsingPassed) {
    std::cerr << RED << "[STAGE 2] Syntax Analysis Failed." << RESET
      << std::endl;
    // Cleanup
    for (auto token : tokenList)
      delete token;
    delete chain;
    return 1;
  }
  std::cout << GREEN << "[STAGE 2] Syntax Analysis Passed." << RESET
    << std::endl;

  // ------------ STAGE 3: SEMANTIC ANALYSIS ------------
  TreeBuilder* tree = parse.getTree();
  if (!tree || !tree->getRoot()) {
    std::cerr << RED << "[STAGE 3] Semantic Analysis Failed: Empty CST root."
      << RESET << std::endl;
    for (auto token : tokenList)
      delete token;
    delete chain;
    return 1;
  }

  // typeChecker holds a reference into scopeTree, so delete it first.
  ScopeTree* scopeTree = nullptr;
  TypeChecker* typeChecker = nullptr;

  int semanticStage = 3;
  try {
    scopeTree = new ScopeTree(*tree);
    tree->writeXML(tree->getRoot());
    std::cout << GREEN
      << "[STAGE 3] Semantic Analysis & Scope Resolution Passed."
      << RESET << std::endl;

    // ------------ STAGE 4: TYPE ANALYSIS ------------
    semanticStage = 4;
    typeChecker = new TypeChecker(tree->getRoot(), scopeTree->getSymbolTable());
    bool isInteger =
      scopeTree->getSymbolTable().getNumberMode() == NumberMode::INTEGER;
    std::cout << GREEN << "[STAGE 4] Type Analysis Passed ("
      << (isInteger ? "integer" : "float") << " program)." << RESET
      << std::endl;
  }
  catch (const std::exception& e) {
    std::cerr << RED << "[STAGE " << semanticStage
      << "] Semantic Error: " << e.what() << RESET << std::endl;
    std::cout << "\n=============================================="
      << std::endl;
    std::cout << RED << "            COMPILATION FAILED                "
      << RESET << std::endl;
    std::cout << "==============================================\n"
      << std::endl;

    delete typeChecker;
    delete scopeTree;
    if (parse.getTree())
      delete parse.getTree();
    for (auto token : tokenList)
      delete token;
    delete chain;
    return 1;
  }

  // ------------ STAGE 5: CODE GENERATION ------------
  std::string outputName = argc > 2 ? argv[2] : "output.cpp";
  std::string numType =
      numTypeFromMode(scopeTree->getSymbolTable().getNumberMode());
  if (runCodeGeneration(tree->getRoot(), numType, outputName) ==
      CodeGenResult::Failed) {
    delete typeChecker;
    delete scopeTree;
    delete tree;
    for (auto* token : tokenList) {
      delete token;
    }
    delete chain;
    return 1;
    return 1;
  }

  std::cout << "\n==============================================" << std::endl;
  std::cout << GREEN << "          COMPILATION COMPLETED SUCCESSFULLY  "
    << RESET << std::endl;
  std::cout << "==============================================\n" << std::endl;

  // ------------ MEMORY MANAGEMENT ------------
  delete typeChecker;
  delete scopeTree;
  if (parse.getTree())
    delete parse.getTree();
  for (auto token : tokenList)
    delete token;
  delete chain;

  return 0;
}
