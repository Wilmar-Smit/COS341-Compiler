#ifndef PARSE_VISITOR_H
#define PARSE_VISITOR_H

class ParserAction;
class ReduceAction;
class AcceptAction;
class ShiftAction;
class GotoAction;

class ParseVisitor {
public:
  ParseVisitor();
  virtual void visit(ParserAction *action);
  virtual void visit(ReduceAction *action);
  virtual void visit(ShiftAction *action);
  virtual void visit(GotoAction *action);
  virtual void visit(AcceptAction *action);
};

#endif
