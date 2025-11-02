#include <check.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>

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

static void run_lua_test(const char *filename) {
  char fullpath[256];
  snprintf(fullpath, sizeof(fullpath), "./lua_scripts/%s", filename);

  if (luaL_dofile(L, fullpath) != LUA_OK) {
    ck_abort_msg("Lua error: %s", lua_tostring(L, -1));
  }

  ck_assert(lua_toboolean(L, -1));
}

START_TEST(test_literal) { run_lua_test("test_literal.lua"); }
END_TEST

START_TEST(test_identifier) { run_lua_test("test_identifier.lua"); }
END_TEST

START_TEST(test_take_after) { run_lua_test("./test_take_after.lua"); }
END_TEST

START_TEST(test_pair) { run_lua_test("./test_pair.lua"); }
END_TEST

START_TEST(test_one_or_more) { run_lua_test("./test_one_or_more.lua"); }
END_TEST

START_TEST(test_zero_or_more) { run_lua_test("./test_zero_or_more.lua"); }
END_TEST

START_TEST(test_pred) { run_lua_test("./test_pred.lua"); }
END_TEST

START_TEST(test_whitepace) { run_lua_test("./test_whitespace.lua"); }
END_TEST

START_TEST(test_simple_xml) {
  lua_State *L = luaL_newstate();

  luaL_openlibs(L);
  luaopen_parser(L);

  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path");

  luaL_requiref(L, "parser", luaopen_parser, 1);
  lua_pop(L, 1);

  if (luaL_dofile(L, "./example/simple_xml.lua") != LUA_OK) {
    ck_abort_msg("Lua error: %s", lua_tostring(L, -1));
  }

  ck_assert(lua_toboolean(L, -1));

  lua_close(L);
}
END_TEST

Suite *parser_suite(void) {
  Suite *s = suite_create("Parser");
  TCase *tc = tcase_create("Core");

  tcase_add_checked_fixture(tc, test_setup, test_teardown);

  tcase_add_test(tc, test_literal);
  tcase_add_test(tc, test_identifier);
  tcase_add_test(tc, test_take_after);
  tcase_add_test(tc, test_pair);
  tcase_add_test(tc, test_one_or_more);
  tcase_add_test(tc, test_zero_or_more);
  tcase_add_test(tc, test_pred);
  tcase_add_test(tc, test_whitepace);
  tcase_add_test(tc, test_simple_xml);

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
