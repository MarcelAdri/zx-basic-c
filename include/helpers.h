//
// Created by marcel on 19-05-2026.
//

#ifndef ZX_BASIC_C_HELPERS_H
#define ZX_BASIC_C_HELPERS_H
#include "errors.h"

ZxError make_double(const char *text, double *out_float);
char *format_double(double number);
ZxError formatted_number(double number, char *out_string, size_t out_string_size);
ZxError parse_number_to_double(const uint8_t *expression, size_t expression_size, double *number, size_t result_size, size_t *bytes_read);
ZxError parse_number_to_string(const uint8_t *expression, size_t expression_size, char *number_string, size_t result_size, size_t *bytes_read);
ZxError parse_variable_name(const uint8_t *expression, size_t expression_size, char *variable_name, size_t *bytes_read);
ZxError parse_string_literal(const uint8_t *expression, size_t expression_size, char *literal, size_t result_size, size_t *bytes_read);
ZxError parse_string_literal_with_quotes(const uint8_t *expression, size_t expression_size, char *literal, size_t result_size);

#endif //ZX_BASIC_C_HELPERS_H
