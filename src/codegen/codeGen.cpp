#include "codeGen.h"

#include <algorithm>
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

constexpr std::size_t TERM_LEAF_SIZE = 1;
constexpr std::size_t TERM_UNARY_SIZE = 4;
constexpr std::size_t TERM_BINARY_SIZE = 5;
constexpr std::size_t TERM_FIRST_OPERAND = 2;
constexpr std::size_t TERM_SECOND_OPERAND = 3;

const char *const PLANNED_BRANCH = "step 3";
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
  if (kind == "BRANCH")
    unsupported("BRANCH", PLANNED_BRANCH);
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
