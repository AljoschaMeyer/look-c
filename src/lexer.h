#ifndef OO_LEX_H
#define OO_LEX_H

#include <stddef.h>

// The different token types the lexer can emit.
typedef enum {
  AT, TILDE, EQ, LPAREN, RPAREN, LBRACKET, RBRACKET, LBRACE, RBRACE, DOT,
  LANGLE, RANGLE, COMMA, DOLLAR, PLUS, MINUS, TIMES, DIV, MOD, COLON,
  SEMI, PIPE, SCOPE, ARROW, FAT_ARROW, BEGIN_ATTRIBUTE, UNDERSCORE,
  ID, INT, FLOAT, STRING, USE, KW_MOD, SELF, SUPER, DEP, MAGIC, GOTO,
  LABEL, BREAK, RETURN, IF, ELSE, WHILE, CASE, LOOP, AS, VAL, TYPE,
  MACRO, MUT, PUB, FFI, SIZEOF, ALIGNOF, PLUS_ASSIGN, MINUS_ASSIGN,
  TIMES_ASSIGN, DIV_ASSIGN, MOD_ASSIGN, XOR_ASSIGN, AND_ASSIGN, OR_ASSIGN,
  SHIFTL_ASSIGN,  SHIFTR_ASSIGN, XOR, AMPERSAND, LOR, LAND, NOT, EQUALS,
  NOTEQUALS,  GREATER, GREATER_EQ, SMALLER, SMALLER_EQ, END, COMMENT,
  DOC_COMMENT, WS, ERR_BEGIN_ATTRIBUTE,
  ERR_EOF, ERR_TAB, ERR_CARRIAGE, ERR_UNKNOWN, ERR_INVALID_ESCAPE,
  ERR_LOWER_HEX, ERR_NON_HEX
} TokenType;

// Returns a string with a a human-readable error message for the TokenType
// if it is an error type, or NULL otherwise.
const char * token_type_error(TokenType tt);

// A TokenType, together with the lenght of the token.
typedef struct Token {
  TokenType tt;
  size_t len;
} Token;

// Returns the first token of the given string. To tokenize a whole string,
// repeatedly call this function, always advancing the char pointer by the
// len of the previously returned token.
Token tokenize(const char *src);

#endif
