#include "parser.h"

#ifndef __LOM_INSPECT
#define __LOM_INSPECT

static char *inspect_literal(Parser *p, int indent);
static char *inspect_any_char(Parser *p, int indent);

static char *inspect_binary(const char *name, Parser *left, Parser *right,
                            int indent);

static char *inspect_pair(Parser *p, int indent);
static char *inspect_take_after(Parser *p, int indent);
static char *inspect_drop_for(Parser *p, int indent);
static char *inspect_or_else(Parser *p, int indnet);

static char *inspect_one_or_more(Parser *p, int indent);
static char *inspect_zero_or_more(Parser *p, int indent);

static char *inspect_unary_with_func(const char *name, int indent);
static char *inspect_pred(Parser *p, int indent);
static char *inspect_map(Parser *p, int indent);
static char *inspect_and_then(Parser *p, int indent);

static char *inspect_lazy(Parser *p, int indent);

char *inspect_parser(Parser *p, int ident);

#endif
