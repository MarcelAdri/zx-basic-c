//
// Created by Marcel on 18-05-2026.
//

#ifndef ZX_BASIC_C_EXPRESSIONS_H
#define ZX_BASIC_C_EXPRESSIONS_H

#include "errors.h"
#include "machine.h"
#include "zx_types.h"

#define SLICE_NO_TO (-1)
#define SLICE_OPEN_TO (-2)

typedef struct {
    ZxMachine machine;
    const uint8_t *buffer;
    size_t size;
    size_t cursor;
} ParserContext;

ZxError solve_expression(ZxMachine machine, const uint8_t *expression, size_t expression_size, ZxValue *result, size_t *bytes_read);
ZxError zx_parse_variable_reference(ZxMachine machine, const uint8_t *buffer, size_t size, size_t *bytes_read, char *out_var_name, uint16_t *out_indices, uint8_t *out_num_indices, int32_t *out_desired_len);

#endif //ZX_BASIC_C_EXPRESSIONS_H
