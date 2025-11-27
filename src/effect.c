#include "effect.h"
#include <lauxlib.h>
#include <lua.h>
#include <string.h>

static const char *get_field_string(lua_State *L, int index,
                                    const char *field) {
  lua_getfield(L, index, field);
  const char *s = NULL;
  if (lua_isstring(L, -1)) {
    s = lua_tostring(L, -1);
  }
  lua_pop(L, 1);
  return s;
}

static int get_field_int(lua_State *L, int index, const char *field, int def) {
  lua_getfield(L, index, field);
  int result = def;
  if (lua_isnumber(L, -1)) {
    result = (int)lua_tointeger(L, -1);
  }
  lua_pop(L, 1);
  return result;
}

Effect parse_effect(lua_State *L, int index) {
  Effect out;
  memset(&out, 0, sizeof(Effect));

  if (index < 0) {
    index = lua_gettop(L) + index + 1;
  }

  lua_getfield(L, index, "kind");
  const char *kind = lua_tostring(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, index, "data");
  int data_index = lua_gettop(L);

  if (strcmp(kind, "error") == 0) {
    out.kind = EFFECT_ERROR;
    out.as.error.message = get_field_string(L, data_index, "message");
  }
  else if (strcmp(kind, "file") == 0) {
    out.kind = EFFECT_FILE;

    const char *content = get_field_string(L, data_index, "content");

    lua_getfield(L, data_index, "opts");
    int opts_index = lua_gettop(L);

    out.as.file.content = content;
    out.as.file.path = get_field_string(L, opts_index, "path");
    out.as.file.mode = get_field_string(L, opts_index, "mode");

    lua_pop(L, 1);
  }
  else if (strcmp(kind, "network") == 0) {
    out.kind = EFFECT_NETWORK;

    const char *content = get_field_string(L, data_index, "content");

    lua_getfield(L, data_index, "opts");
    int opts_index = lua_gettop(L);

    out.as.network.content = content;
    out.as.network.url = get_field_string(L, opts_index, "url");
    out.as.network.mode = get_field_string(L, opts_index, "mode");
    out.as.network.timeout = get_field_int(L, opts_index, "timeout", -1);

    lua_pop(L, 1);
  }

  lua_pop(L, 1);
  return out;
}
