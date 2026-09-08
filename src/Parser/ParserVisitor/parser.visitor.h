#ifndef PARSE_VISITOR_H
#define PARSE_VISITOR_H

#include <stack>
#include <vector>
using std::stack;
using std::vector;
#include "../StateEnums/state.enum.h"
class ParserAction;
class ReduceAction;
class AcceptAction;
class ShiftAction;
class GotoAction;

class ParseVisitor {
private:
  vector<vector<ParserAction *>> &table; // gets filled in by the function
  stack<ParserStates> &stack;

public:
  ParseVisitor(vector<vector<ParserAction *>> &table,
               std::stack<ParserStates> &stack);
  virtual void visit(ParserAction *action);
  virtual void visit(ReduceAction *action);
  virtual void visit(ShiftAction *action);
  virtual void visit(GotoAction *action);
  virtual void visit(AcceptAction *action);
};

#endif
