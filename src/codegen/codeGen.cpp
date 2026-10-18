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

constexpr int STEP_TERMS_AND_ASSIGN = 2;
constexpr int STEP_BRANCH = 3;
constexpr int STEP_LOOP = 4;
constexpr int STEP_CALL = 5;

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

[[noreturn]] void unsupported(const std::string &construct, int step) {
  throw CodeGenUnsupported("code generation for " + construct +
                           " is not implemented yet (planned for step " +
                           std::to_string(step) + ")");
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

// OUTP -> ( TERM ) | STRING
std::string genOutp(const Node *node) {
  expectNode(node, "OUTP");
  if (node->children.size() == OUTP_TERM_SIZE)
    unsupported("print ( TERM )", STEP_TERMS_AND_ASSIGN);

  expectSize(node, KEYWORD_ONLY_SIZE);
  if (!isStringLexeme(node->children[0]))
    malformed("OUTP expected a STRING");
  return "std::cout << " + node->children[0]->symbol;
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
  if (kind == "ASSIGN")
    unsupported("ASSIGN", STEP_TERMS_AND_ASSIGN);
  if (kind == "BRANCH")
    unsupported("BRANCH", STEP_BRANCH);
  if (kind == "LOOP")
    unsupported("LOOP", STEP_LOOP);
  if (kind == "CALL")
    unsupported("CALL", STEP_CALL);

  malformed("unknown instruction '" + kind + "'");
}
