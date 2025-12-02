#ifndef __HERMES_EFFECT
#define __HERMES_EFFECT

#include <lua.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { EFFECT_ERROR, EFFECT_FILE, EFFECT_NETWORK } EffectKind;

typedef struct {
  const char *message;
} ErrorEffect;

typedef struct {
  const char *mode; // "w", "a", "x"
  const char *path;
  const char *content;
} FileEffect;

typedef struct {
  const char *mode; // "post", "put", "patch", "send"
  const char *url;
  const char *content;
  int timeout; // -1 if absent
} NetworkEffect;

typedef struct {
  EffectKind kind;
  union {
    ErrorEffect error;
    FileEffect file;
    NetworkEffect network;
  } as;
} Effect;

#ifdef __cplusplus
}
#endif


Effect *parse_effect(lua_State *L, int index);

#endif
