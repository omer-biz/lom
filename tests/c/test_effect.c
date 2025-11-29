#include "effect.h"

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

  // update `package.path` to include `LUA_SRC_DIR`
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path");
  const char *current_path = lua_tostring(L, -1);

  const char *extra_path = LUA_SRC_DIR "/?.lua";

  char new_path[2048];
  snprintf(new_path, sizeof(new_path), "%s;%s", current_path, extra_path);

  lua_pop(L, 1);

  lua_pushstring(L, new_path);
  lua_setfield(L, -2, "path");
  lua_pop(L, 1);
}

void test_teardown(void) { lua_close(L); }

void run_lua(lua_State *L, const char *src) {
  if (luaL_loadstring(L, src) || lua_pcall(L, 0, 1, 0)) {
    ck_abort_msg("Lua error: %s", lua_tostring(L, -1));
  }
}

START_TEST(test_error) {
  const char *lua_src = "local effect = require 'effect'"
                        "return effect.error('Error returned from lua effect')";

  run_lua(L, lua_src);

  Effect e = parse_effect(L, -1);
  ck_assert_int_eq(e.kind, EFFECT_ERROR);
  ck_assert_str_eq(e.as.error.message, "Error returned from lua effect");
}
END_TEST

START_TEST(test_file) {
  const char *lua_src =
      "local effect = require 'effect'"
      "return effect.file('hello world', { path = 'out.txt', mode = 'a' })";

  run_lua(L, lua_src);

  Effect e = parse_effect(L, -1);
  ck_assert_int_eq(e.kind, EFFECT_FILE);
  ck_assert_str_eq(e.as.file.content, "hello world");
  ck_assert_str_eq(e.as.file.path, "out.txt");
  ck_assert_str_eq(e.as.file.mode, "a");
}
END_TEST

START_TEST(test_network) {
  const char *lua_src = "local effect = require 'effect'"
                        "return effect.network('payload', {"
                        "  url = 'https://example.com/api',"
                        "  mode = 'post',"
                        "  timeout = 5000"
                        "})";

  run_lua(L, lua_src);

  Effect e = parse_effect(L, -1);
  ck_assert_int_eq(e.kind, EFFECT_NETWORK);
  ck_assert_str_eq(e.as.network.content, "payload");
  ck_assert_str_eq(e.as.network.url, "https://example.com/api");
  ck_assert_str_eq(e.as.network.mode, "post");
  ck_assert_int_eq(e.as.network.timeout, 5000);
}
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
