//
// Created by marcel on 19-05-2026.
//

#ifndef ZX_BASIC_C_HELPERS_H
#define ZX_BASIC_C_HELPERS_H
#include <stdbool.h>

ZxError make_float(const char *text, float *out_float);
ZxError parse_number_to_float(const char **input, float *number, size_t result_size);
ZxError parse_number_to_string(const char **input, char *number_string, size_t result_size);
ZxError parse_variable_name(const char **input, char *variable_name);
ZxError parse_string_literal(const char **input, char *literal, size_t result_size);
ZxError parse_string_literal_with_quotes(const char **input, char *literal, size_t result_size);

#endif //ZX_BASIC_C_HELPERS_H
