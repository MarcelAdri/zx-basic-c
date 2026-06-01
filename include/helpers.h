//
// Created by marcel on 19-05-2026.
//

#ifndef ZX_BASIC_C_HELPERS_H
#define ZX_BASIC_C_HELPERS_H
#include "errors.h"
#include "zx_types.h"

ZxError make_double(const char *text, double *out_float);
ZxError formatted_number(double number, uint8_t *out_string, size_t out_string_size, size_t *bytes_written);
ZxError parse_number_to_double(const uint8_t *expression, size_t expression_size, ZxValue *out_number, size_t *bytes_read);
ZxError parse_variable_name(const uint8_t *expression, size_t expression_size, char *variable_name, size_t *bytes_read);
int name_to_index(uint8_t name);
ZxError parse_string_literal(const uint8_t *expression, size_t expression_size, ZxValue *literal, size_t *bytes_read);
ZxError parse_string_literal_with_quotes(const uint8_t *expression, size_t expression_size, ZxValue *literal);

#endif //ZX_BASIC_C_HELPERS_H
