#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>

int multiplication(lua_State *L) {
  // check first argument to be an integer and return the value
  int a = luaL_checkinteger(L, 1);

  // check second argument to be an integer and return the value
  int b = luaL_checkinteger(L, 2);

  lua_Integer c = a * b;

  // to return a value first we must push the value onto the stack
  lua_pushinteger(L, c);

  // then we return the number of values we returned
  return 1;
}

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

  // add varialbe to lua land
  lua_pushinteger(L, 42);
  lua_setglobal(L, "answer");

  code = "print(answer)";

  if (luaL_dostring(L, code) == LUA_OK) {
    lua_pop(L, lua_gettop(L));
  }

  // add a function to lua
  // push the pointer to the function to lua
  /* lua_pushcfunction(L, multipleication); */

  // get the value on top of the stack(the function pointer)
  // and set it to "mul"
  /* lua_setglobal(L, "mul"); */

  // to simplifiy this furthur the "lua_register" macro can be sued
  lua_register(L, "mul", multiplication);

  // use the function in lua
  code = "print(mul(8, 8))";

  int ret_status = luaL_dostring(L, code);
  printf("status: %d\n", ret_status);

  if (ret_status == LUA_OK) {
    lua_pop(L, lua_gettop(L));
  }

  // functions with namespaces
  // namespaces are tables in lua
  const struct luaL_Reg MyMathLib[] = {{"mul", multiplication}, {NULL, NULL}};

  // crate a table on top of the stack
  lua_newtable(L);

  // set the 'MyMathLib' to the table on top of the stack
  luaL_setfuncs(L, MyMathLib, 0);

  // set the table on top to the global variable "MyMath"
  lua_setglobal(L, "MyMath");

  code = "print(MyMath.mul(7, 8))";

  ret_status = luaL_dostring(L, code);
  printf("status: %d\n", ret_status);

  if (ret_status == LUA_OK) {
    lua_pop(L, lua_gettop(L));
  }

  // running lua scripts
  if (luaL_dofile(L, "script.lua") == LUA_OK) {
    lua_pop(L, lua_gettop(L));
  }

  // getting variables from lua
  if (luaL_dofile(L, "script2.lua") == LUA_OK) {
    lua_pop(L, lua_gettop(L));
  }

  lua_getglobal(L, "message");

  if (lua_isstring(L, -1)) {
    const char *message = lua_tostring(L, -1);
    lua_pop(L, 1);

    printf("Message from lua: %s\n", message);
  }

  // calling lua function
  if (luaL_dofile(L, "script3.lua") == LUA_OK) {
    lua_pop(L, lua_gettop(L));
  }

  lua_getglobal(L, "my_function");
  if (lua_isfunction(L, -1)) {
    if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
      lua_pop(L, lua_gettop(L));
    }
  }

  lua_close(L);
  return 0;
}
