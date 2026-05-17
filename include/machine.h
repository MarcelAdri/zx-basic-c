//
// Created by Marcel on 17-05-2026.
//

#ifndef ZX_BASIC_C_MACHINE_H
#define ZX_BASIC_C_MACHINE_H
#include "errors.h"

#define MAX_VAR_NAME_LEN 100

typedef struct Machine* ZxMachine;

ZxMachine machine_create(void);
ZxError machine_set_numeric(ZxMachine machine, const char *var_name, float value);
ZxError machine_get_numeric(ZxMachine machine, const char *var_name, float *value);
void machine_destroy(ZxMachine machine);

#endif //ZX_BASIC_C_MACHINE_H
