#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>

int main() {
  // create the state, virtual machine, registery table
  lua_State *L = luaL_newstate();

  // make the standard liberary available to lua
  luaL_openlibs(L);

  // example running lua code
  char *code = "print('Hello, Cruel World')";

  // load the string
  if (luaL_loadstring(L, code) == LUA_OK) {
    // use lua_pcall to execute the string
    if (lua_pcall(L, 0, 0, 0) == LUA_OK) {
      // if successful remove the code from the stack
      lua_pop(L, lua_gettop(L));
    }
  }

  lua_close(L);
  return 0;
}
