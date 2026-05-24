//
// Created by Marcel on 17-05-2026.
//

#ifndef ZX_BASIC_C_MACHINE_H
#define ZX_BASIC_C_MACHINE_H
#include "errors.h"

#define MAX_VAR_NAME_LEN 100
#define MAX_TEXT_SENTENCE_LEN 256
#define MAX_TOKEN_SENTENCE_LEN 200

typedef struct Machine* ZxMachine;
typedef void (*ZxPrintCallback)(const char *text);

ZxMachine machine_create(void);
ZxError machine_set_numeric(ZxMachine machine, const char *var_name, double value);
ZxError machine_get_numeric(ZxMachine machine, const char *var_name, double *value);
void machine_destroy(ZxMachine machine);
void machine_set_print_callback(ZxMachine machine, ZxPrintCallback callback);
void machine_print_output(ZxMachine machine, const char *text);

#endif //ZX_BASIC_C_MACHINE_H
