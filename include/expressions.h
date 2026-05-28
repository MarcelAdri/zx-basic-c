//
// Created by Marcel on 18-05-2026.
//

#ifndef ZX_BASIC_C_EXPRESSIONS_H
#define ZX_BASIC_C_EXPRESSIONS_H

#include "errors.h"
#include "machine.h"
#include "zx_types.h"

ZxError solve_expression_to_string(ZxMachine machine, const uint8_t *expression, size_t expression_size, char *result, size_t result_size, size_t *bytes_read);
ZxError solve_expression_to_number(ZxMachine machine, const uint8_t *expression, size_t expression_size, ZxValue *result, size_t result_size, size_t *bytes_read);

#endif //ZX_BASIC_C_EXPRESSIONS_H
