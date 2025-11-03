#include <stdlib.h>
#include <string.h>

#ifndef __LOM_UTILS
#define __LOM_UTILS

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

#endif
