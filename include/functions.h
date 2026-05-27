//
// Created by marcel on 22-05-2026.
//

#ifndef ZX_BASIC_C_FUNCTIONS_H
#define ZX_BASIC_C_FUNCTIONS_H
#include <stddef.h>
#include "errors.h"
#include "machine.h"

ZxError zx_num_function_call(uint8_t function, double num_argument, const char *string_arg, double *result);


#endif //ZX_BASIC_C_FUNCTIONS_H
