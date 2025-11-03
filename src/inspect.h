#include "parser.h"

#ifndef __LOM_INSPECT
#define __LOM_INSPECT

static char *inspect_binary(const char *name, Parser *left, Parser *right,
                            int indent);
static char *inspect_pair(Parser *p, int indent);
static char *inspect_parser(Parser *p, int ident);

#endif
