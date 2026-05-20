//
// Created by Marcel on 18-05-2026.
//

#ifndef ZX_BASIC_C_EXECUTE_H
#define ZX_BASIC_C_EXECUTE_H
#include "errors.h"
#include "machine.h"

ZxError execute(ZxMachine machine, const char **input);

#endif //ZX_BASIC_C_EXECUTE_H
