#include <check.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "../src/parser.h"

START_TEST(test_match_literal) {
  lua_State *L = luaL_newstate();

  luaL_openlibs(L);
  luaopen_parser(L);

  luaL_requiref(L, "parser", luaopen_parser, 1);
  lua_pop(L, 1);

  const char *lua_code = "local P = require('parser')\n"
                         "local p = P.literal('abc')\n"
                         "local out, rest = p:parse('abcdef')\n"
                         "print('rest', rest)\n"
                         "print('out',  out)\n"
                         "return rest == 'def' and out == 'abc'";

  if (luaL_dostring(L, lua_code) != LUA_OK) {
    ck_abort_msg("Lua error: %s", lua_tostring(L, -1));
  }

  ck_assert(lua_toboolean(L, -1));
  lua_close(L);
}
END_TEST

Suite *parser_suite(void) {
  Suite *s = suite_create("Parser");
  TCase *tc = tcase_create("Core");
  tcase_add_test(tc, test_match_literal);
  suite_add_tcase(s, tc);
  return s;
}

int main(void) {
  int number_failed;
  Suite *s = parser_suite();
  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? 0 : 1;
}
