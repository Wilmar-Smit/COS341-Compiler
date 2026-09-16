// Table integrity tests.
//
// These exercise src/fileReader/parseTableReader.h (ParseTableReader /
// ActionTableReader / GotoTableReader) against the checked-in
// SLR_ACTION_Table.csv, SLR_GOTO_Table.csv and ProductionRules.txt.
//
// A couple of checks (header-column mapping) call the free functions
// `terminalFromSymbol` / `nonTerminalFromName` that parseTableReader.h
// already exposes at file scope - no production code changes needed.
//
// The "every ReduceAction's referenced production index is in range"
// check can't be done by inspecting a built ReduceAction directly:
// ReduceAction::rule is private with only ParseVisitor as a friend, so
// there is no public way to read back which production a given cell
// refers to. Two things stand in for it instead, without modifying
// production code:
//   1. ActionTableReader::create() calls ruleTable.at(value) for every
//      "rN" cell - if any reduce cell referenced a production number the
//      rules file doesn't define, .read() itself would throw. So a clean,
//      non-throwing read of the real table already proves every reduce
//      cell resolved to a real rule.
//   2. Independently (without touching ParseTableReader's internals), this
//      file re-parses ProductionRules.txt and SLR_ACTION_Table.csv's own
//      "rN" cells with a few lines of ad-hoc text parsing (plain text, not
//      XML - no need for the pugixml/no-hand-rolled-parsing rule that
//      applies to tree.xml) and cross-checks the referenced numbers
//      against the defined production range.

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "fileReader/parseTableReader.h"
#include "parser.actions.h"

namespace {

std::vector<std::string> splitCsvLine(const std::string &line) {
  std::vector<std::string> fields;
  std::stringstream ss(line);
  std::string field;
  while (std::getline(ss, field, ',')) fields.push_back(field);
  if (!line.empty() && line.back() == ',') fields.push_back("");
  return fields;
}

std::string stripCR(std::string s) {
  if (!s.empty() && s.back() == '\r') s.pop_back();
  return s;
}

}  // namespace

TEST_CASE("SLR_ACTION_Table.csv loads to exactly 143 states",
          "[table][action]") {
  ActionTableReader reader;
  std::vector<std::vector<ParserAction *>> table;
  REQUIRE_NOTHROW(table = reader.read("SLR_ACTION_Table.csv"));

  INFO("action table has " << table.size() << " rows");
  REQUIRE(table.size() == 143);

  for (std::size_t state = 0; state < table.size(); ++state) {
    INFO("state " << state << " row width");
    REQUIRE(table[state].size() == TOKEN_TYPE_COUNT);
    for (std::size_t col = 0; col < table[state].size(); ++col) {
      INFO("state " << state << ", column " << col
                     << " should never be a null cell (gaps default to "
                        "ErrorAction)");
      REQUIRE(table[state][col] != nullptr);
    }
  }

  for (auto &row : table)
    for (auto *action : row) delete action;
}

TEST_CASE("SLR_GOTO_Table.csv loads to exactly 143 states",
          "[table][goto]") {
  GotoTableReader reader;
  std::vector<std::vector<ParserAction *>> table;
  REQUIRE_NOTHROW(table = reader.read("SLR_GOTO_Table.csv"));

  INFO("goto table has " << table.size() << " rows");
  REQUIRE(table.size() == 143);

  for (std::size_t state = 0; state < table.size(); ++state) {
    INFO("state " << state << " row width");
    REQUIRE(table[state].size() == NONTERMINAL_COUNT);
    for (std::size_t col = 0; col < table[state].size(); ++col) {
      INFO("state " << state << ", column " << col
                     << " should never be a null cell");
      REQUIRE(table[state][col] != nullptr);
    }
  }

  for (auto &row : table)
    for (auto *action : row) delete action;
}

TEST_CASE("every SLR_ACTION_Table.csv header column maps to a TokenType",
          "[table][action][header]") {
  std::ifstream fin("SLR_ACTION_Table.csv");
  REQUIRE(fin.is_open());
  std::string header;
  std::getline(fin, header);
  header = stripCR(header);

  auto fields = splitCsvLine(header);
  REQUIRE(fields.size() > 1);
  REQUIRE(fields[0] == "State");

  for (std::size_t i = 1; i < fields.size(); ++i) {
    INFO("action table header column " << i << " label '" << fields[i]
                                        << "' should resolve to a TokenType");
    REQUIRE(terminalFromSymbol(fields[i]).has_value());
  }
}

TEST_CASE("every SLR_GOTO_Table.csv header column maps to a NonTerminal",
          "[table][goto][header]") {
  std::ifstream fin("SLR_GOTO_Table.csv");
  REQUIRE(fin.is_open());
  std::string header;
  std::getline(fin, header);
  header = stripCR(header);

  auto fields = splitCsvLine(header);
  REQUIRE(fields.size() > 1);
  REQUIRE(fields[0] == "State");

  for (std::size_t i = 1; i < fields.size(); ++i) {
    INFO("goto table header column " << i << " label '" << fields[i]
                                      << "' should resolve to a NonTerminal");
    REQUIRE(nonTerminalFromName(fields[i]).has_value());
  }
}

TEST_CASE("ProductionRules.txt defines the expected production set",
          "[table][rules]") {
  std::ifstream fin("ProductionRules.txt");
  REQUIRE(fin.is_open());

  std::set<int> definedNumbers;
  std::set<int> recognisedLhsNumbers;
  std::string line;
  while (std::getline(fin, line)) {
    if (line.empty()) continue;
    std::stringstream s(line);
    int number;
    std::string lhsName, arrow;
    s >> number >> lhsName >> arrow;
    definedNumbers.insert(number);
    if (nonTerminalFromName(lhsName)) recognisedLhsNumbers.insert(number);
  }

  // 44 lines total: production 0 is the augmented start rule
  // (SPL_PROG' -> SPL_PROG), which ParseTableReader::loadProductionRules
  // deliberately skips (its LHS isn't a real NonTerminal), leaving 43
  // reducible productions numbered 1..43.
  INFO("total numbered lines in ProductionRules.txt: "
       << definedNumbers.size());
  REQUIRE(definedNumbers.size() == 44);
  REQUIRE(definedNumbers.count(0) == 1);

  INFO("productions with a recognised (reducible) LHS: "
       << recognisedLhsNumbers.size());
  REQUIRE(recognisedLhsNumbers.size() == 43);
  REQUIRE(recognisedLhsNumbers.count(0) == 0);
  for (int n = 1; n <= 43; ++n) {
    INFO("production " << n << " should have a recognised LHS");
    REQUIRE(recognisedLhsNumbers.count(n) == 1);
  }
}

TEST_CASE(
    "every reduce cell in SLR_ACTION_Table.csv references an in-range "
    "production",
    "[table][action][reduce]") {
  std::ifstream fin("SLR_ACTION_Table.csv");
  REQUIRE(fin.is_open());

  std::string line;
  std::getline(fin, line);  // header

  int rowNumber = 0;
  while (std::getline(fin, line)) {
    line = stripCR(line);
    if (line.empty()) continue;
    auto fields = splitCsvLine(line);
    ++rowNumber;

    for (std::size_t col = 1; col < fields.size(); ++col) {
      const std::string &cell = fields[col];
      if (cell.empty() || cell[0] != 'r') continue;

      int productionNumber = std::stoi(cell.substr(1));
      INFO("row (state) " << (rowNumber - 1) << ", column " << col
                           << ", cell '" << cell
                           << "' should reference production 1..43");
      REQUIRE(productionNumber >= 1);
      REQUIRE(productionNumber <= 43);
    }
  }

  // Cross-check against the real reader: if any reduce cell referenced a
  // production number missing from ProductionRules.txt, ActionTableReader
  // would throw std::out_of_range building the ReduceAction for that cell.
  ActionTableReader reader;
  std::vector<std::vector<ParserAction *>> table;
  REQUIRE_NOTHROW(table = reader.read("SLR_ACTION_Table.csv"));
  for (auto &row : table)
    for (auto *action : row) delete action;
}
