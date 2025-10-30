#ifndef __LOM_H
#define __LOM_H

#ifdef __cplusplus
extern "C" {
#endif

int lom_init(void);
int lom_run(const char *script);
void lom_close(void);

#ifdef __cplusplus
}
#endif

#endif
