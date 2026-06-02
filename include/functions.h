//
// Created by marcel on 22-05-2026.
//

#ifndef ZX_BASIC_C_FUNCTIONS_H
#define ZX_BASIC_C_FUNCTIONS_H
#include <stddef.h>
#include "errors.h"
#include "machine.h"

ZxError zx_function_call(ZxMachine machine, uint8_t function, ZxValue argument, ZxValue *result);


#endif //ZX_BASIC_C_FUNCTIONS_H
