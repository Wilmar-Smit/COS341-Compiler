#include "parser.actions.h"
#include "../ParserVisitor/parser.visitor.h"

void ShiftAction::AcceptVisitor(ParseVisitor *vis) { vis->visit(this); }

void ReduceAction::AcceptVisitor(ParseVisitor *vis) { vis->visit(this); }

void ErrorAction::AcceptVisitor(ParseVisitor *vis) { vis->visit(this); }

void AcceptAction::AcceptVisitor(ParseVisitor *vis) { vis->visit(this); }

void GotoAction::AcceptVisitor(ParseVisitor *vis) { vis->visit(this); }
