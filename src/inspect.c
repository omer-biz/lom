#include "inspect.h"
#include "parser.h"
#include "utils.h"
#include <stdio.h>

static char *inspect_binary(const char *name, Parser *left, Parser *right,
                            int indent) {
  char *ind = make_indent(indent);
  char *left_str = inspect_parser(left, indent + 1);
  char *right_str = inspect_parser(right, indent + 1);

  int size = snprintf(NULL, 0, "%s%s(\n%s\n%s\n%s)\n", ind, name, left_str,
                      right_str, ind) +
             1;

  char *buff = malloc(size);
  snprintf(buff, size, "%s%s(\n%s\n%s\n%s)\n", ind, name, left_str, right_str,
           ind);

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

  int size =
      snprintf(NULL, 0, "%sone_or_more(\n%s\n%s)\n", ind, inner, ind) + 1;

  char *buff = malloc(size);
  if (!buff) {
  free(ind);
  free(inner);
    return NULL;

  }
  snprintf(buff, size, "%sone_or_more(\n%s\n%s)\n", ind, inner, ind);

  free(ind);
  free(inner);

  return buff;
}

static char *inspect_zero_or_more(Parser *p, int indent) {
  RepData *d = (RepData *)p->data;

  char *ind = make_indent(indent);
  char *inner = inspect_parser(d->inner, indent + 1);

  int size =
      snprintf(NULL, 0, "%szero_or_more(\n%s\n%s)\n", ind, inner, ind) + 1;

  char *buff = malloc(size);
  if (!buff) {
    free(ind);
    free(inner);
    return NULL;
  }

  snprintf(buff, size, "%szero_or_more(\n%s\n%s)\n", ind, inner, ind);

  free(ind);
  free(inner);

  return buff;
}

static char *inspect_unary_with_func(const char *name, Parser *inner,
                                     int func_ref, int indent) {
  lua_State *L = inner->L; // it's all the same, I think. Might be different if
                           // we are going to use threads
  char *ind = make_indent(indent);
  char *inner_str = inspect_parser(inner, indent + 1);
  char *func_desc = describe_lua_function(L, func_ref);

  int size = snprintf(NULL, 0, "%s%s(%s\n%s%s)\n", ind, name, func_desc,
                      inner_str, ind) +
             1;
  char *buff = malloc(size);
  if (!buff) {
    free(ind);
    free(inner_str);
    free(func_desc);
    return NULL;
  }

  snprintf(buff, size, "%s%s(%s\n%s%s)\n", ind, name, func_desc, inner_str,
           ind);

  free(ind);
  free(inner_str);
  free(func_desc);

  return buff;
}

static char *inspect_pred(Parser *p, int indent) {
  PredData *d = (PredData *)p->data;
  return inspect_unary_with_func("red", d->inner, d->func_ref, indent);
}

static char *inspect_map(Parser *p, int indent) {
  MapData *d = (MapData *)p->data;
  return inspect_unary_with_func("map", d->inner, d->func_ref, indent);
}

static char *inspect_and_then(Parser *p, int indent) {
  AndThenData *d = (AndThenData *)p->data;
  return inspect_unary_with_func("and_then", d->inner, d->func_ref, indent);
}

static char *inspect_parser(Parser *p, int indent) { return ""; }
