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

ZxError build_zx_sentence (const uint8_t *characters, size_t length, char *result);
const char* get_content_from_token (uint8_t token);
int get_token_from_key (char key, char mode);
char get_expected_cursor_mode(const uint8_t *buffer, size_t length);
bool is_zx_printable_character(uint8_t c);
bool is_zx_alnum(uint8_t c);
bool is_zx_alpha(uint8_t c);
bool is_zx_space(uint8_t c);
bool is_zx_number_character(uint8_t c);
bool is_zx_number_start_character(uint8_t c);
bool is_zx_plus_character(uint8_t c);
bool is_zx_minus_character(uint8_t c);
bool is_zx_asterisk_character(uint8_t c);
bool is_zx_slash_character(uint8_t c);
bool is_zx_power_character(uint8_t c);
bool is_num_function_num_arg(uint8_t c);
bool is_num_function_no_arg(uint8_t c);
bool is_num_function_str_arg(uint8_t c);
bool is_num_function(uint8_t c);
bool is_string_function_no_argument(uint8_t c);
bool is_string_function_str_argument(uint8_t c);
bool is_string_function_num_argument(uint8_t c);
bool is_string_function(uint8_t c);
bool is_function(uint8_t c);

#endif //ZX_BASIC_C_CHARACTERS_H
