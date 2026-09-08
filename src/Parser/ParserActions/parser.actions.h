#ifndef PARSE_ACTIONS_H
#define PARSE_ACTIONS_H

#include "../ParserVisitor/parser.visitor.h"
class ParserAction {
public:
  ParserAction() {}
  virtual void AcceptVisitor(ParseVisitor vis) = 0;
};

// Stores the index of the next state to go onto the stack
class ShiftAction : public ParserAction {
public:
  ShiftAction() {}
  virtual void AcceptVisitor(ParseVisitor vis);
};

// Stores information needed to delete from the stack as well as which index
// from goto table to hit
class ReduceAction : public ParserAction {
public:
  ReduceAction() {}
  virtual void AcceptVisitor(ParseVisitor vis);
};

// this is the default of the table basically if this then something went wrong
class ErrorAction : public ParserAction {
public:
  ErrorAction() {}
  virtual void AcceptVisitor(ParseVisitor vis);
};

// If this then yay we parsed
class AcceptAction : public ParserAction {
public:
  AcceptAction() {}
  virtual void AcceptVisitor(ParseVisitor vis);
};

// stores next index to lookup in the table
class GotoAction : public ParserAction {
public:
  GotoAction() {}
  virtual void AcceptVisitor(ParseVisitor vis);
};

#endif
