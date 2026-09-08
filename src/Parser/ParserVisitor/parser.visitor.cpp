#include "parser.visitor.h"
#include "parser.actions.h"
#include <stdexcept>

void ParseVisitor::visit(ParserAction *action) {
  throw std::runtime_error("Visit should not be called on this abstract class");
}
void ParseVisitor::visit(ReduceAction *action) {
    
}
void ParseVisitor::visit(ShiftAction *action) {
    
}
void ParseVisitor::visit(GotoAction *action) {
    
}
void ParseVisitor::visit(AcceptAction *action) {
    
}
