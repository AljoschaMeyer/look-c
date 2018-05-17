#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "../src/lexer.h"

void test_empty(void) {
  Token t = tokenize("");
  assert(t.tt == END);
  assert(t.len == 0);
}

void test_simple(void) {
  char *src = "@~()[]";
  Token t = tokenize(src);

  assert(t.tt == AT);
  assert(t.len == 1);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == TILDE);
  assert(t.len == 1);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == LPAREN);
  assert(t.len == 1);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == RPAREN);
  assert(t.len == 1);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == LBRACKET);
  assert(t.len == 1);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == RBRACKET);
  assert(t.len == 1);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == END);
  assert(t.len == 0);

  // The whole lexer code is indirectly tested by the parser tests,
  // so there's really no reason to further extend this test case.
}

void test_simple_compound(void) {
  char *src = "#[";
  Token t = tokenize(src);

  assert(t.tt == BEGIN_ATTRIBUTE);
  assert(t.len == 2);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == END);
  assert(t.len == 0);
}

void test_compound(void) {
  char *src = "=>===&&&=&|||=|";
  Token t = tokenize(src);

  assert(t.tt == FAT_ARROW);
  assert(t.len == 2);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == EQUALS);
  assert(t.len == 2);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == EQ);
  assert(t.len == 1);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == LAND);
  assert(t.len == 2);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == AND_ASSIGN);
  assert(t.len == 2);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == AMPERSAND);
  assert(t.len == 1);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == LOR);
  assert(t.len == 2);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == OR_ASSIGN);
  assert(t.len == 2);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == PIPE);
  assert(t.len == 1);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == END);
  assert(t.len == 0);
}

void test_ws(void) {
  char *src = "  \n  //\n/// abc\n// \n";
  Token t = tokenize(src);
  assert(t.tt == END);
  assert(t.len == 20);
}

void test_id(void) {
  char *src = "_ __ _9_ a";
  Token t = tokenize(src);
  assert(t.tt == UNDERSCORE);
  assert(t.len == 1);
  assert(t.token_len == 1);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == ID);
  assert(t.len == 3);
  assert(t.token_len == 2);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == ID);
  assert(t.len == 4);
  assert(t.token_len == 3);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == ID);
  assert(t.len == 2);
  assert(t.token_len == 1);

  src += t.len;
  t = tokenize(src);
  assert(t.tt == END);
  assert(t.len == 0);

  t = tokenize(" a");
  assert(t.tt == ID);
  assert(t.len == 2);

  t = tokenize("use");
  assert(t.tt == USE);
  assert(t.len == 3);

  t = tokenize("mod");
  assert(t.tt == KW_MOD);
  assert(t.len == 3);

  t = tokenize("super");
  assert(t.tt == SUPER);
  assert(t.len == 5);

  t = tokenize("dep");
  assert(t.tt == DEP);
  assert(t.len == 3);

  t = tokenize("magic");
  assert(t.tt == MAGIC);
  assert(t.len == 5);

  t = tokenize("goto");
  assert(t.tt == GOTO);
  assert(t.len == 4);

  t = tokenize("label");
  assert(t.tt == LABEL);
  assert(t.len == 5);

  t = tokenize("break");
  assert(t.tt == BREAK);
  assert(t.len == 5);

  t = tokenize("return");
  assert(t.tt == RETURN);
  assert(t.len == 6);

  t = tokenize("if");
  assert(t.tt == IF);
  assert(t.len == 2);

  t = tokenize("else");
  assert(t.tt == ELSE);
  assert(t.len == 4);

  t = tokenize("case");
  assert(t.tt == CASE);
  assert(t.len == 4);

  t = tokenize("while");
  assert(t.tt == WHILE);
  assert(t.len == 5);

  t = tokenize("loop");
  assert(t.tt == LOOP);
  assert(t.len == 4);

  t = tokenize("as");
  assert(t.tt == AS);
  assert(t.len == 2);

  t = tokenize("val");
  assert(t.tt == VAL);
  assert(t.len == 3);

  t = tokenize("fn");
  assert(t.tt == FN);
  assert(t.len == 2);

  t = tokenize("type");
  assert(t.tt == TYPE);
  assert(t.len == 4);

  t = tokenize("macro");
  assert(t.tt == MACRO);
  assert(t.len == 5);

  t = tokenize("mut");
  assert(t.tt == MUT);
  assert(t.len == 3);

  t = tokenize("ffi");
  assert(t.tt == FFI);
  assert(t.len == 3);

  t = tokenize("pub ");
  assert(t.tt == PUB);
  assert(t.len == 3);

  t = tokenize(" pub ");
  assert(t.tt == PUB);
  assert(t.len == 4);

  t = tokenize("sizeof");
  assert(t.tt == SIZEOF);
  assert(t.len == 6);

  t = tokenize("alignof");
  assert(t.tt == ALIGNOF);
  assert(t.len == 7);
}

void test_number(void) {
  Token t = tokenize("001");
  assert(t.tt == INT);
  assert(t.len == 3);

  t = tokenize("0.0");
  assert(t.tt == FLOAT);
  assert(t.len == 3);

  t = tokenize("0.0e0");
  assert(t.tt == FLOAT);
  assert(t.len == 5);

  t = tokenize("0.0e-0");
  assert(t.tt == FLOAT);
  assert(t.len == 6);
}

void test_string(void) {
  Token t = tokenize("\"\"");
  assert(t.tt == STRING);
  assert(t.len == 2);

  t = tokenize("\"abc\"");
  assert(t.tt == STRING);
  assert(t.len == 5);

  t = tokenize("\"\\\\\"");
  assert(t.tt == STRING);
  assert(t.len == 4);

  t = tokenize("\"\\0\"");
  assert(t.tt == STRING);
  assert(t.len == 4);

  t = tokenize("\"\\n\"");
  assert(t.tt == STRING);
  assert(t.len == 4);

  t = tokenize("\"\\\"\"");
  assert(t.tt == STRING);
  assert(t.len == 4);

  t = tokenize("\"\\u09AF\"");
  assert(t.tt == STRING);
  assert(t.len == 8);

  t = tokenize("\"\\u09AF45BD\"");
  assert(t.tt == STRING);
  assert(t.len == 12);
}

void test_error(void) {
  Token t00 = tokenize("#a");
  assert(t00.tt == ERR_BEGIN_ATTRIBUTE);
  assert(t00.len == 1);

  Token t01 = tokenize("#");
  assert(t01.tt == ERR_BEGIN_ATTRIBUTE);
  assert(t01.len == 1);

  Token t02 = tokenize("//");
  assert(t02.tt == ERR_EOF);
  assert(t02.len == 2);

  Token t03 = tokenize("///");
  assert(t03.tt == ERR_EOF);
  assert(t03.len == 3);

  Token t04 = tokenize("// ");
  assert(t04.tt == ERR_EOF);
  assert(t04.len == 3);

  Token t05 = tokenize("\t");
  assert(t05.tt == ERR_TAB);
  assert(t05.len == 0);

  Token t06 = tokenize("\r");
  assert(t06.tt == ERR_CARRIAGE);
  assert(t06.len == 0);

  Token t07 = tokenize("ä");
  assert(t07.tt == ERR_UNKNOWN);
  assert(t07.len == 0);

  Token t08 = tokenize("\"");
  assert(t08.tt == ERR_EOF);
  assert(t08.len == 1);

  Token t09 = tokenize("\"\\");
  assert(t09.tt == ERR_EOF);
  assert(t09.len == 2);

  Token t10 = tokenize("\"\\t\"");
  assert(t10.tt == ERR_INVALID_ESCAPE);
  assert(t10.len == 3);

  // TODO eof in u or U escape, invalid u or U escapes
}

int main(void)
{
  test_empty();
  test_simple();
  test_simple_compound();
  test_compound();
  test_ws();
  test_id();
  test_number();
  test_string();
  test_error();

  return 0;
}
