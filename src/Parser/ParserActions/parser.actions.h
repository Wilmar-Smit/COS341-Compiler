#ifndef PARSE_ACTIONS_H
#define PARSE_ACTIONS_H

#include "../Parser/ProductionRules.h"
#include "../ParserVisitor/parser.visitor.h"
#include "state.enum.h"
class ParserAction {
  friend class ParseVisitor;

public:
  ParserAction() {}
  virtual ~ParserAction() {}
  virtual void AcceptVisitor(ParseVisitor *vis) = 0;
};

// Stores the index of the next state to go onto the stack
class ShiftAction : public ParserAction {
  friend class ParseVisitor;

  ParserStates state;

private:
  ProductionRule rule;

public:
  // simple just stores the next state to put onto the stack
  ShiftAction(ParserStates state) : state(state) {}
  virtual void AcceptVisitor(ParseVisitor *vis);
};

// Stores information needed to delete from the stack as well as which index
// from goto table to hit

class ReduceAction : public ParserAction {
  friend class ParseVisitor;

private:
  ProductionRule rule;

public:
  ReduceAction(ProductionRule rule) : rule(rule) {}
  virtual void AcceptVisitor(ParseVisitor *vis);
};

// this is the default of the table basically if this then something went wrong
class ErrorAction : public ParserAction {
  friend class ParseVisitor;

public:
  ErrorAction() {}
  virtual void AcceptVisitor(ParseVisitor *vis);
};

// If this then yay we parsed
class AcceptAction : public ParserAction {
  friend class ParseVisitor;

public:
  AcceptAction() {}
  virtual void AcceptVisitor(ParseVisitor *vis);
};

// stores next index to lookup in the table
class GotoAction : public ParserAction {
  friend class ParseVisitor;
  ParserStates state;

public:
  GotoAction(ParserStates state) : state(state) {}
  virtual void AcceptVisitor(ParseVisitor *vis);
};

#endif
