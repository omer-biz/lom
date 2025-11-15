#include <lauxlib.h>
#include <lua.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

/* ---------------------------
   ParseResult
   --------------------------- */

static ParseResult parse_ok(const char *rest, int lua_ref) {
  ParseResult r = {1, rest, lua_ref};
  return r;
}

static ParseResult parse_err(const char *input) {
  ParseResult r = {0, input, LUA_NOREF};
  return r;
}

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
  p->lua_ref = LUA_NOREF;
  return p;
}

static void parser_ref(Parser *p) {
  if (p)
    p->refcount++;
}

/* Unref and free if needed */
static void parser_unref(Parser *p) {
  if (!p)
    return;

  p->refcount--;
  if (p->refcount <= 0) {
    luaL_unref(p->L, LUA_REGISTRYINDEX, p->lua_ref);
    p->lua_ref = LUA_NOREF;

    if (p->destroy) {
      p->destroy(p);
    }
    free(p);
  }
}

static ParseResult literal_parse(Parser *p, const char *input) {
  LiteralData *d = (LiteralData *)p->data;
  size_t n = strlen(d->lit);
  if (strncmp(input, d->lit, n) == 0) {

    lua_pushlstring(p->L, d->lit, n);
    int ref = luaL_ref(p->L, LUA_REGISTRYINDEX);

    return parse_ok(input + n, ref);
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

  lua_pushlstring(p->L, input, len);
  int ref = luaL_ref(p->L, LUA_REGISTRYINDEX);

  return parse_ok(input + len, ref);
}

static void any_char_destroy(Parser *p) { free(p->data); }

static Parser *make_any_char(lua_State *L) {
  AnyCharData *d = (AnyCharData *)malloc(sizeof(AnyCharData));
  return parser_new(P_ANY_CHAR, any_char_parse, any_char_destroy, d, L);
}

static ParseResult map_parse(Parser *p, const char *input) {
  MapData *d = (MapData *)p->data;
  ParseResult r = d->inner->parse(d->inner, input);
  if (!r.ok)
    return r;

  // get function from registry and call with r.output
  lua_State *L = p->L;
  lua_rawgeti(L, LUA_REGISTRYINDEX, d->func_ref); // push function

  if (r.lua_ref != LUA_NOREF)
    lua_rawgeti(L, LUA_REGISTRYINDEX, r.lua_ref);
  else
    lua_pushnil(L);

  if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
    // error calling lua function - return parse error
    const char *err = lua_tostring(L, -1);
    fprintf(stderr, "map callback error: %s\n", err ? err : "(unknown)");
    lua_pop(L, 1);
    return parse_err(input);
  }

  if (r.lua_ref != LUA_NOREF) {
    luaL_unref(L, LUA_REGISTRYINDEX, r.lua_ref);
  }

  // push the result to the registery
  int ref = luaL_ref(L, LUA_REGISTRYINDEX);

  return parse_ok(r.rest, ref);
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

static ParseResult and_then_parse(Parser *p, const char *input) {
  AndThenData *d = (AndThenData *)p->data;
  ParseResult r = d->inner->parse(d->inner, input);
  if (!r.ok)
    return r;

  lua_State *L = p->L;
  lua_rawgeti(L, LUA_REGISTRYINDEX, d->func_ref);

  if (r.lua_ref != LUA_NOREF)
    lua_rawgeti(L, LUA_REGISTRYINDEX, r.lua_ref);
  else
    lua_pushnil(L);

  if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
    const char *err = lua_tostring(L, -1);
    fprintf(stderr, "and_then callback error: %s\n", err ? err : "(unknown)");
    lua_pop(L, 1);
    return parse_err(input);
  }

  // check returned value is Parser userdata
  Parser **retp = (Parser **)luaL_testudata(L, -1, "Parser");
  if (!retp) {
    // not a parser
    lua_pop(L, 1);
    return parse_err(input);
  }

  Parser *next = *retp;
  // we must ensure ownership: parser returned by Lua userdata is still owned by
  // that userdata but we can call parse on it; it must be valid while Lua holds
  // it. For safety, increment ref so we can use it and unref after:
  parser_ref(next);
  lua_pop(L, 1); // pop return value, remove userdata pointer from the stack

  ParseResult r2 = next->parse(next, r.rest);
  parser_unref(next);

  if (r.lua_ref != LUA_NOREF)
    luaL_unref(L, LUA_REGISTRYINDEX, r.lua_ref);

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

static ParseResult pred_parse(Parser *p, const char *input) {
  PredData *d = (PredData *)p->data;
  lua_State *L = p->L;

  const char *start = input;
  ParseResult inner_r = d->inner->parse(d->inner, input);
  if (!inner_r.ok) {
    return inner_r;
  }

  lua_rawgeti(L, LUA_REGISTRYINDEX, d->func_ref);
  if (inner_r.lua_ref != LUA_NOREF) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, inner_r.lua_ref);
  } else {
    lua_pushnil(L);
  }

  if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
    const char *err = lua_tostring(L, -1);
    fprintf(stderr, "pred callback error: %s\n", err ? err : "(unknown)");
    lua_pop(L, 1);
    return parse_err(start);
  }

  int truthy = lua_toboolean(L, -1);
  lua_pop(L, 1);

  if (truthy) {
    return inner_r;
  }

  if (inner_r.lua_ref != LUA_NOREF) {
    luaL_unref(L, LUA_REGISTRYINDEX, inner_r.lua_ref);
  }

  return parse_err(start);
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

// TODO: rename to take_after
// since we are using it as a method lit1:left(lit2) doesn't make sense
// instead: lit1:take_after(lit2) makes it clear, we are taking lit1 after
// parsing lit2
static ParseResult take_after_parse(Parser *p, const char *input) {
  TakeAfterData *d = (TakeAfterData *)p->data;
  lua_State *L = p->L;

  ParseResult r1 = d->left->parse(d->left, input);
  if (!r1.ok)
    return r1;

  ParseResult r2 = d->right->parse(d->right, r1.rest);
  if (!r2.ok) {
    // if we are here, that means r1 has succeeded which means it allocated a
    // memory for it's result which have to free if r2 fails.
    if (r1.lua_ref != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, r1.lua_ref);

    return r2;
  }

  if (r2.lua_ref != LUA_NOREF) {
    luaL_unref(L, LUA_REGISTRYINDEX, r2.lua_ref);
  }

  return parse_ok(r2.rest, r1.lua_ref);
}

static void take_after_destroy(Parser *p) {
  TakeAfterData *d = (TakeAfterData *)p->data;

  if (d) {
    if (d->left)
      parser_unref(d->left);
    if (d->right)
      parser_unref(d->right);

    free(d);
  }
}

static Parser *make_take_after(lua_State *L, Parser *left, Parser *right) {
  TakeAfterData *d = (TakeAfterData *)malloc(sizeof(TakeAfterData));

  d->left = left;
  parser_ref(left);

  d->right = right;
  parser_ref(right);

  return parser_new(P_TAKE_AFTER, take_after_parse, take_after_destroy, d, L);
}

// TODO: rename to drop_for
// since we are using it as a method lit1:right(lit2) doesn't make sense
// instead: lit1:drop_for(lit2) makes it clear, we are droping lit1 for lit2
// after parsing lit1
static ParseResult drop_for_parse(Parser *p, const char *input) {
  DropForData *d = (DropForData *)p->data;
  lua_State *L = p->L;

  ParseResult r1 = d->left->parse(d->left, input);
  if (!r1.ok)
    return r1;

  ParseResult r2 = d->right->parse(d->right, r1.rest);
  if (!r2.ok) {
    if (r1.lua_ref != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, r1.lua_ref);

    return r2;
  }

  if (r1.lua_ref != LUA_NOREF)
    luaL_unref(L, LUA_REGISTRYINDEX, r1.lua_ref);

  return parse_ok(r2.rest, r2.lua_ref);
}

static void drop_for_destroy(Parser *p) {
  DropForData *d = (DropForData *)p->data;

  if (d) {
    if (d->left)
      parser_unref(d->left);
    if (d->right)
      parser_unref(d->right);

    free(d);
  }
}

static Parser *make_drop_for(lua_State *L, Parser *left, Parser *right) {
  DropForData *d = (DropForData *)malloc(sizeof(DropForData));

  d->left = left;
  parser_ref(left);

  d->right = right;
  parser_ref(right);

  return parser_new(P_DROP_FOR, drop_for_parse, drop_for_destroy, d, L);
}

static ParseResult one_or_more_parse(Parser *p, const char *input) {
  RepData *d = (RepData *)p->data;
  lua_State *L = p->L;
  const char *cur = input;

  // First parse (must succeed)
  ParseResult r = d->inner->parse(d->inner, cur);
  if (!r.ok)
    return r;

  lua_newtable(L);
  int count = 0;

  do {
    count++;
    if (r.lua_ref != LUA_NOREF) {
      lua_rawgeti(L, LUA_REGISTRYINDEX, r.lua_ref);
      luaL_unref(L, LUA_REGISTRYINDEX, r.lua_ref);
    } else {
      lua_pushnil(L);
    }
    lua_rawseti(L, -2, count);
    cur = r.rest;

    r = d->inner->parse(d->inner, cur); // RE-PARSE HERE
  } while (r.ok);

  int ref = luaL_ref(L, LUA_REGISTRYINDEX);
  return parse_ok(cur, ref);
}

static ParseResult zero_or_more_parse(Parser *p, const char *input) {
  RepData *d = (RepData *)p->data;
  lua_State *L = p->L;
  const char *cur = input;

  lua_newtable(L);
  int count = 0;

  while (1) {
    ParseResult r = d->inner->parse(d->inner, cur);
    if (!r.ok)
      break;

    count++;
    if (r.lua_ref != LUA_NOREF) {
      lua_rawgeti(L, LUA_REGISTRYINDEX, r.lua_ref);
      luaL_unref(L, LUA_REGISTRYINDEX, r.lua_ref);
    } else {
      lua_pushnil(L);
    }
    lua_rawseti(L, -2, count);

    cur = r.rest;
  }

  int ref = luaL_ref(L, LUA_REGISTRYINDEX);
  return parse_ok(cur, ref);
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

static ParseResult pair_parse(Parser *p, const char *input) {
  PairData *d = (PairData *)p->data;
  lua_State *L = p->L;

  ParseResult r_left = d->left->parse(d->left, input);
  if (!r_left.ok) {
    return r_left;
  }

  ParseResult r_right = d->right->parse(d->right, r_left.rest);

  if (!r_right.ok) {
    if (r_left.lua_ref != LUA_NOREF) {
      luaL_unref(L, LUA_REGISTRYINDEX, r_left.lua_ref);
    }

    return r_right;
  }

  lua_newtable(L);

  if (r_left.lua_ref != LUA_NOREF) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, r_left.lua_ref);
    luaL_unref(L, LUA_REGISTRYINDEX, r_left.lua_ref);
  } else {
    lua_pushnil(L);
  }
  lua_rawseti(L, -2, 1); // left is first element

  if (r_right.lua_ref != LUA_NOREF) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, r_right.lua_ref);
    luaL_unref(L, LUA_REGISTRYINDEX, r_right.lua_ref);
  } else {
    lua_pushnil(L);
  }
  lua_rawseti(L, -2, 2); // right is first element

  int ref = luaL_ref(L, LUA_REGISTRYINDEX);
  return parse_ok(r_right.rest, ref);
}

static void pair_destroy(Parser *p) {
  PairData *d = (PairData *)p->data;
  if (d) {
    if (d->left)
      parser_unref(d->left);
    if (d->right)
      parser_unref(d->right);
    free(d);
  }
}

static Parser *make_pair(lua_State *L, Parser *left, Parser *right) {
  PairData *d = (PairData *)malloc(sizeof(PairData));

  d->left = left;
  parser_ref(left);

  d->right = right;
  parser_ref(right);

  return parser_new(P_PAIR, pair_parse, pair_destroy, d, L);
}

static ParseResult lazy_parse(Parser *p, const char *input) {
  LazyData *d = (LazyData *)p->data;
  lua_State *L = p->L;

  lua_rawgeti(L, LUA_REGISTRYINDEX, d->func_ref);

  if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
    const char *err = lua_tostring(L, -1);
    fprintf(stderr, "lazy parser thunk error: %s\n", err ? err : "(unknown)");
    lua_pop(L, 1);

    return parse_err(input);
  }

  Parser **pp = (Parser **)luaL_testudata(L, -1, "Parser");

  if (!pp) {
    lua_pop(L, 1);
    return parse_err(input);
  }

  Parser *inner = *pp;
  parser_ref(inner);

  lua_pop(L, 1);

  ParseResult r = inner->parse(inner, input);

  parser_unref(inner);
  return r;
}

static void lazy_destroy(Parser *p) {
  LazyData *d = (LazyData *)p->data;
  if (d) {
    if (d->func_ref != LUA_NOREF) {
      luaL_unref(p->L, LUA_REGISTRYINDEX, d->func_ref);
    }
    free(d);
  }
}

static Parser *make_lazy(lua_State *L, int func_ref) {
  LazyData *d = (LazyData *)malloc(sizeof(LazyData));
  d->func_ref = func_ref;

  return parser_new(P_LAZY, lazy_parse, lazy_destroy, d, L);
}

static ParseResult custom_parse(Parser *p, const char *input) {
  CustomData *d = (CustomData *)p->data;
  lua_State *L = p->L;

  lua_rawgeti(L, LUA_REGISTRYINDEX, d->func_ref);
  lua_pushstring(L, input);
  if (lua_pcall(L, 1, 2, 0) != LUA_OK) {
    const char *err = lua_tostring(L, -1);
    fprintf(stderr, "unable to create a new parser: %s\n",
            err ? err : "(unknown)");
    lua_pop(L, 1);

    return parse_err(input);
  }

  const char *out = lua_tostring(L, -2);
  const char *rest = lua_tostring(L, -1);
  lua_pop(L, 1);

  int ref = luaL_ref(L, LUA_REGISTRYINDEX);

  return parse_ok(rest, ref);
}

static Parser *make_custom(lua_State *L, int func_ref) {
  CustomData *d = (CustomData *)malloc(sizeof(CustomData));
  d->func_ref = func_ref;

  return parser_new(P_CUSTOM, custom_parse, custom_destroy, d, L);
}

static void custom_destroy(Parser *p) {
  CustomData *d = (CustomData *)p->data;

  if (d) {
    if (d->func_ref != LUA_NOREF) {
      luaL_unref(p->L, LUA_REGISTRYINDEX, d->func_ref);
    }
    free(d);
  }
}

/* inspector */

static char *make_indent(int level) {
  int n = level * 4;

  if (n < 0)
    n = 0;

  if (n > 63)
    n = 63;

  char *idnt = malloc(n + 1);
  memset(idnt, ' ', n);
  idnt[n] = '\0';
  return idnt;
}

static char *inspect_literal(Parser *p, int indent) {
  LiteralData *d = (LiteralData *)p->data;
  char *ind = make_indent(indent);
  const char *templ = "%sliteral(\"%s\")";

  int size = snprintf(NULL, 0, templ, ind, d->lit) + 1;
  char *buff = malloc(size);

  if (buff == NULL) {
    free(ind);
    return NULL;
  }

  snprintf(buff, size, templ, ind, d->lit);

  return buff;
}

static char *inspect_any_char(Parser *p, int indent) {
  (void)p;
  char *ind = make_indent(indent);
  int size = snprintf(NULL, 0, "%sany_char()", ind);
  char *buff = malloc(size);

  if (buff == NULL) {
    free(ind);
    return NULL;
  }

  return buff;
}

static char *inspect_binary(const char *name, Parser *left, Parser *right,
                            int indent) {
  char *ind = make_indent(indent);
  char *left_str = inspect_parser(left, indent + 1);
  char *right_str = inspect_parser(right, indent + 1);
  const char *templ = "%s%s(\n%s\n%s\n%s)";

  int size = snprintf(NULL, 0, templ, ind, name, left_str, right_str, ind) + 1;

  char *buff = malloc(size);
  snprintf(buff, size, templ, ind, name, left_str, right_str, ind);

  free(ind);
  free(left_str);
  free(right_str);

  return buff;
}

static char *inspect_pair(Parser *p, int indent) {
  PairData *d = (PairData *)p->data;
  return inspect_binary("pair", d->left, d->right, indent);
}

static char *inspect_take_after(Parser *p, int indent) {
  TakeAfterData *d = (TakeAfterData *)p->data;
  return inspect_binary("take_after", d->left, d->right, indent);
}

static char *inspect_drop_for(Parser *p, int indent) {
  DropForData *d = (DropForData *)p->data;
  return inspect_binary("drop_for", d->left, d->right, indent);
}

static char *inspect_or_else(Parser *p, int indent) {
  OrData *d = (OrData *)p->data;
  return inspect_binary("or_else", d->left, d->right, indent);
}

static char *inspect_one_or_more(Parser *p, int indent) {
  RepData *d = (RepData *)p->data;

  char *ind = make_indent(indent);
  char *inner = inspect_parser(d->inner, indent + 1);
  const char *templ = "%sone_or_more(\n%s\n%s)";

  int size = snprintf(NULL, 0, templ, ind, inner, ind) + 1;

  char *buff = malloc(size);
  if (!buff) {
    free(ind);
    free(inner);
    return NULL;
  }
  snprintf(buff, size, templ, ind, inner, ind);

  free(ind);
  free(inner);

  return buff;
}

static char *inspect_zero_or_more(Parser *p, int indent) {
  RepData *d = (RepData *)p->data;

  char *ind = make_indent(indent);
  char *inner = inspect_parser(d->inner, indent + 1);
  const char *templ = "%szero_or_more(\n%s\n%s)";

  int size = snprintf(NULL, 0, templ, ind, inner, ind) + 1;

  char *buff = malloc(size);
  if (!buff) {
    free(ind);
    free(inner);
    return NULL;
  }

  snprintf(buff, size, templ, ind, inner, ind);

  free(ind);
  free(inner);

  return buff;
}

static char *inspect_unary_with_func(const char *name, Parser *inner,
                                     int indent) {
  char *ind = make_indent(indent);
  const char *templ = "%s%s(<function>\n%s%s\n)";
  char *inner_str = inspect_parser(inner, indent + 1);

  int size = snprintf(NULL, 0, templ, ind, name, ind, inner_str) + 1;
  char *buff = malloc(size);
  if (!buff) {
    free(ind);
    free(inner_str);
    return NULL;
  }

  snprintf(buff, size, templ, ind, name, ind, inner_str);

  free(ind);
  free(inner_str);

  return buff;
  return strdup("mappp");
}

static char *inspect_pred(Parser *p, int indent) {
  PredData *d = (PredData *)p->data;
  return inspect_unary_with_func("pred", d->inner, indent);
}

static char *inspect_map(Parser *p, int indent) {
  MapData *d = (MapData *)p->data;
  return inspect_unary_with_func("map", d->inner, indent);
}

static char *inspect_and_then(Parser *p, int indent) {
  AndThenData *d = (AndThenData *)p->data;
  return inspect_unary_with_func("and_then", d->inner, indent);
}

static char *inspect_lazy(Parser *p, int indent) {
  LazyData *d = (LazyData *)p->data;

  char *ind = make_indent(indent);
  const char *templ = "%slazy(<function>)\n";

  int size = snprintf(NULL, 0, templ, ind) + 1;

  char *buff = malloc(size);
  if (!buff) {
    free(ind);
    return NULL;
  }

  snprintf(buff, size, templ, ind);
  free(ind);

  return buff;
}

static char *inspect_parser(Parser *p, int indent) {
  // TODO: could crash if recursive combinators are used
  // we don't detect cycles yet.

  lua_State *L = p->L;
  if (L && p->lua_ref != LUA_NOREF) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, p->lua_ref);

    lua_getuservalue(L, -1);
    lua_getfield(L, -1, "inspect");
    if (!lua_isnil(L, -1)) {
      const char *inspect_str = lua_tostring(L, -1);
      char *ind = make_indent(indent);

      int size = snprintf(NULL, 0, "%s%s", ind, inspect_str) + 1;
      char *buff = malloc(size);
      if (!buff) {
        free(ind);
      }

      snprintf(buff, size, "%s%s", ind, inspect_str);
      free(ind);

      lua_pop(L, 3); // inspect, uservalue, userdata
      return buff;
    }
    lua_pop(L, 3);
  }

  switch (p->kind) {
  case P_LITERAL:
    return inspect_literal(p, indent);
  case P_ANY_CHAR:
    return inspect_any_char(p, indent);

  case P_PAIR:
    return inspect_pair(p, indent);
  case P_OR_ELSE:
    return inspect_or_else(p, indent);
  case P_TAKE_AFTER:
    return inspect_take_after(p, indent);
  case P_DROP_FOR:
    return inspect_drop_for(p, indent);

  case P_MAP:
    return inspect_map(p, indent);
  case P_AND_THEN:
    return inspect_and_then(p, indent);
  case P_PRED:
    return inspect_pred(p, indent);

  case P_ONE_OR_MORE:
    return inspect_one_or_more(p, indent);
  case P_ZERO_OR_MORE:
    return inspect_zero_or_more(p, indent);

  case P_LAZY:
    return inspect_lazy(p, indent);
  default:
    return strdup("unknow");
  }
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

  // for custom lua user values
  lua_newtable(L);
  lua_setuservalue(L, -2);

  lua_pushvalue(L, -1);
  p->lua_ref = luaL_ref(L, LUA_REGISTRYINDEX);
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

/* p:parse(input) -> returns output (string or table or nil) , rest (string) */
static int l_parser_parse(lua_State *L) {
  Parser *p = check_parser_ud(L, 1);
  const char *input = luaL_checkstring(L, 2);
  ParseResult r = p->parse(p, input);
  if (r.ok) {

    if (r.lua_ref != LUA_NOREF)
      lua_rawgeti(L, LUA_REGISTRYINDEX, r.lua_ref);
    else
      lua_pushnil(L);

    lua_pushstring(L, r.rest ? r.rest : "");
    return 2;
  } else {
    lua_pushnil(L);
    lua_pushstring(L, r.rest ? r.rest : "");
    return 2;
  }
}

static int l_parser_take_after(lua_State *L) {
  Parser *left = check_parser_ud(L, 1);
  Parser *right = check_parser_ud(L, 2);

  Parser *p = make_take_after(L, left, right);
  push_parser_ud(L, p);
  parser_unref(p);

  return 1;
}

static int l_parser_drop_for(lua_State *L) {
  Parser *left = check_parser_ud(L, 1);
  Parser *right = check_parser_ud(L, 2);

  Parser *p = make_drop_for(L, left, right);
  push_parser_ud(L, p);
  parser_unref(p);

  return 1;
}

static int l_parser_pair(lua_State *L) {
  Parser *left = check_parser_ud(L, 1);
  Parser *right = check_parser_ud(L, 2);

  Parser *p = make_pair(L, left, right);
  push_parser_ud(L, p);
  parser_unref(p);

  return 1;
}

static int l_parser_lazy(lua_State *L) {
  luaL_checktype(L, 1, LUA_TFUNCTION);

  lua_pushvalue(L, 1);
  int func_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  Parser *p = make_lazy(L, func_ref);

  push_parser_ud(L, p);

  return 1;
}

static int l_parser_inspect(lua_State *L) {
  Parser *inner = check_parser_ud(L, 1);
  int ident = luaL_checkinteger(L, 2);

  const char *inspected = inspect_parser(inner, ident);
  if (inspected != NULL) {
    lua_pushstring(L, inspected);
    free((void *)inspected);
  } else {
    lua_pushnil(L);
  }

  return 1;
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
  case P_TAKE_AFTER:
    kind = "take_after";
    break;
  case P_DROP_FOR:
    kind = "drop_for";
    break;
  case P_PAIR:
    kind = "pair";
    break;

  case P_LAZY:
    kind = "lazy";
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
    {"take_after", l_parser_take_after},
    {"drop_for", l_parser_drop_for},
    {"pair", l_parser_pair},
    {"parse", l_parser_parse},
    {NULL, NULL}};

static int parser_index(lua_State *L) {
  lua_getuservalue(L, 1); // uservalue table
  lua_pushvalue(L, 2);    // key
  lua_gettable(L, -2);

  if (!lua_isnil(L, -1)) {
    return 1;
  }

  lua_pop(L, 2);

  // fallback to C methods
  luaL_getmetatable(L, "Parser");
  lua_getfield(L, -1, "__methods");
  lua_pushvalue(L, 2);
  lua_rawget(L, -2);

  return 1;
}

static int parser_newindex(lua_State *L) {
  lua_getuservalue(L, 1); // parser
  lua_pushvalue(L, 2);    // key
  lua_pushvalue(L, 3);    // value
  lua_settable(L, -3);
  return 0;
}

static int l_parser_custom(lua_State *L) {
  luaL_checktype(L, 1, LUA_TFUNCTION);

  lua_pushvalue(L, 1);
  int func_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  Parser *p = make_custom(L, func_ref);
  push_parser_ud(L, p);

  return 1;
}

int luaopen_parser(lua_State *L) {
  // create Parser metatable
  luaL_newmetatable(L, "Parser");

  // native C parsing functions as methods
  lua_newtable(L);
  luaL_setfuncs(L, parser_methods, 0);
  lua_setfield(L, -2, "__methods");

  // set __index to methods, and uservalues table
  lua_pushcfunction(L, parser_index);
  lua_setfield(L, -2, "__index");

  // set uservalues table
  lua_pushcfunction(L, parser_newindex);
  lua_setfield(L, -2, "__newindex");

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
  lua_pushcfunction(L, l_parser_lazy);
  lua_setfield(L, -2, "lazy");
  lua_pushcfunction(L, l_parser_inspect);
  lua_setfield(L, -2, "inspect");
  lua_pushcfunction(L, l_parser_custom);
  lua_setfield(L, -2, "new");

  return 1;
}
