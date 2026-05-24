//
// Created by Marcel on 18-05-2026.
//

#ifndef ZX_BASIC_C_EXPRESSIONS_H
#define ZX_BASIC_C_EXPRESSIONS_H

#include "errors.h"
#include "machine.h"

ZxError solve_expression_to_string(ZxMachine machine, const uint8_t *expression, size_t expression_size, char *result, size_t result_size);
ZxError solve_expression_to_float(ZxMachine machine, const uint8_t *expression, size_t expression_size, float *result, size_t result_size);

#endif //ZX_BASIC_C_EXPRESSIONS_H
