#include "inspect.h"
#include "parser.h"
#include "utils.h"

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
    OrData *d = (OrData *) p->data;
    return inspect_binary("or_else", d->left, d->right, indent);
}

static char *inspect_parser(Parser *p, int indent) { return ""; }
