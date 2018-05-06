#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"

const char * token_type_error(TokenType tt) {
  if (tt == ERR_BEGIN_ATTRIBUTE) {
    return "# must be followed by [, to begin an attribute";
  } else if (tt == ERR_EOF) {
    return "Reached unexpected end of file inside a token";
  } if (tt == ERR_TAB) {
    return "Tabs are not allowed as whitespace, use spaces instead";
  } else if (tt == ERR_CARRIAGE) {
    return "Carriage returns (\"\\r\") are not allowed as whitespace, use newlines (\"\\n\" instead)";
  } else if (tt == ERR_UNKNOWN) {
    return "Encountered disallowed character, only printable ascii characters are allowed outside of comments and string literals";
  } else if (tt == ERR_LOWER_HEX) {
    return "Hexadecimal escapes must use upper case letters (ABCDEF), not lower case";
  } else if (tt == ERR_NON_HEX) {
    return "Expected hexadecimal character (0123456789ABCDEF)";
  } else {
    return NULL;
  }
}

typedef enum {
  S_INITIAL,
  S_HASH, // #[
  S_EQ, // = or => or ==
  S_COLON, // : or ::
  S_PLUS, // + or +=
  S_MINUS, // - or -= or ->
  S_TIMES, // * or *=
  S_DIV, // / or /= or //
  S_MOD, // % or %=
  S_PIPE, // | or || or |=
  S_HAT, // ^ or ^=
  S_AMPERSAND, // & or && or &=
  S_COMMENT_START, // //
  S_COMMENT, // //foo
  S_DOC_COMMENT, // ///foo
  S_WS, // only space and \n
  S_UNDERSCORE, // blank pattern or identifier
  S_ID,
  S_INT,
  S_STRING,
  S_STRING_BACKSLASH,
  S_STRING_u_0, S_STRING_u_1, S_STRING_u_2, S_STRING_u_3,
  S_STRING_U_0, S_STRING_U_1, S_STRING_U_2, S_STRING_U_3,
  S_STRING_U_4, S_STRING_U_5, S_STRING_U_6, S_STRING_U_7
} State;

static bool is_id_char(char c) {
  return (c == '_') || ('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

Token tokenize(const char *src) {
  State s = S_INITIAL;
  size_t len = 0;
  char c;
  TokenType tt;
  Token ret;

  while (true) {
    c = src[len];
    len += 1;

    switch (s) {
      case S_INITIAL:
        if (c == 0) {
          tt = END;
          len = 0;
          goto done;
        } else if (c == '@') {
          tt = AT;
          goto done;
        } else if (c == '~') {
          tt = TILDE;
          goto done;
        } else if (c == '(') {
          tt = LPAREN;
          goto done;
        } else if (c == ')') {
          tt = RPAREN;
          goto done;
        } else if (c == '[') {
          tt = LBRACKET;
          goto done;
        } else if (c == ']') {
          tt = RBRACKET;
          goto done;
        } else if (c == '{') {
          tt = LBRACE;
          goto done;
        } else if (c == '}') {
          tt = RBRACKET;
          goto done;
        } else if (c == '<') {
          tt = LANGLE;
          goto done;
        } else if (c == '>') {
          tt = RANGLE;
          goto done;
        } else if (c == ',') {
          tt = COMMA;
          goto done;
        } else if (c == '$') {
          tt = DOLLAR;
          goto done;
        } else if (c == ';') {
          tt = SEMI;
          goto done;
        } else if (c == '.') {
          tt = DOT;
          goto done;
        } else if (c == '\t') {
          tt = ERR_TAB;
          len = 0;
          goto done;
        } else if (c == '\r') {
          tt = ERR_CARRIAGE;
          len = 0;
          goto done;
        } else if (c == '#') {
          s = S_HASH;
        } else if (c == '&') {
          s = S_AMPERSAND;
        } else if (c == '|') {
          s = S_PIPE;
        } else if (c == '=') {
          s = S_EQ;
        } else if (c == ':') {
          s = S_COLON;
        } else if (c == '+') {
          s = S_PLUS;
        } else if (c == '-') {
          s = S_MINUS;
        } else if (c == '*') {
          s = S_TIMES;
        } else if (c == '/') {
          s = S_DIV;
        } else if (c == '%') {
          s = S_MOD;
        } else if (c == '^') {
          s = S_HAT;
        } else if (c == ' ' || c == '\n') {
          s = S_WS;
          tt = WS;
        } else if (c == '_') {
          s = S_UNDERSCORE;
        } else if ('0' <= c && c <= '9') {
          s = S_INT;
          tt = INT;
        } else if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
          s = S_ID;
          tt = ID;
        } else if (c == '"') {
          s = S_STRING;
          tt = STRING;
        } else {
          tt = ERR_UNKNOWN;
          len -= 1;
          goto done;
        }
        break;
      case S_HASH:
        if (c == '[') {
          tt = BEGIN_ATTRIBUTE;
          goto done;
        } else {
          tt = ERR_BEGIN_ATTRIBUTE;
          len = 1;
          goto done;
        }
      case S_AMPERSAND:
        if (c == '&') {
          tt = LAND;
          goto done;
        } else if (c == '=') {
          tt = AND_ASSIGN;
          goto done;
        } else {
          tt = AMPERSAND;
          len = 1;
          goto done;
        }
      case S_PIPE:
        if (c == '|') {
          tt = LOR;
          goto done;
        } else if (c == '=') {
          tt = OR_ASSIGN;
          goto done;
        } else {
          tt = PIPE;
          len = 1;
          goto done;
        }
      case S_EQ:
        if (c == '>') {
          tt = FAT_ARROW;
          goto done;
        } else if (c == '=') {
          tt = EQUALS;
          goto done;
        } else {
          tt = EQ;
          len = 1;
          goto done;
        }
      case S_COLON:
        if (c == ':') {
          tt = SCOPE;
          goto done;
        } else {
          tt = COLON;
          len = 1;
          goto done;
        }
      case S_PLUS:
        if (c == '=') {
          tt = PLUS_ASSIGN;
          goto done;
        } else {
          tt = PLUS;
          len = 1;
          goto done;
        }
      case S_MINUS:
        if (c == '=') {
          tt = MINUS_ASSIGN;
          goto done;
        } else if (c == '>') {
          tt = ARROW;
          goto done;
        } else {
          tt = MINUS;
          len = 1;
          goto done;
        }
      case S_TIMES:
        if (c == '=') {
          tt = TIMES_ASSIGN;
          goto done;
        } else {
          tt = TIMES;
          len = 1;
          goto done;
        }
      case S_DIV:
        if (c == '=') {
          tt = DIV_ASSIGN;
          goto done;
        } if (c == '/') {
          tt = COMMENT;
          s = S_COMMENT_START;
          break;
        } else {
          tt = DIV;
          len = 1;
          goto done;
        }
      case S_MOD:
        if (c == '=') {
          tt = MOD_ASSIGN;
          goto done;
        } else {
          tt = MOD;
          len = 1;
          goto done;
        }
      case S_HAT:
        if (c == '=') {
          tt = XOR_ASSIGN;
          goto done;
        } else {
          tt = XOR;
          len = 1;
          goto done;
        }
      case S_COMMENT_START:
        if (c == '/') {
          tt = DOC_COMMENT;
          s = S_DOC_COMMENT;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else {
          s = S_COMMENT;
        }
        break;
      case S_DOC_COMMENT:
        if (c == '\n') {
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        }
        break;
      case S_COMMENT:
        if (c == '\n') {
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        }
        break;
      case S_WS:
        if (!(c == ' ' || c == '\n')) {
          len -= 1;
          goto done;
        }
        break;
      case S_UNDERSCORE:
        if (is_id_char(c)) {
          tt = ID;
          s = S_ID;
        } else {
          tt = UNDERSCORE;
          len -= 1;
          goto done;
        }
        break;
      case S_ID:
        if (!is_id_char(c)) {
          len -= 1;

          if (strcmp(src, "use") == 0) {
            tt = USE;
          } else if (strcmp(src, "mod") == 0) {
            tt = KW_MOD;
          } else if (strcmp(src, "super") == 0) {
            tt = SUPER;
          } else if (strcmp(src, "self") == 0) {
            tt = SELF;
          } else if (strcmp(src, "dep") == 0) {
            tt = DEP;
          } else if (strcmp(src, "magic") == 0) {
            tt = MAGIC;
          } else if (strcmp(src, "goto") == 0) {
            tt = GOTO;
          } else if (strcmp(src, "label") == 0) {
            tt = LABEL;
          } else if (strcmp(src, "break") == 0) {
            tt = BREAK;
          } else if (strcmp(src, "return") == 0) {
            tt = RETURN;
          } else if (strcmp(src, "if") == 0) {
            tt = IF;
          } else if (strcmp(src, "else") == 0) {
            tt = ELSE;
          } else if (strcmp(src, "while") == 0) {
            tt = WHILE;
          } else if (strcmp(src, "loop") == 0) {
            tt = LOOP;
          } else if (strcmp(src, "case") == 0) {
            tt = CASE;
          } else if (strcmp(src, "as") == 0) {
            tt = AS;
          } else if (strcmp(src, "val") == 0) {
            tt = VAL;
          } else if (strcmp(src, "fn") == 0) {
            tt = FN;
          } else if (strcmp(src, "type") == 0) {
            tt = TYPE;
          } else if (strcmp(src, "macro") == 0) {
            tt = MACRO;
          } else if (strcmp(src, "mut") == 0) {
            tt = MUT;
          } else if (strcmp(src, "pub") == 0) {
            tt = PUB;
          } else if (strcmp(src, "ffi") == 0) {
            tt = FFI;
          } else if (strcmp(src, "sizeof") == 0) {
            tt = SIZEOF;
          } else if (strcmp(src, "alignof") == 0) {
            tt = ALIGNOF;
          }

          goto done;
        }
        break;
      case S_INT:
        if (!('0' <= c && c <= '9')) {
          len -= 1;
          goto done;
        }
        break;
      case S_STRING:
        if (c == '"') {
          goto done;
        } else if (c == '\\') {
          s = S_STRING_BACKSLASH;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        }
        break;
      case S_STRING_BACKSLASH:
        if (c == '0' || c == '\\' || c == '"' || c == 'n') {
          s = S_STRING;
        } else if (c == 'u') {
          s = S_STRING_u_0;
        } else if (c == 'U') {
          s = S_STRING_U_0;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else {
          tt = ERR_INVALID_ESCAPE;
          goto done;
        }
        break;
      case S_STRING_u_0:
        if ('a' <= c && c <= 'f') {
          tt = ERR_LOWER_HEX;
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))) {
          tt = ERR_NON_HEX;
          goto done;
        } else {
          s = S_STRING_u_1;
        }
        break;
      case S_STRING_u_1:
        if ('a' <= c && c <= 'f') {
          tt = ERR_LOWER_HEX;
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))) {
          tt = ERR_NON_HEX;
          goto done;
        } else {
          s = S_STRING_u_2;
        }
        break;
      case S_STRING_u_2:
        if ('a' <= c && c <= 'f') {
          tt = ERR_LOWER_HEX;
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))) {
          tt = ERR_NON_HEX;
          goto done;
        } else {
          s = S_STRING_u_3;
        }
        break;
      case S_STRING_u_3:
        if ('a' <= c && c <= 'f') {
          tt = ERR_LOWER_HEX;
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))) {
          tt = ERR_NON_HEX;
          goto done;
        } else {
          s = S_STRING;
        }
        break;
      case S_STRING_U_0:
        if ('a' <= c && c <= 'f') {
          tt = ERR_LOWER_HEX;
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))) {
          tt = ERR_NON_HEX;
          goto done;
        } else {
          s = S_STRING_U_1;
        }
        break;
      case S_STRING_U_1:
        if ('a' <= c && c <= 'f') {
          tt = ERR_LOWER_HEX;
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))) {
          tt = ERR_NON_HEX;
          goto done;
        } else {
          s = S_STRING_U_2;
        }
        break;
      case S_STRING_U_2:
        if ('a' <= c && c <= 'f') {
          tt = ERR_LOWER_HEX;
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))) {
          tt = ERR_NON_HEX;
          goto done;
        } else {
          s = S_STRING_U_3;
        }
        break;
      case S_STRING_U_3:
        if ('a' <= c && c <= 'f') {
          tt = ERR_LOWER_HEX;
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))) {
          tt = ERR_NON_HEX;
          goto done;
        } else {
          s = S_STRING_U_4;
        }
        break;
      case S_STRING_U_4:
        if ('a' <= c && c <= 'f') {
          tt = ERR_LOWER_HEX;
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))) {
          tt = ERR_NON_HEX;
          goto done;
        } else {
          s = S_STRING_U_5;
        }
        break;
      case S_STRING_U_5:
        if ('a' <= c && c <= 'f') {
          tt = ERR_LOWER_HEX;
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))) {
          tt = ERR_NON_HEX;
          goto done;
        } else {
          s = S_STRING_U_6;
        }
        break;
      case S_STRING_U_6:
        if ('a' <= c && c <= 'f') {
          tt = ERR_LOWER_HEX;
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))) {
          tt = ERR_NON_HEX;
          goto done;
        } else {
          s = S_STRING_U_7;
        }
        break;
      case S_STRING_U_7:
        if ('a' <= c && c <= 'f') {
          tt = ERR_LOWER_HEX;
          goto done;
        } else if (c == 0) {
          tt = ERR_EOF;
          len -= 1;
          goto done;
        } else if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'))) {
          tt = ERR_NON_HEX;
          goto done;
        } else {
          s = S_STRING;
        }
        break;
    }
  }

  done:
  ret.tt = tt;
  ret.len = len;
  return ret;
}

