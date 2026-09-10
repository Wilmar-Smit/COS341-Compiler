#ifndef PARSE_VISITOR_H
#define PARSE_VISITOR_H

#include "token.h"
#include <stack>
#include <vector>
using std::stack;
using std::vector;
#include "../StateEnums/state.enum.h"
#include "TreeBuilder.h"

class ParserAction;
class ReduceAction;
class AcceptAction;
class ShiftAction;
class GotoAction;
class ErrorAction;

class ParseVisitor {
private:
  vector<vector<ParserAction *>> &table;
  vector<vector<ParserAction *>> &gotoTable;
  stack<ParserStates> &stack;
  vector<Token *> tokens;
  int tokenIndex = 0;
  int StateIndex = 0;
  bool hitAcceptState = false;

  TreeBuilder treeBuilder;

public:
  ParseVisitor(vector<vector<ParserAction *>> &table,
               vector<vector<ParserAction *>> &gotoTable,
               std::stack<ParserStates> &stack, TreeBuilder& tb);

  bool parseTokens(vector<Token *> tokens);

  virtual void visit(ParserAction *action);
  virtual void visit(ReduceAction *action);
  virtual void visit(ShiftAction *action);
  virtual void visit(GotoAction *action);
  virtual void visit(AcceptAction *action);
  virtual void visit(ErrorAction *action);
};

#endif
