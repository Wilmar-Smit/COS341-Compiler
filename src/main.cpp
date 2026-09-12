
#include "fileReader/filereader.h"
#include "token.h"
#include "tokenHandler.h"
#include "tokens/genericTokenHandler/generic.handler.h"
#include "fileReader/parseTableReader.h"
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

  // ------------ LOADING ACTION TABLE + BASIC TESTS ------------
  
  ActionTableReader *tableReader = new ActionTableReader("ProductionRules.txt");
  auto actionTable = tableReader->read("SLR_ACTION_Table.csv");

  cout << "Action table rows: " << actionTable.size() << endl;
  cout << "Action table cols (row 0): " << actionTable[0].size() << endl;

  // state 0 on NAME should shift (V_DECL -> NAME V_DECL)
  bool shiftOk = dynamic_cast<ShiftAction *>(actionTable[0][(int)TokenType::NAME]) != nullptr;
  cout << (shiftOk ? GREEN : RED) << "state 0, NAME -> ShiftAction : "
       << (shiftOk ? "PASS" : "FAIL") << RESET << endl;

  // state 0 on RPAREN/COLON should reduce (V_DECL -> EPSILON)
  bool reduceOk = dynamic_cast<ReduceAction *>(actionTable[0][(int)TokenType::RPAREN]) != nullptr &&
                  dynamic_cast<ReduceAction *>(actionTable[0][(int)TokenType::COLON]) != nullptr;
  cout << (reduceOk ? GREEN : RED) << "state 0, RPAREN/COLON -> ReduceAction : "
       << (reduceOk ? "PASS" : "FAIL") << RESET << endl;

  // state 1 on $ should accept
  bool acceptOk = dynamic_cast<AcceptAction *>(actionTable[1][(int)TokenType::DOLLAR_EOF]) != nullptr;
  cout << (acceptOk ? GREEN : RED) << "state 1, $ -> AcceptAction : "
       << (acceptOk ? "PASS" : "FAIL") << RESET << endl;

  // a column with no entry at state 0 (e.g. LBRACE) should default to ErrorAction
  bool errorOk = dynamic_cast<ErrorAction *>(actionTable[0][(int)TokenType::LBRACE]) != nullptr;
  cout << (errorOk ? GREEN : RED) << "state 0, LBRACE -> ErrorAction : "
       << (errorOk ? "PASS" : "FAIL") << RESET << endl;

  // ------------ MEMORY MANAGEMENT ------------

  for (auto &row : actionTable) {
    for (auto action : row) {
      delete action;
    }
  }
  delete tableReader;
  for (auto token : tokenList) {
    cout << GREEN << "Token code :[" << token->getCode() << "]" << RESET
         << endl;
    delete token;
  }

  delete chain;
  return 0;
}
