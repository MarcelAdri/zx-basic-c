//
// Created by marcel on 19-05-2026.
//

#ifndef ZX_BASIC_C_HELPERS_H
#define ZX_BASIC_C_HELPERS_H
#include "errors.h"
#include "zx_types.h"
#include "machine.h"

ZxError list_program(ZxMachine *machine, uint16_t start_line, bool is_automatic);
ZxError make_double(const char *text, double *out_float);
ZxError formatted_number(double number, uint8_t *out_string, size_t out_string_size, size_t *bytes_written);
ZxError parse_number_to_double(const uint8_t *expression, size_t expression_size, ZxValue *out_number, size_t *bytes_read);
ZxError parse_variable_name(const uint8_t *expression, size_t expression_size, char *variable_name, size_t *bytes_read);
int name_to_index(uint8_t name);
ZxError parse_string_literal(const uint8_t *expression, size_t expression_size, ZxValue *literal, size_t *bytes_read);
ZxError parse_string_literal_with_quotes(const uint8_t *expression, size_t expression_size, ZxValue *literal);
ZxError machine_deserialize_program(ZxMachine machine, const uint8_t* buffer, size_t size);
uint8_t* machine_serialize_program(ZxMachine machine, size_t* out_size);
void extract_statement(const uint8_t *line_buffer, size_t line_size, uint8_t target_statement, const uint8_t **chunk, size_t *chunk_size);
size_t calculate_flat_index(uint16_t *dim_sizes, uint8_t num_dims, const uint16_t *parsed_indices);

#endif //ZX_BASIC_C_HELPERS_H
