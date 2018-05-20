#ifndef OO_PARSER_H
#define OO_PARSER_H

#include <stddef.h>

#include "asg.h"
#include "lexer.h"

typedef enum {
  ERR_NONE,
  ERR_SID,
  ERR_ID,
  ERR_MACRO_INV,
  ERR_LITERAL,
  ERR_REPEAT,
  ERR_BIN_OP,
  ERR_ASSIGN_OP,
  ERR_TYPE,
  ERR_SUMMAND,
  ERR_PATTERN,
  ERR_EXP,
  ERR_BLOCK,
  ERR_ATTR,
  ERR_META,
  ERR_USE_TREE,
  ERR_ITEM,
  ERR_FILE
} ParserErrorTag;

typedef struct ParserError {
  ParserErrorTag tag;
  TokenType tt;
} ParserError;

// All parser functions return how many bytes of the input they consumed.
// The first argument is the string (null-terminated) from which to parse.
// Errors are signaled via the second argument.
// The actual parsed data is populated via the third argument.

size_t parse_file(const char *src, ParserError *err, AsgFile *data);
size_t parse_meta(const char *src, ParserError *err, AsgMeta *data);
size_t parse_item(const char *src, ParserError *err, AsgItem *data);
size_t parse_use_tree(const char *src, ParserError *err, AsgUseTree *data);
size_t parse_item_type(const char *src, ParserError *err, AsgItemType *data);
size_t parse_type(const char *src, ParserError *err, AsgType *data);
size_t parse_summand(const char *src, ParserError *err, AsgSummand *data);
size_t parse_item_val(const char *src, ParserError *err, AsgItemVal *data);
size_t parse_exp(const char *src, ParserError *err, AsgExp *data);
size_t parse_block(const char *src, ParserError *err, AsgBlock *data);
size_t parse_pattern(const char *src, ParserError *err, AsgPattern *data);
size_t parse_item_fun(const char *src, ParserError *err, AsgItemFun *data);
size_t parse_item_ffi_include(const char *src, ParserError *err, AsgItemFfiInclude *data);
size_t parse_item_ffi_val(const char *src, ParserError *err, AsgItemFfiVal *data);
size_t parse_id(const char *src, ParserError *err, AsgId *data);
size_t parse_sid(const char *src, ParserError *err, AsgSid *data);
size_t parse_macro_inv(const char *src, ParserError *err, AsgMacroInv *data);
size_t parse_literal(const char *src, ParserError *err, AsgLiteral *data);
size_t parse_repeat(const char *src, ParserError *err, AsgRepeat *data);

// The free_inner_foo functions free the child nodes of their argument (but
// not the argument itself).

void free_inner_id(AsgId data);
void free_inner_repeat(AsgRepeat data);
void free_inner_type(AsgType data);
void free_inner_summand(AsgSummand data);
void free_inner_pattern(AsgPattern data);
void free_inner_exp(AsgExp data);
void free_inner_block(AsgBlock data);
void free_inner_meta(AsgMeta data);
void free_inner_use_tree(AsgUseTree data);
void free_inner_item(AsgItem data);
void free_inner_file(AsgFile data);

// Free a stretchy buffer and recursively free its contents
void free_sb_meta(AsgMeta *sb);
void free_sb_exp(AsgExp *sb);

#endif
