//
// Created by marcel on 19-05-2026.
//

#ifndef ZX_BASIC_C_HELPERS_H
#define ZX_BASIC_C_HELPERS_H
#include "errors.h"

ZxError make_float(const char *text, float *out_float);
ZxError parse_number_to_float(const uint8_t *expression, size_t expression_size, float *number, size_t result_size);
ZxError parse_number_to_string(const uint8_t *expression, size_t expression_size, char *number_string, size_t result_size);
ZxError parse_variable_name(const uint8_t *expression, size_t expression_size, char *variable_name);
ZxError parse_string_literal(const uint8_t *expression, size_t expression_size, char *literal, size_t result_size);
ZxError parse_string_literal_with_quotes(const uint8_t *expression, size_t expression_size, char *literal, size_t result_size);

#endif //ZX_BASIC_C_HELPERS_H
