#ifndef TOKEN_ENUM_H
#define TOKEN_ENUM_H

enum class TokenType {
  // punctuation
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  SEMI,
  ASSIGN,
  COLON,
  DOLLAR_EOF,

  // keywords
  MOD,
  VOID,
  RETURN,
  NUM_KEYWORD,
  NOP,
  COMMENT,
  ADD,
  SUB,
  MUL,
  DIV,
  NEG,
  IF,
  THEN,
  ELSE,
  NOT,
  AND,
  OR,
  EQ,
  LARGER,
  LESSER,
  DO,
  WHILE,
  UNTIL,
  PRINT,

  // the rest
  NAME,
  STRING,
  NUM,
};
#endif
