#include "inspect.h"
#include "parser.h"
#include "utils.h"
#include <lauxlib.h>
#include <lua.h>
#include <stdio.h>

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

static char *inspect_unary_with_func(const char *name, int indent) {
  char *ind = make_indent(indent);
  const char *templ = "%s%s(<function>)";

  int size = snprintf(NULL, 0, templ, ind, name) + 1;
  char *buff = malloc(size);
  if (!buff) {
    free(ind);
    return NULL;
  }

  snprintf(buff, size, templ, ind, name);

  free(ind);

  return buff;
}

static char *inspect_pred(Parser *p, int indent) {
  PredData *d = (PredData *)p->data;
  return inspect_unary_with_func("pred", indent);
}

static char *inspect_map(Parser *p, int indent) {
  MapData *d = (MapData *)p->data;
  return inspect_unary_with_func("map", indent);
}

static char *inspect_and_then(Parser *p, int indent) {
  AndThenData *d = (AndThenData *)p->data;
  return inspect_unary_with_func("and_then", indent);
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

char *inspect_parser(Parser *p, int indent) {
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

      int size = snprintf(NULL, 0, "%s%s()", ind, inspect_str) + 1;
      char *buff = malloc(size);
      if (!buff) {
        free(ind);
      }

      snprintf(buff, size, "%s%s()", ind, inspect_str);
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
  }
  char *buff = strdup("<unknow>");
  return buff;
}
