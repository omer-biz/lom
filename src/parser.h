// A C parser-combinator host with Lua bindings and correct memory management.
// Compile (example): gcc -std=c11 parser.c -o parser -llua
// (adjust -llua path/names per your platform)

#ifndef __PARSER_LUA

#include <ctype.h>
#include <lauxlib.h>
#include <lua.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int ok;           // 1 success, 0 failure
  const char *rest; // pointer into original input (not owned)
  int lua_ref;      // store the result in the register
} ParseResult;

static ParseResult parse_ok(const char *rest, int lua_ref);
static ParseResult parse_err(const char *input);

/* ---------------------------
   Parser type + refcount
   --------------------------- */

typedef struct Parser Parser;
typedef ParseResult (*parse_fn_t)(Parser *p, const char *input);
typedef void (*destroy_fn_t)(Parser *p);

typedef enum {
  P_LITERAL,
  P_ANY_CHAR,
  P_IDENTIFIER,
  P_MAP,
  P_AND_THEN,
  P_OR_ELSE,
  P_PRED,
  P_ONE_OR_MORE,
  P_ZERO_OR_MORE,
  P_LEFT,
  P_RIGHT,
  P_PAIR,
} ParserKind;

struct Parser {
  ParserKind kind;
  parse_fn_t parse;
  destroy_fn_t destroy;
  void *data;
  int refcount;
  // store pointer to lua_State used to register callbacks (not owned)
  lua_State *L;
};

static Parser *parser_new(ParserKind k, parse_fn_t parse, destroy_fn_t destroy,
                          void *data, lua_State *L);

static void parser_ref(Parser *p);
static void parser_unref(Parser *p);

/* ---------------------------
   Literal parser
   --------------------------- */

typedef struct {
  char *lit; // malloc'd
} LiteralData;

static ParseResult literal_parse(Parser *p, const char *input);
static void literal_destroy(Parser *p);
static Parser *make_literal(lua_State *L, const char *s);

/* ---------------------------
   any_char parser
   --------------------------- */

typedef struct {
  int dummy;
} AnyCharData;

static ParseResult any_char_parse(Parser *p, const char *input);
static void any_char_destroy(Parser *p);
static Parser *make_any_char(lua_State *L);

/* ---------------------------
   identifier parser
   --------------------------- */

typedef struct {
  int dummy;
} IdentData;

static ParseResult identifier_parse(Parser *p, const char *input);
static void identifier_destroy(Parser *p);
static Parser *make_identifier(lua_State *L);

/* ---------------------------
   Map combinator
   data: inner Parser*, int func_ref
   --------------------------- */

typedef struct {
  Parser *inner;
  int func_ref; // registry ref to Lua function
} MapData;

static ParseResult map_parse(Parser *p, const char *input);
static void map_destroy(Parser *p);
static Parser *make_map(lua_State *L, Parser *inner, int func_ref);

/* ---------------------------
   and_then combinator
   data: inner Parser*, int func_ref (lua function returning parser userdata)
   --------------------------- */

typedef struct {
  Parser *inner;
  int func_ref; // lua function: (string) -> parser userdata
} AndThenData;

static ParseResult and_then_parse(Parser *p, const char *input);
static void and_then_destroy(Parser *p);
static Parser *make_and_then(lua_State *L, Parser *inner, int func_ref);

/* ---------------------------
   or_else combinator
   data: left Parser*, right Parser*
   --------------------------- */

typedef struct {
  Parser *left;
  Parser *right;
} OrData;

static ParseResult or_parse(Parser *p, const char *input);
static void or_destroy(Parser *p);
static Parser *make_or(lua_State *L, Parser *a, Parser *b);

/* ---------------------------
   pred combinator
   data: inner Parser*, int func_ref (lua predicate)
   --------------------------- */

typedef struct {
  Parser *inner;
  int func_ref; // lua function: (string) -> boolean
} PredData;

static ParseResult pred_parse(Parser *p, const char *input);
static void pred_destroy(Parser *p);
static Parser *make_pred(lua_State *L, Parser *inner, int func_ref);

/* ---------------------------
  left combinator (returns left output)
  data: left Parser*, right Parser*
   --------------------------- */

typedef struct {
  Parser *left;
  Parser *right;
} LeftData;

static ParseResult left_parse(Parser *p, const char *input);
static void left_destroy(Parser *p);
static Parser *make_left(lua_State *L, Parser *left, Parser *right);

/* ---------------------------
  right combinator (returns right output)
  data: left Parser*, right Parser*
   --------------------------- */

typedef struct {
  Parser *left;
  Parser *right;
} RightData;

static ParseResult right_parse(Parser *p, const char *input);
static void right_destroy(Parser *p);
static Parser *make_right(lua_State *L, Parser *left, Parser *right);

/* ---------------------------
   repetition combinators
   one_or_more and zero_or_more: they concatenate outputs into one malloc'd
   string
   --------------------------- */

typedef struct {
  Parser *inner;
} RepData;

static ParseResult one_or_more_parse(Parser *p, const char *input);
static ParseResult zero_or_more_parse(Parser *p, const char *input);
static void rep_destroy(Parser *p);
static Parser *make_one_or_more(lua_State *L, Parser *inner);
static Parser *make_zero_or_more(lua_State *L, Parser *inner);

/* ---------------------------
   pair combinator
   returns a table contating the result from both parsers
   ---------------------------  */

typedef struct {
  Parser *left;
  Parser *right;
} PairData;

static ParseResult pair_parse(Parser *p, const char *input);
static void pair_destroy(Parser *p);
static Parser *make_pair(lua_State *L, Parser *left, Parser *right);

/* ---------------------------
   Lua userdata helpers
   --------------------------- */

static Parser *check_parser_ud(lua_State *L, int idx);
static void push_parser_ud(lua_State *L, Parser *p);

/* ---------------------------
   Lua-exposed constructors / methods
   --------------------------- */

/* parser.literal(s) */
static int l_parser_literal(lua_State *L);

/* parser.any_char() */
static int l_parser_any_char(lua_State *L);

/* parser.identifier() */
static int l_parser_identifier(lua_State *L);

/* p:map(function) */
static int l_parser_map(lua_State *L);

/* p:map(function) */
static int l_parser_map(lua_State *L);

/* p:and_then(function) -> function must return a Parser userdata */
static int l_parser_and_then(lua_State *L);

/* p:or_else(q) */
static int l_parser_or_else(lua_State *L);

/* p:pred(function) */
static int l_parser_pred(lua_State *L);

/* p:one_or_more() */
static int l_parser_one_or_more(lua_State *L);

/* p:zero_or_more() */
static int l_parser_zero_or_more(lua_State *L);

/* p:parse(input) -> returns output (string or nil) , rest (string) */
static int l_parser_parse(lua_State *L);

int luaopen_parser(lua_State *L);

#define __PARSER_LUA
#endif
