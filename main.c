#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>

#include "parser.h"

int main() {
  // create the state, virtual machine, registery table
  lua_State *L = luaL_newstate();

  // make the standard liberary available to lua
  luaL_openlibs(L);

  // parser
  luaL_requiref(L, "parser", luaopen_parser, 1);
  lua_pop(L, 1);
  // Lua test script
  const char *script =
      "print('--- Lua parser combinators test ---')\n"
      "local parser = require('parser')\n"
      "\n"
      "-- literal parser\n"
      "local lit = parser.literal('foo')\n"
      "\n"
      "-- map example: uppercase the result\n"
      "local upper = lit:map(function(s) return string.upper(s) end)\n"
      "\n"
      "-- identifier parser\n"
      "local ident = parser.identifier()\n"
      "\n"
      "-- zero_or_more example\n"
      "local many_idents = ident:zero_or_more()\n"
      "\n"
      "-- and_then example: after matching 'foo', match 'bar'\n"
      "local combined = lit:and_then(function(_) return parser.literal('bar') "
      "end)\n"
      "\n"
      "-- one_or_more example\n"
      "local repeated = parser.literal('a'):one_or_more()\n"
      "\n"
      "-- or_else example\n"
      "local choice = parser.literal('x'):or_else(parser.literal('y'))\n"
      "\n"
      "-- pred example: only keep 'foo' if length is 3\n"
      "local pred_test = lit:pred(function(s) return #s == 3 end)\n"
      "\n"
      "-- test inputs\n"
      "local function run(p, input)\n"
      "  local out, rest = p:parse(input)\n"
      "  if out then\n"
      "    print('Parse succeeded! Output:', out, 'Remaining:', rest)\n"
      "  else\n"
      "    print('Parse failed. Remaining input:', rest)\n"
      "  end\n"
      "end\n"
      "\n"
      "print('--- map test ---')\n"
      "run(upper, 'foo baz')\n"
      "\n"
      "print('--- zero_or_more test ---')\n"
      "run(many_idents, 'abc123 xyz')\n"
      "\n"
      "print('--- and_then test ---')\n"
      "run(combined, 'foobar')\n"
      "\n"
      "print('--- one_or_more test ---')\n"
      "run(repeated, 'aaaaab')\n"
      "\n"
      "print('--- or_else test ---')\n"
      "run(choice, 'yabc')\n"
      "\n"
      "print('--- pred test ---')\n"
      "run(pred_test, 'foo bar')\n"
      "\n"
      "collectgarbage()\n"
      "print('--- done ---')\n";

  // run the script
  if (luaL_dostring(L, script) != LUA_OK) {
    fprintf(stderr, "Lua error: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
  }

  lua_close(L);
  return 0;
}
