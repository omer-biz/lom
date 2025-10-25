#ifndef __PARSER_LUA
#define __PARSER_LUA

// parser.c
// A C parser-combinator host with Lua bindings and correct memory management.
// Compile (example): gcc -std=c11 parser.c -o parser -llua
// (adjust -llua path/names per your platform)

#include <ctype.h>
#include <lauxlib.h>
#include <lua.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------
   ParseResult
   --------------------------- */

typedef struct {
  int ok;           // 1 success, 0 failure
  const char *rest; // pointer into original input (not owned)
  char *output;     // malloc'd owned string on success, NULL otherwise
} ParseResult;

static ParseResult parse_ok(const char *rest, char *output) {
  ParseResult r = {1, rest, output};
  return r;
}
static ParseResult parse_err(const char *input) {
  ParseResult r = {0, input, NULL};
  return r;
}

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
  P_ZERO_OR_MORE
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
                          void *data, lua_State *L) {
  Parser *p = (Parser *)malloc(sizeof(Parser));
  if (!p) {
    perror("malloc");
    exit(1);
  }
  p->kind = k;
  p->parse = parse;
  p->destroy = destroy;
  p->data = data;
  p->refcount = 1;
  p->L = L;
  return p;
}

static void parser_ref(Parser *p) {
  if (p)
    p->refcount++;
}

static void parser_unref(Parser *p); // forward

/* Forward free helper to allow recursion */
static void parser_free_internal(Parser *p);

/* Unref and free if needed */
static void parser_unref(Parser *p) {
  if (!p)
    return;
  p->refcount--;
  if (p->refcount <= 0) {
    parser_free_internal(p);
  }
}

/* ---------------------------
   Literal parser
   --------------------------- */

typedef struct {
  char *lit; // malloc'd
} LiteralData;

static ParseResult literal_parse(Parser *p, const char *input) {
  LiteralData *d = (LiteralData *)p->data;
  size_t n = strlen(d->lit);
  if (strncmp(input, d->lit, n) == 0) {
    char *out = strdup(d->lit);
    return parse_ok(input + n, out);
  }

  return parse_err(input);
}

static void literal_destroy(Parser *p) {
  LiteralData *d = (LiteralData *)p->data;
  if (d) {
    free(d->lit);
    free(d);
  }
}

static Parser *make_literal(lua_State *L, const char *s) {
  LiteralData *d = (LiteralData *)malloc(sizeof(LiteralData));
  d->lit = strdup(s);
  return parser_new(P_LITERAL, literal_parse, literal_destroy, d, L);
}

/* ---------------------------
   any_char parser
   --------------------------- */

typedef struct {
  int dummy;
} AnyCharData;

static ParseResult any_char_parse(Parser *p, const char *input) {
  (void)p;
  if (!input || input[0] == '\0')
    return parse_err(input);

  unsigned char uc = (unsigned char)input[0];
  int len = 1;
  if ((uc & 0x80) == 0)
    len = 1;
  else if ((uc & 0xE0) == 0xC0)
    len = 2;
  else if ((uc & 0xF0) == 0xE0)
    len = 3;
  else if ((uc & 0xF8) == 0xF0)
    len = 4;

  char *out = (char *)malloc(len + 1);
  memcpy(out, input, len);
  out[len] = '\0';
  return parse_ok(input + len, out);
}

static void any_char_destroy(Parser *p) { free(p->data); }
static Parser *make_any_char(lua_State *L) {
  AnyCharData *d = (AnyCharData *)malloc(sizeof(AnyCharData));
  return parser_new(P_ANY_CHAR, any_char_parse, any_char_destroy, d, L);
}

/* ---------------------------
   identifier parser
   --------------------------- */

typedef struct {
  int dummy;
} IdentData;

static ParseResult identifier_parse(Parser *p, const char *input) {
  (void)p;
  if (!input || input[0] == '\0')
    return parse_err(input);

  size_t i = 0;
  size_t len = strlen(input);
  if (i >= len)
    return parse_err(input);

  unsigned char uc = (unsigned char)input[i];
  if (!isalnum(uc))
    return parse_err(input);

  i++;
  while (i < len) {
    unsigned char c = (unsigned char)input[i];
    if (isalnum(c) || c == '-')
      i++;
    else
      break;
  }

  size_t matched = i;
  char *out = (char *)malloc(matched + 1);
  memcpy(out, input, matched);
  out[matched] = '\0';
  return parse_ok(input + matched, out);
}

static void identifier_destroy(Parser *p) { free(p->data); }

static Parser *make_identifier(lua_State *L) {
  IdentData *d = (IdentData *)malloc(sizeof(IdentData));
  return parser_new(P_IDENTIFIER, identifier_parse, identifier_destroy, d, L);
}

/* ---------------------------
   Map combinator
   data: inner Parser*, int func_ref
   --------------------------- */

typedef struct {
  Parser *inner;
  int func_ref; // registry ref to Lua function
} MapData;

static ParseResult map_parse(Parser *p, const char *input) {
  MapData *d = (MapData *)p->data;
  ParseResult r = d->inner->parse(d->inner, input);
  if (!r.ok)
    return r;

  // get function from registry and call with r.output
  lua_State *L = p->L;
  lua_rawgeti(L, LUA_REGISTRYINDEX, d->func_ref); // push function
  lua_pushstring(L, r.output);
  if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
    // error calling lua function - return parse error
    const char *err = lua_tostring(L, -1);
    fprintf(stderr, "map callback error: %s\n", err ? err : "(unknown)");
    lua_pop(L, 1);
    free(r.output);
    return parse_err(input);
  }
  // expect a string result
  const char *res = lua_tostring(L, -1);
  char *mapped = res ? strdup(res) : strdup("");
  lua_pop(L, 1);
  free(r.output);
  return parse_ok(r.rest, mapped);
}

static void map_destroy(Parser *p) {
  MapData *d = (MapData *)p->data;
  if (d) {
    // unref lua func
    if (d->func_ref != LUA_NOREF && p->L) {
      luaL_unref(p->L, LUA_REGISTRYINDEX, d->func_ref);
    }
    if (d->inner)
      parser_unref(d->inner);
    free(d);
  }
}

static Parser *make_map(lua_State *L, Parser *inner, int func_ref) {
  MapData *d = (MapData *)malloc(sizeof(MapData));
  d->inner = inner;
  parser_ref(d->inner); // take ownership
  d->func_ref = func_ref;
  return parser_new(P_MAP, map_parse, map_destroy, d, L);
}

/* ---------------------------
   and_then combinator
   data: inner Parser*, int func_ref (lua function returning parser userdata)
   --------------------------- */

typedef struct {
  Parser *inner;
  int func_ref; // lua function: (string) -> parser userdata
} AndThenData;

static ParseResult and_then_parse(Parser *p, const char *input) {
  AndThenData *d = (AndThenData *)p->data;
  ParseResult r = d->inner->parse(d->inner, input);
  if (!r.ok)
    return r;

  lua_State *L = p->L;
  // call lua function with r.output, expect it to return a Parser userdata
  lua_rawgeti(L, LUA_REGISTRYINDEX, d->func_ref);
  lua_pushstring(L, r.output);
  if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
    const char *err = lua_tostring(L, -1);
    fprintf(stderr, "and_then callback error: %s\n", err ? err : "(unknown)");
    lua_pop(L, 1);
    free(r.output);
    return parse_err(input);
  }

  // check returned value is Parser userdata
  Parser **retp = (Parser **)luaL_testudata(L, -1, "Parser");
  if (!retp) {
    // not a parser
    lua_pop(L, 1);
    free(r.output);
    return parse_err(input);
  }
  Parser *next = *retp;
  // we must ensure ownership: parser returned by Lua userdata is still owned by
  // that userdata but we can call parse on it; it must be valid while Lua holds
  // it. For safety, increment ref so we can use it and unref after:
  parser_ref(next);
  lua_pop(L, 1); // pop return value

  ParseResult r2 = next->parse(next, r.rest);
  parser_unref(next);
  free(r.output);
  return r2;
}

static void and_then_destroy(Parser *p) {
  AndThenData *d = (AndThenData *)p->data;
  if (d) {
    if (d->func_ref != LUA_NOREF && p->L) {
      luaL_unref(p->L, LUA_REGISTRYINDEX, d->func_ref);
    }
    if (d->inner)
      parser_unref(d->inner);
    free(d);
  }
}

static Parser *make_and_then(lua_State *L, Parser *inner, int func_ref) {
  AndThenData *d = (AndThenData *)malloc(sizeof(AndThenData));
  d->inner = inner;
  parser_ref(d->inner);
  d->func_ref = func_ref;
  return parser_new(P_AND_THEN, and_then_parse, and_then_destroy, d, L);
}

/* ---------------------------
   or_else combinator
   data: left Parser*, right Parser*
   --------------------------- */

typedef struct {
  Parser *left;
  Parser *right;
} OrData;

static ParseResult or_parse(Parser *p, const char *input) {
  OrData *d = (OrData *)p->data;
  ParseResult r1 = d->left->parse(d->left, input);
  if (r1.ok)
    return r1;
  return d->right->parse(d->right, input);
}
static void or_destroy(Parser *p) {
  OrData *d = (OrData *)p->data;
  if (d) {
    if (d->left)
      parser_unref(d->left);
    if (d->right)
      parser_unref(d->right);
    free(d);
  }
}
static Parser *make_or(lua_State *L, Parser *a, Parser *b) {
  OrData *d = (OrData *)malloc(sizeof(OrData));
  d->left = a;
  parser_ref(a);
  d->right = b;
  parser_ref(b);
  return parser_new(P_OR_ELSE, or_parse, or_destroy, d, L);
}

/* ---------------------------
   pred combinator
   data: inner Parser*, int func_ref (lua predicate)
   --------------------------- */

typedef struct {
  Parser *inner;
  int func_ref; // lua function: (string) -> boolean
} PredData;

static ParseResult pred_parse(Parser *p, const char *input) {
  PredData *d = (PredData *)p->data;
  ParseResult r = d->inner->parse(d->inner, input);
  if (!r.ok)
    return r;
  lua_State *L = p->L;
  lua_rawgeti(L, LUA_REGISTRYINDEX, d->func_ref);
  lua_pushstring(L, r.output);
  if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
    const char *err = lua_tostring(L, -1);
    fprintf(stderr, "pred callback error: %s\n", err ? err : "(unknown)");
    lua_pop(L, 1);
    free(r.output);
    return parse_err(input);
  }
  int truthy = lua_toboolean(L, -1);
  lua_pop(L, 1);
  if (truthy)
    return r;
  free(r.output);
  return parse_err(input);
}

static void pred_destroy(Parser *p) {
  PredData *d = (PredData *)p->data;
  if (d) {
    if (d->func_ref != LUA_NOREF && p->L)
      luaL_unref(p->L, LUA_REGISTRYINDEX, d->func_ref);
    if (d->inner)
      parser_unref(d->inner);
    free(d);
  }
}

static Parser *make_pred(lua_State *L, Parser *inner, int func_ref) {
  PredData *d = (PredData *)malloc(sizeof(PredData));
  d->inner = inner;
  parser_ref(d->inner);
  d->func_ref = func_ref;
  return parser_new(P_PRED, pred_parse, pred_destroy, d, L);
}

/* ---------------------------
   repetition combinators
   one_or_more and zero_or_more: they concatenate outputs into one malloc'd
   string
   --------------------------- */

typedef struct {
  Parser *inner;
} RepData;

static ParseResult one_or_more_parse(Parser *p, const char *input) {
  RepData *d = (RepData *)p->data;
  const char *cur = input;
  ParseResult r = d->inner->parse(d->inner, cur);
  if (!r.ok)
    return r;
  // dynamic buffer
  size_t cap = 64;
  size_t len = 0;
  char *buf = (char *)malloc(cap);
  // append first
  size_t add = strlen(r.output);
  while (len + add + 1 > cap) {
    cap *= 2;
    buf = (char *)realloc(buf, cap);
    if (!buf) {
      perror("realloc");
      exit(1);
    }
  }
  memcpy(buf + len, r.output, add);
  len += add;
  buf[len] = '\0';
  cur = r.rest;
  free(r.output);

  while (1) {
    ParseResult r2 = d->inner->parse(d->inner, cur);
    if (!r2.ok)
      break;
    add = strlen(r2.output);
    while (len + add + 1 > cap) {
      cap *= 2;
      buf = (char *)realloc(buf, cap);
      if (!buf) {
        perror("realloc");
        exit(1);
      }
    }
    memcpy(buf + len, r2.output, add);
    len += add;
    buf[len] = '\0';
    cur = r2.rest;
    free(r2.output);
  }
  return parse_ok(cur, buf);
}
static ParseResult zero_or_more_parse(Parser *p, const char *input) {
  RepData *d = (RepData *)p->data;
  const char *cur = input;
  size_t cap = 64;
  size_t len = 0;
  char *buf = (char *)malloc(cap);
  buf[0] = '\0';
  while (1) {
    ParseResult r = d->inner->parse(d->inner, cur);
    if (!r.ok)
      break;
    size_t add = strlen(r.output);
    while (len + add + 1 > cap) {
      cap *= 2;
      buf = (char *)realloc(buf, cap);
      if (!buf) {
        perror("realloc");
        exit(1);
      }
    }
    memcpy(buf + len, r.output, add);
    len += add;
    buf[len] = '\0';
    cur = r.rest;
    free(r.output);
  }
  return parse_ok(cur, buf);
}
static void rep_destroy(Parser *p) {
  RepData *d = (RepData *)p->data;
  if (d) {
    if (d->inner)
      parser_unref(d->inner);
    free(d);
  }
}
static Parser *make_one_or_more(lua_State *L, Parser *inner) {
  RepData *d = (RepData *)malloc(sizeof(RepData));
  d->inner = inner;
  parser_ref(d->inner);
  return parser_new(P_ONE_OR_MORE, one_or_more_parse, rep_destroy, d, L);
}
static Parser *make_zero_or_more(lua_State *L, Parser *inner) {
  RepData *d = (RepData *)malloc(sizeof(RepData));
  d->inner = inner;
  parser_ref(d->inner);
  return parser_new(P_ZERO_OR_MORE, zero_or_more_parse, rep_destroy, d, L);
}

/* ---------------------------
   Central free implementation
   --------------------------- */

static void parser_free_internal(Parser *p) {
  if (!p)
    return;
  switch (p->kind) {
  case P_LITERAL:
    literal_destroy(p);
    break;
  case P_ANY_CHAR:
    any_char_destroy(p);
    break;
  case P_IDENTIFIER:
    identifier_destroy(p);
    break;
  case P_MAP:
    map_destroy(p);
    break;
  case P_AND_THEN:
    and_then_destroy(p);
    break;
  case P_OR_ELSE:
    or_destroy(p);
    break;
  case P_PRED:
    pred_destroy(p);
    break;
  case P_ONE_OR_MORE:
  case P_ZERO_OR_MORE:
    rep_destroy(p);
    break;
  default:
    break;
  }
  free(p);
}

/* ---------------------------
   Lua userdata helpers
   --------------------------- */

static Parser *check_parser_ud(lua_State *L, int idx) {
  Parser **ud = (Parser **)luaL_checkudata(L, idx, "Parser");
  if (!ud)
    luaL_error(L, "expected Parser userdata");
  return *ud;
}

static void push_parser_ud(lua_State *L, Parser *p) {
  Parser **ud = (Parser **)lua_newuserdata(L, sizeof(Parser *));
  *ud = p;
  // userdata owns a ref of parser
  parser_ref(p); // increment for userdata owner
  luaL_getmetatable(L, "Parser");
  lua_setmetatable(L, -2);
}

/* ---------------------------
   Lua-exposed constructors / methods
   --------------------------- */

/* parser.literal(s) */
static int l_parser_literal(lua_State *L) {
  const char *s = luaL_checkstring(L, 1);
  Parser *p = make_literal(L, s);
  push_parser_ud(L, p);
  // unref the initial creator reference: push_parser_ud already added one
  parser_unref(p);
  return 1;
}

/* parser.any_char() */
static int l_parser_any_char(lua_State *L) {
  Parser *p = make_any_char(L);
  push_parser_ud(L, p);
  parser_unref(p);
  return 1;
}

/* parser.identifier() */
static int l_parser_identifier(lua_State *L) {
  Parser *p = make_identifier(L);
  push_parser_ud(L, p);
  parser_unref(p);
  return 1;
}

/* p:map(function) */
static int l_parser_map(lua_State *L) {
  Parser *inner = check_parser_ud(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  lua_pushvalue(L, 2);
  int ref = luaL_ref(L, LUA_REGISTRYINDEX);
  Parser *mp = make_map(L, inner, ref);
  push_parser_ud(L, mp);
  parser_unref(mp);
  return 1;
}

/* p:and_then(function) -> function must return a Parser userdata */
static int l_parser_and_then(lua_State *L) {
  Parser *inner = check_parser_ud(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  lua_pushvalue(L, 2);
  int ref = luaL_ref(L, LUA_REGISTRYINDEX);
  Parser *ap = make_and_then(L, inner, ref);
  push_parser_ud(L, ap);
  parser_unref(ap);
  return 1;
}

/* p:or_else(q) */
static int l_parser_or_else(lua_State *L) {
  Parser *a = check_parser_ud(L, 1);
  Parser *b = check_parser_ud(L, 2);
  Parser *or_p = make_or(L, a, b);
  push_parser_ud(L, or_p);
  parser_unref(or_p);
  return 1;
}

/* p:pred(function) */
static int l_parser_pred(lua_State *L) {
  Parser *inner = check_parser_ud(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  lua_pushvalue(L, 2);
  int ref = luaL_ref(L, LUA_REGISTRYINDEX);
  Parser *pp = make_pred(L, inner, ref);
  push_parser_ud(L, pp);
  parser_unref(pp);
  return 1;
}

/* p:one_or_more() */
static int l_parser_one_or_more(lua_State *L) {
  Parser *inner = check_parser_ud(L, 1);
  Parser *rp = make_one_or_more(L, inner);
  push_parser_ud(L, rp);
  parser_unref(rp);
  return 1;
}

/* p:zero_or_more() */
static int l_parser_zero_or_more(lua_State *L) {
  Parser *inner = check_parser_ud(L, 1);
  Parser *rp = make_zero_or_more(L, inner);
  push_parser_ud(L, rp);
  parser_unref(rp);
  return 1;
}

/* p:parse(input) -> returns output (string or nil) , rest (string) */
static int l_parser_parse(lua_State *L) {
  Parser *p = check_parser_ud(L, 1);
  const char *input = luaL_checkstring(L, 2);
  ParseResult r = p->parse(p, input);
  if (r.ok) {
    lua_pushstring(L, r.output ? r.output : "");
    lua_pushstring(L, r.rest ? r.rest : "");
    free(r.output);
    return 2;
  } else {
    lua_pushnil(L);
    lua_pushstring(L, r.rest ? r.rest : "");
    return 2;
  }
}

/* __gc metamethod: free the userdata-owned reference */
static int l_parser_gc(lua_State *L) {
  Parser **ud = (Parser **)luaL_checkudata(L, 1, "Parser");
  if (ud && *ud) {
    parser_unref(*ud);
    *ud = NULL;
  }
  return 0;
}

/* __tostring for debug */
static int l_parser_tostring(lua_State *L) {
  Parser *p = check_parser_ud(L, 1);
  const char *kind;
  switch (p->kind) {
  case P_LITERAL:
    kind = "literal";
    break;
  case P_ANY_CHAR:
    kind = "any_char";
    break;
  case P_IDENTIFIER:
    kind = "identifier";
    break;
  case P_MAP:
    kind = "map";
    break;
  case P_AND_THEN:
    kind = "and_then";
    break;
  case P_OR_ELSE:
    kind = "or_else";
    break;
  case P_PRED:
    kind = "pred";
    break;
  case P_ONE_OR_MORE:
    kind = "one_or_more";
    break;
  case P_ZERO_OR_MORE:
    kind = "zero_or_more";
    break;
  default:
    kind = "parser";
  }
  lua_pushfstring(L, "<Parser:%s>", kind);
  return 1;
}

/* ---------------------------
   module registration
   --------------------------- */

static const luaL_Reg parser_methods[] = {
    {"map", l_parser_map},
    {"and_then", l_parser_and_then},
    {"or_else", l_parser_or_else},
    {"pred", l_parser_pred},
    {"one_or_more", l_parser_one_or_more},
    {"zero_or_more", l_parser_zero_or_more},
    {"parse", l_parser_parse},
    {NULL, NULL}};

int luaopen_parser(lua_State *L) {
  // create Parser metatable
  luaL_newmetatable(L, "Parser");

  // set __index to methods table
  lua_newtable(L);
  luaL_setfuncs(L, parser_methods, 0);
  lua_setfield(L, -2, "__index");

  // set metamethods
  lua_pushcfunction(L, l_parser_gc);
  lua_setfield(L, -2, "__gc");

  lua_pushcfunction(L, l_parser_tostring);
  lua_setfield(L, -2, "__tostring");

  // pop metatable
  lua_pop(L, 1);

  // module table
  lua_newtable(L);
  lua_pushcfunction(L, l_parser_literal);
  lua_setfield(L, -2, "literal");
  lua_pushcfunction(L, l_parser_any_char);
  lua_setfield(L, -2, "any_char");
  lua_pushcfunction(L, l_parser_identifier);
  lua_setfield(L, -2, "identifier");

  return 1;
}
#endif
