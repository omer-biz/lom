#include "../../src/effect.h"

#include <check.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>

#ifndef LUA_SRC_DIR
#define LUA_SRC_DIR ""
#endif

static lua_State *L;

void test_setup(void) {
  L = luaL_newstate();

  luaL_openlibs(L);
}

void test_teardown(void) { lua_close(L); }

START_TEST(test_error) {
}
END_TEST

START_TEST(test_file) {}
END_TEST

START_TEST(test_network) {}
END_TEST

Suite *parser_suite(void) {
  Suite *s = suite_create("Parser");
  TCase *tc = tcase_create("Core");

  tcase_add_checked_fixture(tc, test_setup, test_teardown);

  tcase_add_test(tc, test_error);
  tcase_add_test(tc, test_file);
  tcase_add_test(tc, test_network);

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
