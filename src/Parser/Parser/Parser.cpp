#include "Parser.h"
#include "createParseTable.h"
// adds the parser
Parser::Parser() { this->table = createParseTable(); }

auto Parser::ParseTokens(vector<Token *> tokens) {}
