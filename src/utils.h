#include <stdlib.h>
#include <string.h>

#include <lauxlib.h>
#include <lua.h>
#include <stdio.h>
#include <stdlib.h>

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


static char *describe_lua_function(lua_State *L, int func_ref) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);

  lua_Debug ar;
  const char *name = "<anonymous>";
  int line = 0;

  if (lua_getinfo(L, ">nS", &ar)) {
    if (ar.name)
      name = ar.name;
    line = ar.linedefined;
  }

  int size = snprintf(NULL, 0, "%s@%s:%d", name, ar.short_src, line) + 1;
  char *buff = malloc(size);
  if (buff)
    snprintf(buff, size, "%s@%s:%d", name, ar.short_src, line);

  lua_pop(L, 1);
  return buff;
}

#endif
