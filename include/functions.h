//
// Created by marcel on 22-05-2026.
//

#ifndef ZX_BASIC_C_FUNCTIONS_H
#define ZX_BASIC_C_FUNCTIONS_H
#include <stddef.h>
#include "errors.h"
#include "machine.h"

ZxError zx_num_function_call(ZxMachine machine, const uint8_t *function, size_t function_size, double *result);


#endif //ZX_BASIC_C_FUNCTIONS_H
