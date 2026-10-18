#include "test_helpers.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "tokens/genericTokenHandler/generic.handler.h"
#include "fileReader/parseTableReader.h"
#include "state.enum.h"
#include "parser.actions.h"
#include "parser.visitor.h"
#include "TreeBuilder.h"

// Single translation unit that includes generic.handler.h - see the ODR
// note in test_helpers.h.

TokenHandler *newHandlerChain() { return createChain(); }

std::vector<Token *> tokenizeAll(const std::string &source) {
  TokenHandler *chain = createChain();
  std::vector<Token *> tokens;
  std::string stream = source;
  while (stream != "") {
    auto res = chain->handle(stream);
    tokens.push_back(res.token);
    stream = res.remainingStream;
  }
  delete chain;
  return tokens;
}

PipelineResult runPipeline(const std::string &source) {
  PipelineResult result;

  std::vector<Token *> tokenList = tokenizeAll(source);
  tokenList.push_back(new Token("$", TokenType::DOLLAR_EOF));

  ActionTableReader actionReader;
  auto table = actionReader.read("SLR_ACTION_Table.csv");
  GotoTableReader gotoReader;
  auto gotoTable = gotoReader.read("SLR_GOTO_Table.csv");

  std::stack<ParserStates> stateStack;
  stateStack.push(ParserStates::S0);
  TreeBuilder xml;
  ParseVisitor visitor(table, gotoTable, stateStack, xml);

  std::error_code ec;
  std::filesystem::remove("tree.xml", ec);

  std::ostringstream capture;
  std::streambuf *oldBuf = std::cout.rdbuf(capture.rdbuf());
  result.accepted = visitor.parseTokens(tokenList);
  std::cout.rdbuf(oldBuf);
  result.diagnostics = capture.str();

  result.xmlWritten = std::filesystem::exists("tree.xml");

  for (auto &row : table)
    for (auto *action : row) delete action;
  for (auto &row : gotoTable)
    for (auto *action : row) delete action;
  for (auto *token : tokenList) delete token;

  return result;
}

CompiledProgram compileSource(const std::string &source) {
  CompiledProgram program;

  std::vector<Token *> rawTokens = tokenizeAll(source);
  rawTokens.push_back(new Token("$", TokenType::DOLLAR_EOF));
  for (Token *token : rawTokens) program.tokens.emplace_back(token);

  program.parser = std::make_unique<Parser>();
  std::ostringstream capture;
  std::streambuf *oldBuf = std::cout.rdbuf(capture.rdbuf());
  bool accepted = program.parser->ParseTokens(rawTokens);
  std::cout.rdbuf(oldBuf);
  program.tree.reset(program.parser->getTree());
  if (!accepted)
    throw std::runtime_error("source was rejected by the parser: " +
                             capture.str());

  program.scopeTree = std::make_unique<ScopeTree>(*program.tree);
  program.root = program.tree->getRoot();
  program.typeChecker = std::make_unique<TypeChecker>(
      program.root, program.scopeTree->getSymbolTable());
  return program;
}

namespace {

std::string searchPath(const std::string &name) {
  const char *pathEnv = std::getenv("PATH");
  if (pathEnv == nullptr) return "";

  std::stringstream paths(pathEnv);
  std::string dir;
  while (std::getline(paths, dir, ':')) {
    std::filesystem::path candidate = std::filesystem::path(dir) / name;
    std::error_code ec;
    auto perms = std::filesystem::status(candidate, ec).permissions();
    if (!ec && std::filesystem::is_regular_file(candidate, ec) &&
        (perms & std::filesystem::perms::owner_exec) !=
            std::filesystem::perms::none)
      return candidate.string();
  }
  return "";
}

// Removes every temporary the compile-and-run produces, even on a throw.
struct TempFiles {
  std::vector<std::filesystem::path> paths;
  ~TempFiles() {
    std::error_code ec;
    for (const auto &path : paths) std::filesystem::remove(path, ec);
  }
};

}  // namespace

std::string findCxxCompiler() {
  const char *fromEnv = std::getenv("CXX");
  if (fromEnv != nullptr && *fromEnv != '\0') return fromEnv;

  for (const char *name : {"g++", "clang++", "c++"}) {
    std::string found = searchPath(name);
    if (!found.empty()) return "\"" + found + "\"";
  }
  return "";
}

std::string wrapInMain(const CompiledProgram &program,
                       const std::string &numType,
                       const std::string &fragment) {
  std::string declarations;
  const Node *decl = program.root->children[0]->children[0];
  while (!decl->children.empty()) {
    declarations += numType + " " + decl->children[0]->symbol + " = 0;\n";
    decl = decl->children[1];
  }
  return "#include <iostream>\nint main() {\n" + declarations + fragment +
         "\nreturn 0;\n}\n";
}

std::string compileAndRun(const std::string &source) {
  static int counter = 0;
  std::string compiler = findCxxCompiler();
  if (compiler.empty()) throw std::runtime_error("no C++ compiler found");

  std::string stem =
      "codegen_run_" +
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
      "_" + std::to_string(counter++);
  std::filesystem::path src = std::filesystem::absolute(stem + ".cpp");
  std::filesystem::path bin = std::filesystem::absolute(stem + ".bin");
  std::filesystem::path log = std::filesystem::absolute(stem + ".log");

  TempFiles temps;
  temps.paths = {src, bin, log};

  std::ofstream(src) << source;

  std::string compile = compiler + " -std=c++17 -w -o \"" + bin.string() +
                        "\" \"" + src.string() + "\" > \"" + log.string() +
                        "\" 2>&1";
  if (std::system(compile.c_str()) != 0) {
    std::ifstream logFile(log);
    std::stringstream diagnostics;
    diagnostics << logFile.rdbuf();
    throw std::runtime_error("generated code failed to compile:\n" +
                             diagnostics.str() + "\n--- source ---\n" + source);
  }

  FILE *pipe = popen(("\"" + bin.string() + "\"").c_str(), "r");
  if (pipe == nullptr) throw std::runtime_error("could not run " + bin.string());

  std::string output;
  std::array<char, 256> buffer{};
  while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) !=
         nullptr)
    output += buffer.data();
  if (pclose(pipe) != 0)
    throw std::runtime_error("generated program exited abnormally");
  return output;
}
