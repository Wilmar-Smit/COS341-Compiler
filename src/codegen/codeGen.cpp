#include "codeGen.h"

#include <array>
#include <span>
#include <utility>
#include <vector>

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

constexpr std::size_t PROG_SIZE = 2;
constexpr std::size_t PROG_P = 0;
constexpr std::size_t PROG_END = 1;

constexpr std::size_t P_SIZE = 5;
constexpr std::size_t P_VDECL = 0;
constexpr std::size_t P_FIRST_COLON = 1;
constexpr std::size_t P_FDECL = 2;
constexpr std::size_t P_SECOND_COLON = 3;
constexpr std::size_t P_ALGO = 4;

constexpr std::size_t DECL_SIZE = 2;

constexpr std::size_t FTYPE_VOID_SIZE = 9;
constexpr std::size_t FTYPE_NUM_SIZE = 12;
constexpr std::size_t FTYPE_NAME = 1;
constexpr std::size_t FTYPE_PARAMS = 3;
constexpr std::size_t FTYPE_BODY = 6;
constexpr std::size_t FTYPE_RETURN_TERM = 9;
using FixedSymbol = std::pair<std::size_t, const char *>;
const std::array<FixedSymbol, 5> FTYPE_VOID_FIXED = {
    {{2, "("}, {4, ")"}, {5, "{"}, {7, "return"}, {8, "}"}}};
const std::array<FixedSymbol, 7> FTYPE_NUM_FIXED = {
    {{2, "("}, {4, ")"}, {5, "{"}, {7, "return"}, {8, "("}, {10, ")"},
     {11, "}"}}};

constexpr std::size_t LOOP_SIZE = 6;
constexpr std::size_t LOOP_TEST_BOOL = 1;
constexpr std::size_t LOOP_TEST_ALGO = 4;
constexpr std::size_t LOOP_REPEAT_ALGO = 2;
constexpr std::size_t LOOP_REPEAT_COND = 4;
constexpr std::size_t LOOP_REPEAT_BOOL = 5;
const std::array<FixedSymbol, 3> LOOP_TEST_FIXED = {
    {{2, "do"}, {3, "{"}, {5, "}"}}};
const std::array<FixedSymbol, 3> LOOP_REPEAT_FIXED = {
    {{0, "do"}, {1, "{"}, {3, "}"}}};

constexpr std::size_t TERM_LEAF_SIZE = 1;
constexpr std::size_t TERM_UNARY_SIZE = 4;
constexpr std::size_t TERM_BINARY_SIZE = 5;
constexpr std::size_t TERM_FIRST_OPERAND = 2;
constexpr std::size_t TERM_SECOND_OPERAND = 3;

constexpr std::size_t CALL_SIZE = 4;
constexpr std::size_t CALL_NAME = 0;
constexpr std::size_t CALL_OPEN = 1;
constexpr std::size_t CALL_INPUT = 2;
constexpr std::size_t CALL_CLOSE = 3;

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

void expectFixed(const Node *node, std::span<const FixedSymbol> fixed) {
  for (const auto &entry : fixed) {
    if (node->children[entry.first]->symbol != entry.second)
      malformed(node->symbol + " expected '" + entry.second + "'");
  }
}

// COND -> while | until
const std::string &condKind(const Node *cond) {
  expectNode(cond, "COND");
  expectSize(cond, KEYWORD_ONLY_SIZE);
  const std::string &kind = cond->children[0]->symbol;
  if (kind != "while" && kind != "until")
    malformed("COND expected while or until but found " + kind);
  return kind;
}

// #if 0 keeps the mine inert even though the generated bodies contain
// /* */ comments, which cannot nest inside one big C comment
std::string wrapMine(const std::string &mine) {
  return std::string("#if 0") + NEWLINE + mine + "#endif" + NEWLINE;
}

// the two parts of a function body that every F_TYPE and main share
void expectBody(const Node *body) {
  expectNode(body, "P");
  expectSize(body, P_SIZE);
  if (body->children[P_FIRST_COLON]->symbol != ":" ||
      body->children[P_SECOND_COLON]->symbol != ":")
    malformed("P expected ':' between its parts");
}

// returns true for a num function, false for a void procedure
bool expectFunctionShape(const Node *node) {
  expectNode(node, "F_TYPE");
  if (node->children.empty())
    malformed("F_TYPE has no children");

  const std::string &kind = node->children[0]->symbol;
  if (kind != "void" && kind != "num")
    malformed("F_TYPE expected void or num but found " + kind);

  bool isNum = kind == "num";
  expectSize(node, isNum ? FTYPE_NUM_SIZE : FTYPE_VOID_SIZE);
  if (isNum)
    expectFixed(node, FTYPE_NUM_FIXED);
  else
    expectFixed(node, FTYPE_VOID_FIXED);

  const Node *name = node->children[FTYPE_NAME];
  if (!name->children.empty() || name->symbol.empty())
    malformed("F_TYPE expected a NAME");
  return isNum;
}

// V_DECL -> EPSILON | NAME V_DECL, as a list of names
std::vector<std::string> declaredNames(const Node *node) {
  std::vector<std::string> names;
  while (true) {
    expectNode(node, "V_DECL");
    if (node->children.empty())
      return names;

    expectSize(node, DECL_SIZE);
    const Node *name = node->children[0];
    if (!name->children.empty() || name->symbol.empty())
      malformed("V_DECL expected a NAME");

    names.push_back(name->symbol);
    node = node->children[1];
  }
}

// INPUT -> EPSILON | TERM INPUT, as a list of argument terms
std::vector<Node *> inputTerms(Node *input) {
  std::vector<Node *> terms;
  while (true) {
    expectNode(input, "INPUT");
    if (input->children.empty())
      return terms;

    expectSize(input, DECL_SIZE);
    terms.push_back(input->children[0]);
    input = input->children[1];
  }
}

// a num function used as an instruction ends in the line of its return term
std::string withoutLastTerminator(std::string code) {
  const std::string terminator = std::string(";") + NEWLINE;
  if (code.size() < terminator.size() ||
      code.compare(code.size() - terminator.size(), terminator.size(),
                   terminator) != 0)
    malformed("an inlined num function does not end in a complete line");
  code.erase(code.size() - terminator.size());
  return code;
}

bool isNumFunction(const Node *fType) {
  return fType->children[0]->symbol == "num";
}

// Inlining has no recursion, so a function that is already being generated
// can only be reached through a call cycle. Leaving the scope, normally or
// by exception, removes the name again.
class ActiveFunction {
public:
  ActiveFunction(std::set<std::string> &active, const std::string &name)
      : active(active), name(name) {
    if (active.contains(name))
      throw CodeGenUnsupported("function " + name +
                               " calls itself, directly or indirectly, and "
                               "recursion cannot be inlined");
    active.insert(name);
  }
  ~ActiveFunction() { active.erase(name); }
  ActiveFunction(const ActiveFunction &) = delete;
  ActiveFunction &operator=(const ActiveFunction &) = delete;
  ActiveFunction(ActiveFunction &&) = delete;
  ActiveFunction &operator=(ActiveFunction &&) = delete;

private:
  std::set<std::string> &active;
  std::string name;
};

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

} // namespace

CodeGenUnsupported::CodeGenUnsupported(const std::string &message)
    : std::runtime_error(message) {}

// Phase 2b decides this for the whole program: mod forces INTEGER, anything
// else is FLOAT
std::string numTypeFromMode(NumberMode mode) {
  return mode == NumberMode::INTEGER ? "int" : "float";
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
  if (kind == "LOOP") {
    expectSize(node, KEYWORD_ONLY_SIZE);
    return genLoop(node->children[0]);
  }
  if (kind == "CALL") {
    expectSize(node, KEYWORD_ONLY_SIZE);
    return genCallInstr(node->children[0]);
  }

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
      return genCallTerm(node->children[0], resultVar);
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

// LOOP -> COND BOOL do { ALGO } | do { ALGO } COND BOOL
std::string CodeGen::genLoop(Node *node) {
  expectNode(node, "LOOP");
  expectSize(node, LOOP_SIZE);

  if (node->children[0]->symbol == "COND") {
    expectFixed(node, LOOP_TEST_FIXED);
    bool untilForm = condKind(node->children[0]) == "until";
    return genTestLoop(node->children[LOOP_TEST_BOOL],
                       node->children[LOOP_TEST_ALGO], untilForm);
  }

  expectFixed(node, LOOP_REPEAT_FIXED);
  bool whileForm = condKind(node->children[LOOP_REPEAT_COND]) == "while";
  return genRepeatLoop(node->children[LOOP_REPEAT_BOOL],
                       node->children[LOOP_REPEAT_ALGO], whileForm);
}

// The condition is tested before the body. while is until with the condition
// negated, so both exit through the same label.
std::string CodeGen::genTestLoop(Node *boolNode, Node *algo, bool untilForm) {
  std::string startLabel = newLabel();
  std::string exitLabel = newLabel();
  std::string bodyLabel = newLabel();

  std::string decision = untilForm ? genBool(boolNode, exitLabel, bodyLabel)
                                   : genNot(boolNode, exitLabel);
  std::string bodyCode = genAlgo(algo);

  return startLabel + " : /* until */" + NEWLINE + decision + bodyLabel +
         " : /* do */" + NEWLINE + bodyCode + ";" + NEWLINE +
         gotoLine(startLabel) + labelLine(exitLabel) + "/* exit */";
}

// The body runs once before the condition is first tested. do-until is
// do-while with the condition negated.
std::string CodeGen::genRepeatLoop(Node *boolNode, Node *algo,
                                   bool whileForm) {
  std::string bodyLabel = newLabel();
  std::string exitLabel = newLabel();

  std::string decision = whileForm ? genBool(boolNode, bodyLabel, exitLabel)
                                   : genNot(boolNode, bodyLabel);
  std::string bodyCode = genAlgo(algo);

  return bodyLabel + " : /* do */" + NEWLINE + bodyCode + ";" + NEWLINE +
         "/* while */" + NEWLINE + decision + exitLabel + " : /* exit */";
}

// SPL_PROG -> P $
std::string CodeGen::genProgram(Node *root) {
  expectNode(root, "SPL_PROG");
  expectSize(root, PROG_SIZE);
  if (root->children[PROG_END]->symbol != "$")
    malformed("SPL_PROG expected '$' after P");

  functions.clear();
  hoistedDecls.clear();
  activeFunctions.clear();
  registerFunctions(root->children[PROG_P]);

  return genP(root->children[PROG_P]) + NEWLINE +
         "return 0; } /* end of executable C */";
}

void CodeGen::registerFunctions(Node *node) {
  if (node == nullptr)
    return;
  if (node->symbol == "F_TYPE") {
    expectFunctionShape(node);
    const std::string &name = node->children[FTYPE_NAME]->symbol;
    if (!functions.emplace(name, FunctionEntry{node, ""}).second)
      malformed("function '" + name + "' is declared twice");
  }
  for (Node *child : node->children)
    registerFunctions(child);
}

CodeGen::FunctionEntry &CodeGen::lookupFunction(const std::string &name) {
  auto found = functions.find(name);
  if (found == functions.end())
    malformed("unknown function '" + name + "'");
  return found->second;
}

// P -> V_DECL : F_DECL : ALGO
std::string CodeGen::genP(Node *node) {
  expectBody(node);

  std::string mine = genFDecl(node->children[P_FDECL]);
  std::string out =
      "/* begin of the non-executable code-mine for later inlining */";
  out += NEWLINE;
  out += mine.empty() ? " " : wrapMine(mine);
  out += NEWLINE;
  out += "/* end of the non-executable code-mine */";
  out += NEWLINE;
  out += "/* begin of executable C code */";
  out += NEWLINE;
  out += "#include <iostream>";
  out += NEWLINE;
  out += "int main()";
  out += NEWLINE;
  out += "{";
  out += NEWLINE;
  out += genVDecl(node->children[P_VDECL]);
  out += NEWLINE;
  out += hoistedDecls;
  out += genAlgo(node->children[P_ALGO]);
  return out;
}

// F_DECL -> EPSILON | F_TYPE F_DECL
std::string CodeGen::genFDecl(Node *node) {
  std::string out;
  while (true) {
    expectNode(node, "F_DECL");
    if (node->children.empty())
      return out;

    expectSize(node, DECL_SIZE);
    out += genFunction(node->children[0]);
    node = node->children[1];
  }
}

// A mine is rendered nested functions first, then the function itself. The
// parameters, locals and result variable are also declared once in main (see
// hoistedDecls), because the mine is not compiled.
std::string CodeGen::genFunction(Node *node) {
  bool isNum = expectFunctionShape(node);
  const std::string &name = node->children[FTYPE_NAME]->symbol;
  Node *body = node->children[FTYPE_BODY];
  expectBody(body);

  std::string out = genFDecl(body->children[P_FDECL]);

  FunctionEntry &entry = lookupFunction(name);
  if (isNum)
    returnVarOf(entry);

  for (const std::string &param : declaredNames(node->children[FTYPE_PARAMS]))
    hoistedDecls += declare(param);
  for (const std::string &local : declaredNames(body->children[P_VDECL]))
    hoistedDecls += declare(local);
  if (isNum)
    hoistedDecls += declare(entry.returnVar);

  const std::string label =
      std::string(isNum ? "Function " : "void-procedure ") + name;
  out += "begin of code-mine for " + label + NEWLINE;
  out += std::string("Parameters for inlining:") + NEWLINE;
  out += genVDecl(node->children[FTYPE_PARAMS]);
  out += NEWLINE;
  out += std::string("Body of the procedure:") + NEWLINE;
  out += genVDecl(body->children[P_VDECL]);
  out += NEWLINE;
  out += genBody(entry);
  out += NEWLINE;
  out += "end of code-mine for " + label + NEWLINE;
  if (isNum)
    out += "The result is found in the return value " + entry.returnVar +
           NEWLINE;
  return out;
}

// the result variable is allocated when it is first needed, by the mine of the
// function or by a call to it, whichever comes first
const std::string &CodeGen::returnVarOf(FunctionEntry &entry) {
  if (entry.returnVar.empty())
    entry.returnVar = entry.node->children[FTYPE_NAME]->symbol + newVar();
  return entry.returnVar;
}

// the code of a function body, shared by its mine and by every inlining
std::string CodeGen::genBody(FunctionEntry &entry) {
  const Node *fType = entry.node;
  ActiveFunction active(activeFunctions, fType->children[FTYPE_NAME]->symbol);

  std::string out = genAlgo(fType->children[FTYPE_BODY]->children[P_ALGO]);
  if (isNumFunction(fType)) {
    out += std::string(";") + NEWLINE;
    out += genTerm(fType->children[FTYPE_RETURN_TERM], returnVarOf(entry));
  }
  return out;
}

// CALL -> NAME ( INPUT )
CodeGen::FunctionEntry &CodeGen::calleeOf(Node *call) {
  expectNode(call, "CALL");
  expectSize(call, CALL_SIZE);
  if (call->children[CALL_OPEN]->symbol != "(" ||
      call->children[CALL_CLOSE]->symbol != ")")
    malformed("CALL expected '(' and ')'");

  const Node *name = call->children[CALL_NAME];
  if (!name->children.empty() || name->symbol.empty())
    malformed("CALL expected a NAME");
  return lookupFunction(name->symbol);
}

// Every argument is captured in a fresh variable first, so an argument that
// itself calls a function cannot disturb the parameters assigned afterwards.
// Parameters, locals and the result variable are declared once in main.
std::string CodeGen::genInline(Node *call, FunctionEntry &callee) {
  std::vector<Node *> arguments = inputTerms(call->children[CALL_INPUT]);
  std::vector<std::string> parameters =
      declaredNames(callee.node->children[FTYPE_PARAMS]);
  if (arguments.size() != parameters.size())
    malformed("call of " + callee.node->children[FTYPE_NAME]->symbol +
              " passes " + std::to_string(arguments.size()) +
              " arguments, expected " + std::to_string(parameters.size()));

  std::vector<std::string> captured;
  std::string out;
  for (std::size_t i = 0; i < arguments.size(); i++) {
    captured.push_back(newVar());
    out += declare(captured.back());
  }
  for (std::size_t i = 0; i < arguments.size(); i++)
    out += genTerm(arguments[i], captured[i]);
  for (std::size_t i = 0; i < arguments.size(); i++)
    out += parameters[i] + " = " + captured[i] + ";" + NEWLINE;
  return out + genBody(callee);
}

std::string CodeGen::genCallInstr(Node *call) {
  FunctionEntry &callee = calleeOf(call);
  std::string code = genInline(call, callee);
  return isNumFunction(callee.node) ? withoutLastTerminator(code) : code;
}

std::string CodeGen::genCallTerm(Node *call, const std::string &resultVar) {
  FunctionEntry &callee = calleeOf(call);
  if (!isNumFunction(callee.node))
    malformed("void procedure " + callee.node->children[FTYPE_NAME]->symbol +
              " used as a TERM");
  return genInline(call, callee) + resultVar + " = " + returnVarOf(callee) +
         ";" + NEWLINE;
}

// V_DECL -> EPSILON | NAME V_DECL
std::string CodeGen::genVDecl(Node *node) const {
  std::string out;
  while (true) {
    expectNode(node, "V_DECL");
    if (node->children.empty())
      return out + " ";

    expectSize(node, DECL_SIZE);
    const Node *name = node->children[0];
    if (!name->children.empty() || name->symbol.empty())
      malformed("V_DECL expected a NAME");

    out += declare(name->symbol);
    node = node->children[1];
  }
}
