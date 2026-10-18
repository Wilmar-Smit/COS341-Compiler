#include "codeGen.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>

namespace {

constexpr const char *NEWLINE = "\n";

constexpr std::size_t ALGO_SEQUENCE_SIZE = 3;
constexpr std::size_t ALGO_INSTR = 0;
constexpr std::size_t ALGO_SEMI = 1;
constexpr std::size_t ALGO_REST = 2;

constexpr std::size_t KEYWORD_ONLY_SIZE = 1;
constexpr std::size_t KEYWORD_ARG_SIZE = 2;
constexpr std::size_t OUTP_TERM_SIZE = 3;
constexpr std::size_t OUTP_TERM = 1;

constexpr std::size_t ASSIGN_SIZE = 3;
constexpr std::size_t ASSIGN_NAME = 0;
constexpr std::size_t ASSIGN_EQUALS = 1;
constexpr std::size_t ASSIGN_TERM = 2;

constexpr std::size_t BOOL_NOT_SIZE = 4;
constexpr std::size_t BOOL_BINARY_SIZE = 5;
constexpr std::size_t BOOL_FIRST_OPERAND = 2;
constexpr std::size_t BOOL_SECOND_OPERAND = 3;

constexpr std::size_t BRANCH_SIZE = 10;
constexpr std::size_t BRANCH_BOOL = 1;
constexpr std::size_t BRANCH_THEN = 4;
constexpr std::size_t BRANCH_ELSE = 8;
const std::array<std::pair<std::size_t, const char *>, 7> BRANCH_FIXED = {{
    {0, "if"}, {2, "then"}, {3, "{"}, {5, "}"}, {6, "else"}, {7, "{"},
    {9, "}"}}};

constexpr std::size_t TERM_LEAF_SIZE = 1;
constexpr std::size_t TERM_UNARY_SIZE = 4;
constexpr std::size_t TERM_BINARY_SIZE = 5;
constexpr std::size_t TERM_FIRST_OPERAND = 2;
constexpr std::size_t TERM_SECOND_OPERAND = 3;

const char *const PLANNED_LOOP = "step 4";
const char *const PLANNED_CALL = "Phase 3b (function calls via inlining)";

[[noreturn]] void malformed(const std::string &message) {
  throw std::logic_error("CodeGen: malformed syntax tree, " + message);
}

void expectNode(const Node *node, const std::string &nonTerminal) {
  if (node == nullptr)
    malformed("expected " + nonTerminal + " but found nothing");
  if (node->symbol != nonTerminal)
    malformed("expected " + nonTerminal + " but found " + node->symbol);
}

void expectSize(const Node *node, std::size_t size) {
  if (node->children.size() != size)
    malformed(node->symbol + " has " + std::to_string(node->children.size()) +
              " children, expected " + std::to_string(size));
}

// STRING is the only leaf that starts with a quote, and the lexer keeps them
bool isStringLexeme(const Node *node) {
  return node != nullptr && node->children.empty() &&
         !node->symbol.empty() && node->symbol.front() == '"';
}

// names are renamed to sysN in phase 2a, so only NUM leaves start this way
bool isNumLiteral(const Node *node) {
  if (node->symbol.empty())
    return false;
  char first = node->symbol.front();
  return std::isdigit(static_cast<unsigned char>(first)) != 0 || first == '-';
}

[[noreturn]] void unsupported(const std::string &construct,
                              const std::string &plannedFor) {
  throw CodeGenUnsupported("code generation for " + construct +
                           " is not implemented yet (planned for " +
                           plannedFor + ")");
}

std::string binaryOperator(const std::string &opName) {
  if (opName == "mod")
    return "%";
  if (opName == "div")
    return "/";
  if (opName == "add")
    return "+";
  if (opName == "sub")
    return "-";
  if (opName == "mul")
    return "*";
  malformed("unknown operator '" + opName + "' in TERM");
}

std::string labelLine(const std::string &label) {
  return label + " :" + NEWLINE;
}

std::string gotoLine(const std::string &label) {
  return "goto " + label + ";" + NEWLINE;
}

std::string comparisonOperator(const std::string &opName) {
  if (opName == "eq")
    return "==";
  if (opName == "larger")
    return ">";
  if (opName == "lesser")
    return "<";
  malformed("unknown comparison '" + opName + "' in BOOL");
}

// and is the only construct that jumps to the false label, and or hands it to
// its operands, so these are the ones that cannot be the first half of an or
bool jumpsToFalseLabel(const Node *node) {
  if (node->children.size() != BOOL_BINARY_SIZE)
    return false;
  const std::string &opName = node->children[0]->symbol;
  if (opName == "and")
    return true;
  return opName == "or" &&
         (jumpsToFalseLabel(node->children[BOOL_FIRST_OPERAND]) ||
          jumpsToFalseLabel(node->children[BOOL_SECOND_OPERAND]));
}

bool needsFloat(const Node *node) {
  if (node->children.empty()) {
    if (node->symbol == "div")
      return true;
    return isNumLiteral(node) && node->symbol.find('.') != std::string::npos;
  }
  return std::any_of(node->children.begin(), node->children.end(),
                     [](const Node *child) { return needsFloat(child); });
}

} // namespace

CodeGenUnsupported::CodeGenUnsupported(const std::string &message)
    : std::runtime_error(message) {}

std::string determineNumType(Node *root) {
  if (root == nullptr)
    malformed("cannot determine the numeric type of a null tree");
  return needsFloat(root) ? "float" : "int";
}

CodeGen::CodeGen(std::string numType)
    : numType(std::move(numType)), varCounter(0), labelCounter(0) {}

const std::string &CodeGen::getNumType() const { return numType; }

std::string CodeGen::newVar() { return "v" + std::to_string(++varCounter); }

std::string CodeGen::newLabel() { return "L" + std::to_string(++labelCounter); }

// ALGO -> EPSILON | INSTR ; ALGO
std::string CodeGen::genAlgo(Node *node) {
  std::string out;
  while (true) {
    expectNode(node, "ALGO");
    if (node->children.empty())
      return out + "/* end */";

    expectSize(node, ALGO_SEQUENCE_SIZE);
    if (node->children[ALGO_SEMI]->symbol != ";")
      malformed("ALGO expected ';' after the instruction");

    out += genInstr(node->children[ALGO_INSTR]) + ";" + NEWLINE;
    node = node->children[ALGO_REST];
  }
}

// INSTR -> print OUTP | nop | comment STRING | ASSIGN | BRANCH | LOOP | CALL
std::string CodeGen::genInstr(Node *node) {
  expectNode(node, "INSTR");
  if (node->children.empty())
    malformed("INSTR has no children");

  const Node *first = node->children[0];
  const std::string &kind = first->symbol;

  if (kind == "nop") {
    expectSize(node, KEYWORD_ONLY_SIZE);
    return "/* nop */";
  }
  if (kind == "comment") {
    expectSize(node, KEYWORD_ARG_SIZE);
    if (!isStringLexeme(node->children[1]))
      malformed("comment expected a STRING");
    return "/* " + node->children[1]->symbol + " */";
  }
  if (kind == "print") {
    expectSize(node, KEYWORD_ARG_SIZE);
    return genOutp(node->children[1]);
  }
  if (kind == "ASSIGN") {
    expectSize(node, KEYWORD_ONLY_SIZE);
    return genAssign(node->children[0]);
  }
  if (kind == "BRANCH") {
    expectSize(node, KEYWORD_ONLY_SIZE);
    return genBranch(node->children[0]);
  }
  if (kind == "LOOP")
    unsupported("LOOP", PLANNED_LOOP);
  if (kind == "CALL")
    unsupported("CALL", PLANNED_CALL);

  malformed("unknown instruction '" + kind + "'");
}

std::string CodeGen::declare(const std::string &var) const {
  return numType + " " + var + ";" + NEWLINE;
}

// OUTP -> ( TERM ) | STRING
std::string CodeGen::genOutp(Node *node) {
  expectNode(node, "OUTP");
  if (node->children.size() == OUTP_TERM_SIZE) {
    std::string var = newVar();
    return declare(var) + genTerm(node->children[OUTP_TERM], var) +
           "std::cout << " + var;
  }

  expectSize(node, KEYWORD_ONLY_SIZE);
  if (!isStringLexeme(node->children[0]))
    malformed("OUTP expected a STRING");
  return "std::cout << " + node->children[0]->symbol;
}

// ASSIGN -> NAME = TERM
std::string CodeGen::genAssign(Node *node) {
  expectNode(node, "ASSIGN");
  expectSize(node, ASSIGN_SIZE);
  const Node *name = node->children[ASSIGN_NAME];
  if (!name->children.empty() || name->symbol.empty())
    malformed("ASSIGN expected a NAME on the left");
  if (node->children[ASSIGN_EQUALS]->symbol != "=")
    malformed("ASSIGN expected '='");

  std::string var = newVar();
  return declare(var) + genTerm(node->children[ASSIGN_TERM], var) +
         name->symbol + " = " + var;
}

// TERM -> NAME | NUM | CALL | neg ( TERM ) | op ( TERM TERM )
std::string CodeGen::genTerm(Node *node, const std::string &resultVar) {
  expectNode(node, "TERM");

  if (node->children.size() == TERM_LEAF_SIZE) {
    const Node *leaf = node->children[0];
    if (leaf->symbol == "CALL")
      unsupported("a CALL inside a TERM", PLANNED_CALL);
    if (!leaf->children.empty() || leaf->symbol.empty())
      malformed("TERM expected a NAME or NUM leaf");
    return resultVar + " = " + leaf->symbol + ";" + NEWLINE;
  }
  if (node->children.size() == TERM_UNARY_SIZE)
    return genNegation(node, resultVar);
  if (node->children.size() == TERM_BINARY_SIZE)
    return genBinary(node, resultVar);

  malformed("TERM has " + std::to_string(node->children.size()) +
            " children");
}

std::string CodeGen::genNegation(Node *node, const std::string &resultVar) {
  if (node->children[0]->symbol != "neg")
    malformed("TERM with four children must start with neg");

  std::string operand = newVar();
  return declare(operand) +
         genTerm(node->children[TERM_FIRST_OPERAND], operand) + resultVar +
         " = -" + operand + ";" + NEWLINE;
}

std::string CodeGen::genBinary(Node *node, const std::string &resultVar) {
  const std::string opSymbol = binaryOperator(node->children[0]->symbol);

  std::string left = newVar();
  std::string right = newVar();
  // separate statements: operand order of + is unspecified, and the
  // temporary numbering depends on the left operand being generated first
  std::string out = declare(left) + declare(right);
  out += genTerm(node->children[TERM_FIRST_OPERAND], left);
  out += genTerm(node->children[TERM_SECOND_OPERAND], right);
  return out + resultVar + " = " + left + " " + opSymbol + " " + right + ";" +
         NEWLINE;
}

// BOOL -> not ( BOOL ) | and|or ( BOOL BOOL ) | eq|larger|lesser ( TERM TERM )
//
// The spec swaps the labels for not and rewrites and as not/not. With one
// conditional jump and fall-through that puts the fall-through on the wrong
// label (a while written as until-not would run its body either way, and
// and would behave like or), so those two use fresh local labels instead.
// or additionally needs its own label when its first operand contains an and,
// because that operand's goto-false would skip the second operand.
std::string CodeGen::genBool(Node *node, const std::string &trueLabel,
                             const std::string &falseLabel) {
  expectNode(node, "BOOL");
  if (node->children.size() == BOOL_NOT_SIZE) {
    if (node->children[0]->symbol != "not")
      malformed("BOOL with four children must start with not");
    return genNot(node->children[BOOL_FIRST_OPERAND], trueLabel);
  }
  expectSize(node, BOOL_BINARY_SIZE);

  const std::string &opName = node->children[0]->symbol;
  if (opName == "and")
    return genAnd(node, trueLabel, falseLabel);
  if (opName == "or")
    return genOr(node, trueLabel, falseLabel);
  return genComparison(node, trueLabel);
}

std::string CodeGen::genComparison(Node *node, const std::string &trueLabel) {
  const std::string opSymbol = comparisonOperator(node->children[0]->symbol);

  std::string left = newVar();
  std::string right = newVar();
  std::string out = declare(left) + declare(right);
  out += genTerm(node->children[BOOL_FIRST_OPERAND], left);
  out += genTerm(node->children[BOOL_SECOND_OPERAND], right);
  return out + "if (" + left + " " + opSymbol + " " + right + ") goto " +
         trueLabel + ";" + NEWLINE;
}

std::string CodeGen::genNot(Node *inner, const std::string &trueLabel) {
  std::string innerTrue = newLabel();
  std::string innerFalse = newLabel();

  std::string out = genBool(inner, innerTrue, innerFalse);
  return out + labelLine(innerFalse) + gotoLine(trueLabel) +
         labelLine(innerTrue);
}

std::string CodeGen::genAnd(Node *node, const std::string &trueLabel,
                            const std::string &falseLabel) {
  std::string secondStart = newLabel();
  std::string firstFalse = newLabel();

  std::string out = genBool(node->children[BOOL_FIRST_OPERAND], secondStart,
                            firstFalse);
  out += labelLine(firstFalse) + gotoLine(falseLabel) + labelLine(secondStart);
  out += genBool(node->children[BOOL_SECOND_OPERAND], trueLabel, falseLabel);
  return out;
}

std::string CodeGen::genOr(Node *node, const std::string &trueLabel,
                           const std::string &falseLabel) {
  Node *first = node->children[BOOL_FIRST_OPERAND];
  Node *second = node->children[BOOL_SECOND_OPERAND];

  if (!jumpsToFalseLabel(first)) {
    std::string out = genBool(first, trueLabel, falseLabel);
    out += genBool(second, trueLabel, falseLabel);
    return out;
  }

  std::string secondStart = newLabel();
  std::string out = genBool(first, trueLabel, secondStart);
  out += labelLine(secondStart);
  out += genBool(second, trueLabel, falseLabel);
  return out;
}

// BRANCH -> if BOOL then { ALGO } else { ALGO }
std::string CodeGen::genBranch(Node *node) {
  expectNode(node, "BRANCH");
  expectSize(node, BRANCH_SIZE);
  for (const auto &fixed : BRANCH_FIXED) {
    if (node->children[fixed.first]->symbol != fixed.second)
      malformed(std::string("BRANCH expected '") + fixed.second + "'");
  }

  std::string thenLabel = newLabel();
  std::string elseLabel = newLabel();
  std::string endLabel = newLabel();

  std::string decision =
      genBool(node->children[BRANCH_BOOL], thenLabel, elseLabel);
  std::string trueCode = genAlgo(node->children[BRANCH_THEN]);
  std::string falseCode = genAlgo(node->children[BRANCH_ELSE]);

  return std::string("/* if */") + NEWLINE + decision + elseLabel +
         " : /* else */" + NEWLINE + falseCode + ";" + NEWLINE +
         gotoLine(endLabel) + thenLabel + " : /* then */" + NEWLINE +
         trueCode + ";" + NEWLINE + endLabel + " : /* continue */";
}
