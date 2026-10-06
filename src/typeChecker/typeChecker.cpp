#include "typeChecker.h"
#include <cctype>
#include <stdexcept>

namespace {

[[noreturn]] void typeError(const std::string &message) {
  throw std::runtime_error("Type Error: " + message);
}

bool isNumLiteral(const Node *node) {
  if (!node->children.empty() || node->symbol.empty())
    return false;
  char c = node->symbol[0];
  return std::isdigit(static_cast<unsigned char>(c)) || c == '-';
}

// TERM -> NUM is the only place a NUM token can sit in the tree
void collectOperators(const Node *node, bool &hasMod, bool &hasDiv,
                      bool &hasDecimal) {
  if (node->symbol == "TERM" && !node->children.empty()) {
    const Node *first = node->children[0];
    if (first->symbol == "mod")
      hasMod = true;
    else if (first->symbol == "div")
      hasDiv = true;
    else if (node->children.size() == 1 && isNumLiteral(first) &&
             first->symbol.find('.') != std::string::npos)
      hasDecimal = true;
  }
  for (const Node *child : node->children)
    collectOperators(child, hasMod, hasDiv, hasDecimal);
}

} // namespace

std::string semTypeToString(SemType type) {
  switch (type) {
  case SemType::NUMERIC:
    return "numeric";
  case SemType::BOOLEAN:
    return "boolean";
  case SemType::PROCEDURE:
    return "procedure";
  case SemType::OK:
    return "ok";
  default:
    return "unknown";
  }
}

TypeChecker::TypeChecker(Node *root, SymbolTable &sym) : sym(sym) {
  if (!root)
    return;
  checkFloatIntegerConflict(root);
  checkProg(root);
}

SemType TypeChecker::typeOf(const Node *node) const {
  auto it = nodeTypes.find(node->id);
  return it != nodeTypes.end() ? it->second : SemType::UNKNOWN;
}

SemType TypeChecker::record(Node *node, SemType type) {
  nodeTypes[node->id] = type;
  return type;
}

void TypeChecker::expect(Node *node, const std::string &nonTerminal) {
  if (!node || node->symbol != nonTerminal)
    typeError("malformed syntax tree, expected " + nonTerminal + " but found " +
              (node ? node->symbol : "nothing"));
}

void TypeChecker::checkFloatIntegerConflict(Node *root) {
  bool hasMod = false, hasDiv = false, hasDecimal = false;
  collectOperators(root, hasMod, hasDiv, hasDecimal);

  if (hasMod && hasDiv)
    typeError("A float-integer-conflict might perhaps be possible");
  if (hasMod && hasDecimal)
    typeError("program uses mod, so no number may contain a decimal dot");
}

// SPL_PROG -> P $
SemType TypeChecker::checkProg(Node *node) {
  expect(node, "SPL_PROG");
  checkP(node->children[0]);
  return record(node, SemType::OK);
}

// P -> V_DECL : F_DECL : ALGO
SemType TypeChecker::checkP(Node *node) {
  expect(node, "P");
  checkVDecl(node->children[0]);
  checkFDecl(node->children[2]);
  checkAlgo(node->children[4]);
  return record(node, SemType::OK);
}

// V_DECL -> EPSILON | NAME V_DECL
SemType TypeChecker::checkVDecl(Node *node) {
  expect(node, "V_DECL");
  if (!node->children.empty()) {
    checkVDecl(node->children[1]);
    Node *name = node->children[0];
    sym.setNameType(name->symbol, SemType::NUMERIC);
    record(name, SemType::NUMERIC);
  }
  return record(node, SemType::OK);
}

// F_DECL -> EPSILON | F_TYPE F_DECL
SemType TypeChecker::checkFDecl(Node *node) {
  expect(node, "F_DECL");
  if (!node->children.empty()) {
    checkFType(node->children[0]);
    checkFDecl(node->children[1]);
  }
  return record(node, SemType::OK);
}

// F_TYPE -> void NAME ( V_DECL ) { P return }
// F_TYPE -> num NAME ( V_DECL ) { P return ( TERM ) }
SemType TypeChecker::checkFType(Node *node) {
  expect(node, "F_TYPE");
  Node *name = node->children[1];

  checkVDecl(node->children[3]);
  checkP(node->children[6]);

  SemType nameType = SemType::PROCEDURE;
  if (node->children[0]->symbol == "num") {
    if (checkTerm(node->children[9]) != SemType::NUMERIC)
      typeError("function '" + name->symbol +
                "' must return a numeric value");
    nameType = SemType::NUMERIC;
  }

  sym.setNameType(name->symbol, nameType);
  record(name, nameType);
  return record(node, SemType::OK);
}

// ALGO -> EPSILON | INSTR ; ALGO
SemType TypeChecker::checkAlgo(Node *node) {
  expect(node, "ALGO");
  if (!node->children.empty()) {
    checkInstr(node->children[0]);
    checkAlgo(node->children[2]);
  }
  return record(node, SemType::OK);
}

// INSTR -> print OUTP | nop | comment STRING | ASSIGN | BRANCH | LOOP | CALL
SemType TypeChecker::checkInstr(Node *node) {
  expect(node, "INSTR");
  Node *first = node->children[0];
  const std::string &kind = first->symbol;

  if (kind == "print") {
    checkOutp(node->children[1]);
  } else if (kind == "ASSIGN") {
    checkAssign(first);
  } else if (kind == "BRANCH") {
    checkBranch(first);
  } else if (kind == "LOOP") {
    checkLoop(first);
  } else if (kind == "CALL") {
    if (checkCall(first) != SemType::PROCEDURE)
      typeError("function '" + first->children[0]->symbol +
                "' returns a value and cannot be called as an instruction");
  }
  // nop and comment STRING are always ok
  return record(node, SemType::OK);
}

// OUTP -> ( TERM ) | STRING
SemType TypeChecker::checkOutp(Node *node) {
  expect(node, "OUTP");
  if (node->children.size() == 3 &&
      checkTerm(node->children[1]) != SemType::NUMERIC)
    typeError("print expects a numeric value or a string");
  return record(node, SemType::OK);
}

// CALL -> NAME ( INPUT )
SemType TypeChecker::checkCall(Node *node) {
  expect(node, "CALL");
  Node *name = node->children[0];

  checkInput(node->children[2]);

  SemType nameType = sym.getNameType(name->symbol);
  if (nameType == SemType::UNKNOWN)
    typeError("function '" + name->symbol + "' has no known type");

  record(name, nameType);
  return record(node, nameType);
}

// INPUT -> EPSILON | TERM INPUT
SemType TypeChecker::checkInput(Node *node) {
  expect(node, "INPUT");
  if (!node->children.empty()) {
    if (checkTerm(node->children[0]) != SemType::NUMERIC)
      typeError("function arguments must be numeric");
    checkInput(node->children[1]);
  }
  return record(node, SemType::OK);
}

// ASSIGN -> NAME = TERM
SemType TypeChecker::checkAssign(Node *node) {
  expect(node, "ASSIGN");
  Node *name = node->children[0];

  if (sym.getNameType(name->symbol) != SemType::NUMERIC)
    typeError("cannot assign to '" + name->symbol +
              "', it is not a numeric variable");
  record(name, SemType::NUMERIC);

  if (checkTerm(node->children[2]) != SemType::NUMERIC)
    typeError("right-hand side of assignment to '" + name->symbol +
              "' must be numeric");
  return record(node, SemType::OK);
}

// TERM -> NAME | NUM | CALL
// TERM -> neg ( TERM )
// TERM -> add|sub|mul|div|mod ( TERM TERM )
SemType TypeChecker::checkTerm(Node *node) {
  expect(node, "TERM");
  Node *first = node->children[0];

  if (node->children.size() == 1) {
    if (first->symbol == "CALL") {
      if (checkCall(first) != SemType::NUMERIC)
        typeError("procedure '" + first->children[0]->symbol +
                  "' returns no value and cannot be used as a term");
    } else if (isNumLiteral(first)) {
      record(first, SemType::NUMERIC);
    } else {
      if (sym.getNameType(first->symbol) != SemType::NUMERIC)
        typeError("'" + first->symbol + "' is not a numeric variable");
      record(first, SemType::NUMERIC);
    }
    return record(node, SemType::NUMERIC);
  }

  const std::string &op = first->symbol;
  if (op == "neg") {
    if (checkTerm(node->children[2]) != SemType::NUMERIC)
      typeError("neg expects a numeric operand");
  } else {
    if (checkTerm(node->children[2]) != SemType::NUMERIC ||
        checkTerm(node->children[3]) != SemType::NUMERIC)
      typeError(op + " expects two numeric operands");
  }
  return record(node, SemType::NUMERIC);
}

// BRANCH -> if BOOL then { ALGO } else { ALGO }
SemType TypeChecker::checkBranch(Node *node) {
  expect(node, "BRANCH");
  if (checkBool(node->children[1]) != SemType::BOOLEAN)
    typeError("if condition must be boolean");
  checkAlgo(node->children[4]);
  checkAlgo(node->children[8]);
  return record(node, SemType::OK);
}

// BOOL -> not ( BOOL )
// BOOL -> and|or ( BOOL BOOL )
// BOOL -> eq|larger|lesser ( TERM TERM )
SemType TypeChecker::checkBool(Node *node) {
  expect(node, "BOOL");
  const std::string &op = node->children[0]->symbol;

  if (op == "not") {
    if (checkBool(node->children[2]) != SemType::BOOLEAN)
      typeError("not expects a boolean operand");
  } else if (op == "and" || op == "or") {
    if (checkBool(node->children[2]) != SemType::BOOLEAN ||
        checkBool(node->children[3]) != SemType::BOOLEAN)
      typeError(op + " expects two boolean operands");
  } else {
    if (checkTerm(node->children[2]) != SemType::NUMERIC ||
        checkTerm(node->children[3]) != SemType::NUMERIC)
      typeError(op + " expects two numeric operands");
  }
  return record(node, SemType::BOOLEAN);
}

// LOOP -> COND BOOL do { ALGO }
// LOOP -> do { ALGO } COND BOOL
SemType TypeChecker::checkLoop(Node *node) {
  expect(node, "LOOP");
  Node *cond, *boolNode, *algo;
  if (node->children[0]->symbol == "COND") {
    cond = node->children[0];
    boolNode = node->children[1];
    algo = node->children[4];
  } else {
    algo = node->children[2];
    cond = node->children[4];
    boolNode = node->children[5];
  }

  checkCond(cond);
  if (checkBool(boolNode) != SemType::BOOLEAN)
    typeError("loop condition must be boolean");
  checkAlgo(algo);
  return record(node, SemType::OK);
}

// COND -> while | until
SemType TypeChecker::checkCond(Node *node) {
  expect(node, "COND");
  return record(node, SemType::OK);
}
