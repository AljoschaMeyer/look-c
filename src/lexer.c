#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "util.h"

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
  } else if (tt == ERR_FLOAT_NO_DECIMALS) {
    return "Floating point literals must have at least one decimal";
  } else if (tt == ERR_FLOAT_NO_EXPONENT) {
    return "Expected an exponent for the floating point literal";
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
  S_COMMENT, // //foo
  S_UNDERSCORE, // blank pattern or identifier
  S_ID,
  S_INT,
  S_INT_DOT,
  S_FLOAT,
  S_FLOAT_E,
  S_FLOAT_E_MINUS,
  S_FLOAT_EXPONENT,
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
  size_t token_start = 0;
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
          len -= 1;
          goto done;
        } else if (c == '@') {
          tt = AT;
          goto done;
        } else if (c == '~') {
          tt = TILDE;
          goto done;
        } else if (c == '!') {
          tt = NOT;
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
          tt = RBRACE;
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
          len -= 1;
          goto done;
        } else if (c == '\r') {
          tt = ERR_CARRIAGE;
          len -= 1;
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
          token_start += 1;
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
          len -= 1;
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
          len -= 1;
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
          len -= 1;
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
          len -= 1;
          goto done;
        }
      case S_COLON:
        if (c == ':') {
          tt = SCOPE;
          goto done;
        } else {
          tt = COLON;
          len -= 1;
          goto done;
        }
      case S_PLUS:
        if (c == '=') {
          tt = PLUS_ASSIGN;
          goto done;
        } else {
          tt = PLUS;
          len -= 1;
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
          len -= 1;
          goto done;
        }
      case S_TIMES:
        if (c == '=') {
          tt = TIMES_ASSIGN;
          goto done;
        } else {
          tt = TIMES;
          len -= 1;
          goto done;
        }
      case S_DIV:
        if (c == '=') {
          tt = DIV_ASSIGN;
          goto done;
        } if (c == '/') {
          token_start += 2;
          s = S_COMMENT;
          break;
        } else {
          tt = DIV;
          len -= 1;
          goto done;
        }
      case S_MOD:
        if (c == '=') {
          tt = MOD_ASSIGN;
          goto done;
        } else {
          tt = MOD;
          len -= 1;
          goto done;
        }
      case S_HAT:
        if (c == '=') {
          tt = XOR_ASSIGN;
          goto done;
        } else {
          tt = XOR;
          len -= 1;
          goto done;
        }
      case S_COMMENT:
        token_start += 1;
        if (c == '\n') {
          s = S_INITIAL;
        } else if (c == 0) {
          tt = ERR_EOF;
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
          Str s;
          s.start = src + token_start;
          s.len = len - token_start;

          Str s_use;
          s_use.start = "use";
          s_use.len = 3;

          Str s_mod;
          s_mod.start = "mod";
          s_mod.len = 3;

          Str s_dep;
          s_dep.start = "dep";
          s_dep.len = 3;

          Str s_magic;
          s_magic.start = "magic";
          s_magic.len = 5;

          Str s_goto;
          s_goto.start = "goto";
          s_goto.len = 4;

          Str s_label;
          s_label.start = "label";
          s_label.len = 5;

          Str s_break;
          s_break.start = "break";
          s_break.len = 5;

          Str s_return;
          s_return.start = "return";
          s_return.len = 6;

          Str s_if;
          s_if.start = "if";
          s_if.len = 2;

          Str s_else;
          s_else.start = "else";
          s_else.len = 4;

          Str s_while;
          s_while.start = "while";
          s_while.len = 5;

          Str s_loop;
          s_loop.start = "loop";
          s_loop.len = 4;

          Str s_case;
          s_case.start = "case";
          s_case.len = 4;

          Str s_as;
          s_as.start = "as";
          s_as.len = 2;

          Str s_val;
          s_val.start = "val";
          s_val.len = 3;

          Str s_fn;
          s_fn.start = "fn";
          s_fn.len = 2;

          Str s_type;
          s_type.start = "type";
          s_type.len = 4;

          Str s_macro;
          s_macro.start = "macro";
          s_macro.len = 5;

          Str s_mut;
          s_mut.start = "mut";
          s_mut.len = 3;

          Str s_pub;
          s_pub.start = "pub";
          s_pub.len = 3;

          Str s_ffi;
          s_ffi.start = "ffi";
          s_ffi.len = 3;

          Str s_size_of;
          s_size_of.start = "sizeof";
          s_size_of.len = 6;

          Str s_align_of;
          s_align_of.start = "alignof";
          s_align_of.len = 7;

          if (str_eq(s, s_use)) {
            tt = USE;
          } else if (str_eq(s, s_mod)) {
            tt = KW_MOD;
          } else if (str_eq(s, s_dep)) {
            tt = DEP;
          } else if (str_eq(s, s_magic)) {
            tt = MAGIC;
          } else if (str_eq(s, s_goto)) {
            tt = GOTO;
          } else if (str_eq(s, s_label)) {
            tt = LABEL;
          } else if (str_eq(s, s_break)) {
            tt = BREAK;
          } else if (str_eq(s, s_return)) {
            tt = RETURN;
          } else if (str_eq(s, s_if)) {
            tt = IF;
          } else if (str_eq(s, s_else)) {
            tt = ELSE;
          } else if (str_eq(s, s_while)) {
            tt = WHILE;
          } else if (str_eq(s, s_loop)) {
            tt = LOOP;
          } else if (str_eq(s, s_case)) {
            tt = CASE;
          } else if (str_eq(s, s_as)) {
            tt = AS;
          } else if (str_eq(s, s_val)) {
            tt = VAL;
          } else if (str_eq(s, s_fn)) {
            tt = FN;
          } else if (str_eq(s, s_type)) {
            tt = TYPE;
          } else if (str_eq(s, s_macro)) {
            tt = MACRO;
          } else if (str_eq(s, s_mut)) {
            tt = MUT;
          } else if (str_eq(s, s_pub)) {
            tt = PUB;
          } else if (str_eq(s, s_ffi)) {
            tt = FFI;
          } else if (str_eq(s, s_size_of)) {
            tt = SIZEOF;
          } else if (str_eq(s, s_align_of)) {
            tt = ALIGNOF;
          }

          goto done;
        }
        break;
      case S_INT:
        if (c == '.') {
          s = S_INT_DOT;
        } else if (!('0' <= c && c <= '9')) {
          len -= 1;
          goto done;
        }
        break;
      case S_INT_DOT:
        if (!('0' <= c && c <= '9')) {
          len -= 1;
          tt = ERR_FLOAT_NO_DECIMALS;
          goto done;
        } else {
          tt = FLOAT;
          s = S_FLOAT;
        }
        break;
      case S_FLOAT:
        if (c == 'e') {
          s = S_FLOAT_E;
        } else if (!('0' <= c && c <= '9')) {
          len -= 1;
          goto done;
        }
        break;
      case S_FLOAT_E:
        if (c == '-') {
          s = S_FLOAT_E_MINUS;
        } else if ('0' <= c && c <= '9') {
          s = S_FLOAT_EXPONENT;
        } else {
          len -= 1;
          tt = ERR_FLOAT_NO_EXPONENT;
          goto done;
        }
        break;
      case S_FLOAT_E_MINUS:
        if ('0' <= c && c <= '9') {
          s = S_FLOAT_EXPONENT;
        } else {
          len -= 1;
          tt = ERR_FLOAT_NO_EXPONENT;
          goto done;
        }
        break;
      case S_FLOAT_EXPONENT:
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
  ret.token_len = len - token_start;
  return ret;
}
