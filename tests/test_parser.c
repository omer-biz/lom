#include <check.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "../src/parser.h"

static lua_State *L;

void test_setup(void) {
  L = luaL_newstate();

  luaL_openlibs(L);
  luaopen_parser(L);

  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path");

  luaL_requiref(L, "parser", luaopen_parser, 1);
  lua_pop(L, 1);
}

void test_teardown(void) { lua_close(L); }

START_TEST(test_literal) {

  if (luaL_dofile(L, "./lua_scripts/test_literal.lua") != LUA_OK) {
    ck_abort_msg("Lua error: %s", lua_tostring(L, -1));
  }

  ck_assert(lua_toboolean(L, -1));
}
END_TEST

START_TEST(test_identifier) {
  if (luaL_dofile(L, "./lua_scripts/test_identifier.lua") != LUA_OK) {
    ck_abort_msg("Lua error: %s", lua_tostring(L, -1));
  }

  ck_assert(lua_toboolean(L, -1));
}
END_TEST

Suite *parser_suite(void) {
  Suite *s = suite_create("Parser");
  TCase *tc = tcase_create("Core");

  tcase_add_checked_fixture(tc, test_setup, test_teardown);

  tcase_add_test(tc, test_literal);
  tcase_add_test(tc, test_identifier);

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
