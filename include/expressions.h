//
// Created by Marcel on 18-05-2026.
//

#ifndef ZX_BASIC_C_EXPRESSIONS_H
#define ZX_BASIC_C_EXPRESSIONS_H

#include "errors.h"
#include "machine.h"

ZxError solve_expression_to_string(ZxMachine *machine, const char **expression, char *result, const size_t result_size);

#endif //ZX_BASIC_C_EXPRESSIONS_H
