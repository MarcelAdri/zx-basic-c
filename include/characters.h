//
// Created by Marcel on 22-05-2026.
//

#ifndef ZX_BASIC_C_CHARACTERS_H
#define ZX_BASIC_C_CHARACTERS_H

#include <stdbool.h>
#include "errors.h"

#define UNDEFINED_KEYSTROKE (-1)
#define KEYMAP_MODE_KEYWORD 'K'
#define KEYMAP_MODE_ABOVE 'F'
#define KEYMAP_MODE_LITERAL 'L'
#define KEYMAP_MODE_ON_RED 'E'
#define KEYMAP_MODE_BELOW 'G'

// ZX Spectrum specifieke tekens en tokens
#define ZX_OP_PLUS       43
#define ZX_OP_MINUS      45
#define ZX_OP_MULTIPLY   42
#define ZX_OP_DIVIDE     47

#define ZX_OP_LESS       60
#define ZX_OP_EQUAL      61
#define ZX_OP_GREATER    62

#define ZX_OP_POWER   128
#define ZX_OP_LESS_EQ 199
#define ZX_OP_GTR_EQ  200
#define ZX_OP_NOT_EQ  201
#define ZX_OP_AND     198
#define ZX_OP_OR      197
#define ZX_OP_NOT     195

#define ZX_TOKEN_BIN      196


//ZX Spectrum functies
#define ZX_FUN_ABS      189
#define ZX_FUN_ACS      182
#define ZX_FUN_ASN      181
#define ZX_FUN_AT       172
#define ZX_FUN_ATN      183
#define ZX_FUN_ATTR     171
#define ZX_FUN_CHR_S    194
#define ZX_FUN_CODE     175
#define ZX_FUN_COS      179
#define ZX_FUN_EXP      185
#define ZX_FUN_FN       168
#define ZX_FUN_IN       191
#define ZX_FUN_INKEY_S  166
#define ZX_FUN_INT      186
#define ZX_FUN_LEN      177
#define ZX_FUN_LN       184
#define ZX_FUN_PEEK     190
#define ZX_FUN_PI       167
#define ZX_FUN_POINT    169
#define ZX_FUN_RND      165
#define ZX_FUN_SCREEN_S 170
#define ZX_FUN_SGN      188
#define ZX_FUN_SIN      178
#define ZX_FUN_SQR      187
#define ZX_FUN_STR_S    193
#define ZX_FUN_TAB      173
#define ZX_FUN_TAN      180
#define ZX_FUN_USR      192
#define ZX_FUN_VAL      176
#define ZX_FUN_VAL_S    174

//Spectrum statements
#define ZX_STATEMENT_CLS 251
#define ZX_STATEMENT_LET 241
#define ZX_STATEMENT_LIST 240
#define ZX_STATEMENT_LOAD 239
#define ZX_STATEMENT_NEW 230
#define ZX_STATEMENT_PRINT 245
#define ZX_STATEMENT_RUN 247
#define ZX_STATEMENT_SAVE 248
#define ZX_STATEMENT_STOP 226


ZxError build_zx_sentence (const uint8_t *characters, size_t length, char *result);
const char* get_content_from_token (uint8_t token);
const char* get_printable_content_from_token (uint8_t token);
int get_token_from_key (char key, char mode);
ZxError string_to_zx_characters (const char *input, size_t input_length, uint8_t *output, size_t output_length, size_t *bytes_written);
char get_expected_cursor_mode(const uint8_t *buffer, size_t length);
bool is_zx_printable_character(uint8_t c);
bool is_zx_graphics_character(uint8_t c);
bool is_zx_alnum(uint8_t c);
bool is_zx_alpha(uint8_t c);
bool is_zx_space(uint8_t c);
bool is_no(uint8_t c);
bool is_zx_colon(uint8_t c);
bool is_zx_number_character(uint8_t c);
bool is_zx_digit_character(uint8_t c);
bool is_zx_relational_character(uint8_t c);
bool is_zx_number_start_character(uint8_t c);
bool is_zx_plus_character(uint8_t c);
bool is_zx_minus_character(uint8_t c);
bool is_zx_asterisk_character(uint8_t c);
bool is_zx_slash_character(uint8_t c);
bool is_zx_power_character(uint8_t c);
bool is_zx_quotes(uint8_t c);
bool is_num_function_num_arg(uint8_t c);
bool is_num_function_no_arg(uint8_t c);
bool is_num_function_str_arg(uint8_t c);
bool is_num_function_coordinate_arg(uint8_t c);
bool is_num_function(uint8_t c);
bool is_string_function_str_argument(uint8_t c);
bool is_string_function_num_argument(uint8_t c);
bool is_string_function_coordinate_argument(uint8_t c);
bool is_string_function_no_argument(uint8_t c);
bool is_string_function(uint8_t c);
bool is_argument_function(uint8_t c);
bool is_coordinate_function(uint8_t c);
bool is_no_arg_function(uint8_t c);
bool is_function_no_coordinate(uint8_t c);
bool is_function(uint8_t c);

#endif //ZX_BASIC_C_CHARACTERS_H
