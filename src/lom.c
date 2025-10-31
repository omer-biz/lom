#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>

#include "lom.h"
#include "parser.h"

static lua_State *G_L = NULL;

int lom_init(void) {
  if (G_L)
    return 0;

  lua_State *L = luaL_newstate();
  if (!L) {
    return 0;
  }

  luaL_openlibs(L);

  luaopen_parser(L);

  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path");

  luaL_requiref(L, "parser", luaopen_parser, 1);
  lua_pop(L, 1);

  G_L = L;

  return 1;
}

int lom_run(const char *script) {
  if (!G_L) {
    fprintf(stderr, "[lom] not initialized");
    return 0;
  }

  if (luaL_dostring(G_L, script) != LUA_OK) {
    fprintf(stderr, "Lua error: %s\n", lua_tostring(G_L, -1));
    lua_pop(G_L, 1);
    return 0;
  }

  return 1;
}

void lom_close(void) {
  if (G_L) {
    lua_close(G_L);
    G_L = NULL;
  }
}
