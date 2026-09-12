#ifndef CSVREADER_H
#define CSVREADER_H

#include "../Parser/ParserActions/parser.actions.h"
#include "../tokens/tokenTable.h"
#include <fstream>
#include <ios>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

inline optional<NonTerminal> nonTerminalFromName(const string &name) {
  static const map<string, NonTerminal> table = {
      {"SPL_PROG", NonTerminal::SPL_PROG}, {"P", NonTerminal::P},
      {"V_DECL", NonTerminal::V_DECL},     {"F_DECL", NonTerminal::F_DECL},
      {"F_TYPE", NonTerminal::F_TYPE},     {"ALGO", NonTerminal::ALGO},
      {"OUTP", NonTerminal::OUTP},         {"INSTR", NonTerminal::INSTR},
      {"CALL", NonTerminal::CALL},         {"INPUT", NonTerminal::INPUT},
      {"ASSIGN", NonTerminal::ASSIGN},     {"TERM", NonTerminal::TERM},
      {"BRANCH", NonTerminal::BRANCH},     {"BOOL", NonTerminal::BOOL},
      {"LOOP", NonTerminal::LOOP},         {"COND", NonTerminal::COND},
  };
  auto it = table.find(name);
  if (it == table.end()) return nullopt;
  return it->second;
}

// Grammar productions reference lexical categories (NAME/NUM/STRING), not a
// concrete lexeme, so these can't be matched via the lexer's regexes.
inline optional<TokenType> placeholderTerminal(const string &symbol) {
  static const map<string, TokenType> table = {
      {"NAME", TokenType::NAME},
      {"NUM", TokenType::NUM},
      {"STRING", TokenType::STRING},
  };
  auto it = table.find(symbol);
  if (it == table.end()) return nullopt;
  return it->second;
}

// Keywords and punctuation from ProductionRules.txt and the CSV headers are matched against the same
// regexes the lexer uses, so the grammar/tables and the lexer can never
// disagree about what a symbol like "mod" or "(" means.
inline optional<TokenType> terminalFromSymbol(const string &symbol) {
  if (auto placeholder = placeholderTerminal(symbol)) return placeholder;

  for (std::size_t i = 0; i < TOKEN_TYPE_COUNT; ++i) {
    auto type = static_cast<TokenType>(i);
    if (type == TokenType::NAME || type == TokenType::NUM ||
        type == TokenType::STRING || type == TokenType::SPACE) {
      continue;
    }
    if (regex_match(symbol, regex(patternFor(type)))) return type;
  }
  return nullopt;
}

// Template method: read() owns the shared file/line/word parsing loop and
// defers building each cell to create(), which subclasses implement.
class ParseTableReader {
public:
  explicit ParseTableReader(const string &ruleFilePath = "ProductionRules.txt") {
    loadProductionRules(ruleFilePath);
  }

  virtual ~ParseTableReader() {}

  // The CSV's own column order doesnt necessarily match the enum's declaration
  // order, so we read the header row once and use
  // it to place each cell at its TokenType/NonTerminal ordinal. that's
  // the index ParseVisitor uses directly via static_cast, so it must line
  // up exactly
  vector<vector<ParserAction *>> read(const string &filePath) {
    ifstream fin(filePath);
    if (!fin.is_open()) {
      throw std::runtime_error("Failed to open parse table file: " + filePath);
    }
    string line;

    getline(fin, line);
    stripCR(line);

    // this is where we change the column order coz the visitor requires the same order as in the
    // tokentype enum class
    vector<int> columnOrder = parseHeaderColumns(line);

    vector<vector<ParserAction *>> table;

    while (getline(fin, line)) {
      stripCR(line);
      if (line.empty()) continue;

      vector<ParserAction *> row(columnCount(), nullptr);

      stringstream s(line);
      string word;
      bool isStateColumn = true;
      size_t fieldPos = 0;

      while (getline(s, word, ',')) {
        if (isStateColumn) { // first column is the state number, not a cell so ignore it
          isStateColumn = false;
          continue;
        }
        row[columnOrder.at(fieldPos)] = create(word);
        fieldPos++;
      }
      // getline(s, word, ',') can't tell an empty trailing field from
      // end-of-stream, so a line ending in "," silently drops its last
      // (empty) cell above so we add it back.
      if (line.back() == ',') {
        row[columnOrder.at(fieldPos)] = create("");
        fieldPos++;
      }

      // any enum value with no column in this table (e.g. SPACE) defaults to error
      // I am not entirely sure about this, but I think Wilmar said that we will just
      // do nothing when we encounter spaces since the parse table does not account for them either
      for (auto &action : row) {
        if (!action) action = create("");
      }

      table.push_back(row);
    }

    return table;
  }

protected:
  map<int, ProductionRule> ruleTable;

  virtual ParserAction *create(const string &word) = 0;
  virtual std::size_t columnCount() = 0;
  virtual int columnIndexFor(const string &label) = 0;

private:
  static void stripCR(string &line) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
  }

  vector<int> parseHeaderColumns(const string &headerLine) {
    vector<int> order;
    stringstream hs(headerLine);
    string label;
    bool isStateColumn = true;

    while (getline(hs, label, ',')) {
      if (isStateColumn) {
        isStateColumn = false;
        continue;
      }
      order.push_back(columnIndexFor(label));
    }
    return order;
  }

  void loadProductionRules(const string &ruleFilePath) {
    ifstream fin(ruleFilePath);
    if (!fin.is_open()) {
      throw std::runtime_error("Failed to open production rules file: " + ruleFilePath);
    }
    string line;

    while (getline(fin, line)) {
      if (line.empty()) continue;

      stringstream s(line);
      int number;
      string lhsName, arrow;
      s >> number >> lhsName >> arrow;

      auto lhs = nonTerminalFromName(lhsName);
      if (!lhs) continue; // e.g. the augmented start rule "SPL_PROG'"

      vector<GrammarSymbol> rhs;
      string symbol;
      while (s >> symbol) {
        if (symbol == "EPSILON") continue;

        if (auto nt = nonTerminalFromName(symbol)) {
          rhs.push_back(*nt);
        } else if (auto tt = terminalFromSymbol(symbol)) {
          rhs.push_back(Token(symbol, *tt));
        }
      }

      ruleTable[number] = ProductionRule{*lhs, rhs, (int)rhs.size()};
    }
  }
};

class ActionTableReader : public ParseTableReader {
public:
  using ParseTableReader::ParseTableReader;

protected:
  std::size_t columnCount() override { return TOKEN_TYPE_COUNT; }

  int columnIndexFor(const string &label) override {
    auto type = terminalFromSymbol(label);
    if (!type) {
      throw std::runtime_error("Unrecognised action-table column: " + label);
    }
    return static_cast<int>(*type);
  }

  ParserAction *create(const string &word) override {
    if (word.empty()) return new ErrorAction();
    if (word == "acc") return new AcceptAction();

    // stoi just takes the number out of the string and returns an int "s12" becomes 12
    int value = stoi(word.substr(1));
    switch (word[0]) {
      case 's':
        return new ShiftAction(static_cast<ParserStates>(value));
      case 'r':
        return new ReduceAction(ruleTable.at(value));
      default:
        return new ErrorAction();
    }
  }
};

class GotoTableReader : public ParseTableReader {
public:
  using ParseTableReader::ParseTableReader;

protected:
  std::size_t columnCount() override { return NONTERMINAL_COUNT; }

  int columnIndexFor(const string &label) override {
    auto nt = nonTerminalFromName(label);
    if (!nt) {
      throw std::runtime_error("Unrecognised goto-table column: " + label);
    }
    return static_cast<int>(*nt);
  }

  ParserAction *create(const string &word) override {
    if (word.empty()) return new ErrorAction();

    // stoi just takes the number out of the string and returns an int eg "s12" becomes 12
    return new GotoAction(static_cast<ParserStates>(stoi(word)));
  }
};

#endif
