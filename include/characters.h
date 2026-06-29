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

#define ZX_TOKEN_ESC 27

// ZX Spectrum specifieke tekens en tokens
#define ZX_TOKEN_MASTER_LIST \
    X(ZX_CHAR_SPACE, 32, " ") \
    X(ZX_CHAR_EXCL, 33, "!") \
    X(ZX_CHAR_QUOTES, 34, "\"") \
    X(ZX_CHAR_HASH, 35, "#") \
    X(ZX_CHAR_DOLLAR, 36, "$") \
    X(ZX_CHAR_PERCENT, 37 , "%") \
    X(ZX_CHAR_ET, 38, "&") \
    X(ZX_CHAR_QUOTE, 39, "'") \
    X(ZX_CHAR_BRACKET_OPEN, 40, "(") \
    X(ZX_CHAR_BRACKET_CLOSE, 41, ")") \
    X(ZX_OP_MULTIPLY, 42, "*") \
    X(ZX_OP_PLUS, 43, "+") \
    X(ZX_CHAR_COMMA, 44, ",") \
    X(ZX_OP_MINUS, 45, "-") \
    X(ZX_CHAR_POINT, 46, ".") \
    X(ZX_OP_DIVIDE, 47, "/") \
    X(ZX_DIGIT_ZERO, 48, "0") \
    X(ZX_DIGIT_ONE, 49, "1") \
    X(ZX_DIGIT_TWO, 50, "2") \
    X(ZX_DIGIT_THREE, 51, "3") \
    X(ZX_DIGIT_FOUR, 52, "4") \
    X(ZX_DIGIT_FIVE, 53, "5") \
    X(ZX_DIGIT_SIX, 54, "6") \
    X(ZX_DIGIT_SEVEN, 55, "7") \
    X(ZX_DIGIT_EIGHT, 56, "8") \
    X(ZX_DIGIT_NINE, 57, "9") \
    X(ZX_CHAR_COLON, 58, ":") \
    X(ZX_CHAR_SEMICOLON, 59, ";") \
    X(ZX_OP_LESS, 60, "<") \
    X(ZX_OP_EQUAL, 61, "=") \
    X(ZX_OP_GREATER, 62, ">") \
    X(ZX_CHAR_QUESTION, 63, "?") \
    X(ZX_CHAR_AT, 64, "@") \
    X(ZX_UCASE_A, 65, "A") \
    X(ZX_UCASE_B, 66, "B") \
    X(ZX_UCASE_C, 67, "C") \
    X(ZX_UCASE_D, 68, "D") \
    X(ZX_UCASE_E, 69, "E") \
    X(ZX_UCASE_F, 70, "F") \
    X(ZX_UCASE_G, 71, "G") \
    X(ZX_UCASE_H, 72, "H") \
    X(ZX_UCASE_I, 73, "I") \
    X(ZX_UCASE_J, 74, "J") \
    X(ZX_UCASE_K, 75, "K") \
    X(ZX_UCASE_L, 76, "L") \
    X(ZX_UCASE_M, 77, "M") \
    X(ZX_UCASE_N, 78, "N") \
    X(ZX_UCASE_O, 79, "O") \
    X(ZX_UCASE_P, 80, "P") \
    X(ZX_UCASE_Q, 81, "Q") \
    X(ZX_UCASE_R, 82, "R") \
    X(ZX_UCASE_S, 83, "S") \
    X(ZX_UCASE_T, 84, "T") \
    X(ZX_UCASE_U, 85, "U") \
    X(ZX_UCASE_V, 86, "V") \
    X(ZX_UCASE_W, 87, "W") \
    X(ZX_UCASE_X, 88, "X") \
    X(ZX_UCASE_Y, 89, "Y") \
    X(ZX_UCASE_Z, 90, "Z") \
    X(ZX_CHAR_SQ_BRACK_OPEN, 91, "[") \
    X(ZX_CHAR_BACKSLASH, 92, "\\") \
    X(ZX_CHAR_SQ_BRACK_CLOSE, 93, "]") \
    X(ZX_OP_POWER, 94, "↑") \
    X(ZX_CHAR_UNDERSCORE, 95, "_") \
    X(ZX_CHAR_UKP, 96, "£") \
    X(ZX_LCASE_A, 97, "a") \
    X(ZX_LCASE_B, 98, "b") \
    X(ZX_LCASE_C, 99, "c") \
    X(ZX_LCASE_D, 100, "d") \
    X(ZX_LCASE_E, 101, "e") \
    X(ZX_LCASE_F, 102, "f") \
    X(ZX_LCASE_G, 103, "g") \
    X(ZX_LCASE_H, 104, "h") \
    X(ZX_LCASE_I, 105, "i") \
    X(ZX_LCASE_J, 106, "j") \
    X(ZX_LCASE_K, 107, "k") \
    X(ZX_LCASE_L, 108, "l") \
    X(ZX_LCASE_M, 109, "m") \
    X(ZX_LCASE_N, 110, "n") \
    X(ZX_LCASE_O, 111, "o") \
    X(ZX_LCASE_P, 112, "p") \
    X(ZX_LCASE_Q, 113, "q") \
    X(ZX_LCASE_R, 114, "r") \
    X(ZX_LCASE_S, 115, "s") \
    X(ZX_LCASE_T, 116, "t") \
    X(ZX_LCASE_U, 117, "u") \
    X(ZX_LCASE_V, 118, "v") \
    X(ZX_LCASE_W, 119, "w") \
    X(ZX_LCASE_X, 120, "x") \
    X(ZX_LCASE_Y, 121, "y") \
    X(ZX_LCASE_Z, 122, "z") \
    X(ZX_CHAR_CRLY_BRACK_OPEN, 123, "{") \
    X(ZX_CHAR_PIPE, 124, "|") \
    X(ZX_CHAR_CRLY_BRACK_CLOSE, 125, "}") \
    X(ZX_CHAR_TILDE, 126, "~") \
    X(ZX_CHAR_COPYRIGHT, 127, "©") \
    X(ZX_GRAPH_EIGHT, 128, "■") \
    X(ZX_GRAPH_ONE, 129, "■") \
    X(ZX_GRAPH_TWO, 130, "■") \
    X(ZX_GRAPH_THREE, 131, "■") \
    X(ZX_GRAPH_FOUR, 132, "■") \
    X(ZX_GRAPH_FIVE, 133, "■") \
    X(ZX_GRAPH_SIX, 134, "■") \
    X(ZX_GRAPH_SEVEN, 135, "■") \
    X(ZX_GRAPH_SHFT_SEVEN, 136, "■") \
    X(ZX_GRAPH_SHFT_SIX, 137, "■") \
    X(ZX_GRAPH_SHFT_FIVE, 138, "■") \
    X(ZX_GRAPH_SHFT_FOUR, 139, "■") \
    X(ZX_GRAPH_SHFT_THREE, 140, "■") \
    X(ZX_GRAPH_SHFT_TWO, 141, "■") \
    X(ZX_GRAPH_SHFT_ONE, 142, "■") \
    X(ZX_GRAPH_SHFT_EIGHT, 143, "■") \
    X(ZX_GRAPH_A, 144, "■") \
    X(ZX_GRAPH_B, 145, "■") \
    X(ZX_GRAPH_C, 146, "■") \
    X(ZX_GRAPH_D, 147, "■") \
    X(ZX_GRAPH_E, 148, "■") \
    X(ZX_GRAPH_F, 149, "■") \
    X(ZX_GRAPH_G, 150, "■") \
    X(ZX_GRAPH_H, 151, "■") \
    X(ZX_GRAPH_I, 152, "■") \
    X(ZX_GRAPH_J, 153, "■") \
    X(ZX_GRAPH_K, 154, "■") \
    X(ZX_GRAPH_L, 155, "■") \
    X(ZX_GRAPH_M, 156, "■") \
    X(ZX_GRAPH_N, 157, "■") \
    X(ZX_GRAPH_O, 158, "■") \
    X(ZX_GRAPH_P, 159, "■") \
    X(ZX_GRAPH_Q, 160, "■") \
    X(ZX_GRAPH_R, 161, "■") \
    X(ZX_GRAPH_S, 162, "■") \
    X(ZX_GRAPH_T, 163, "■") \
    X(ZX_GRAPH_U, 164, "■") \
    X(ZX_FUN_RND, 165, "RND ") \
    X(ZX_FUN_INKEY_S, 166, "INKEY$ ") \
    X(ZX_FUN_PI, 167, "PI ") \
    X(ZX_FUN_FN, 168, "FN ") \
    X(ZX_FUN_POINT, 169, "POINT ") \
    X(ZX_FUN_SCREEN_S, 170, "SCREEN$ ") \
    X(ZX_FUN_ATTR, 171, "ATTR ") \
    X(ZX_TOKEN_AT, 172, "AT ") \
    X(ZX_TOKEN_TAB, 173, "TAB ") \
    X(ZX_FUN_VAL_S, 174, "VAL$ ") \
    X(ZX_FUN_CODE, 175, "CODE ") \
    X(ZX_FUN_VAL, 176, "VAL ") \
    X(ZX_FUN_LEN, 177, "LEN ") \
    X(ZX_FUN_SIN, 178, "SIN ") \
    X(ZX_FUN_COS, 179, "COS ") \
    X(ZX_FUN_TAN, 180, "TAN ") \
    X(ZX_FUN_ASN, 181, "ASN ") \
    X(ZX_FUN_ACS, 182, "ACS ") \
    X(ZX_FUN_ATN, 183, "ATN ") \
    X(ZX_FUN_LN, 184, "LN ") \
    X(ZX_FUN_EXP, 185, "EXP ") \
    X(ZX_FUN_INT, 186, "INT ") \
    X(ZX_FUN_SQR, 187, "SQR ") \
    X(ZX_FUN_SGN, 188, "SGN ") \
    X(ZX_FUN_ABS, 189, "ABS ") \
    X(ZX_FUN_PEEK, 190, "PEEK ") \
    X(ZX_FUN_IN, 191, "IN ") \
    X(ZX_FUN_USR, 192, "USR ") \
    X(ZX_FUN_STR_S, 193, "STR$ ") \
    X(ZX_FUN_CHR_S, 194, "CHR$ ") \
    X(ZX_OP_NOT, 195, "NOT ") \
    X(ZX_TOKEN_BIN, 196, "BIN ") \
    X(ZX_OP_OR, 197, "OR ") \
    X(ZX_OP_AND, 198, "AND ") \
    X(ZX_OP_LESS_EQ, 199, "<=") \
    X(ZX_OP_GTR_EQ, 200, ">=") \
    X(ZX_OP_NOT_EQ, 201, "<>") \
    X(ZX_TOKEN_LINE, 202, "LINE ") \
    X(ZX_TOKEN_THEN, 203, " THEN ") \
    X(ZX_TOKEN_TO, 204, " TO ") \
    X(ZX_TOKEN_STEP, 205, " STEP ") \
    X(ZX_STATEMENT_DEF_FN, 206, "DEF FN ") \
    X(ZX_TOKEN_CAT, 207, " CAT ") \
    X(ZX_STATEMENT_FORMAT, 208, "FORMAT ") \
    X(ZX_STATEMENT_MOVE, 209, "MOVE ") \
    X(ZX_STATEMENT_ERASE, 210, "ERASE ") \
    X(ZX_STATEMENT_OPEN_H, 211, "OPEN# ") \
    X(ZX_STATEMENT_CLOSE_H, 212, "CLOSE# ") \
    X(ZX_STATEMENT_MERGE, 213, "MERGE ") \
    X(ZX_STATEMENT_VERIFY, 214, "VERIFY ") \
    X(ZX_STATEMENT_BEEP, 215, "BEEP ") \
    X(ZX_STATEMENT_CIRCLE, 216, "CIRCLE ") \
    X(ZX_STATEMENT_INK, 217, "INK ") \
    X(ZX_STATEMENT_PAPER, 218, "PAPER ") \
    X(ZX_STATEMENT_FLASH, 219, "FLASH ") \
    X(ZX_STATEMENT_BRIGHT, 220, "BRIGHT ") \
    X(ZX_STATEMENT_INVERSE, 221, "INVERSE ") \
    X(ZX_STATEMENT_OVER, 222, "OVER ") \
    X(ZX_STATEMENT_OUT, 223, "OUT ") \
    X(ZX_STATEMENT_LPRINT, 224, "LPRINT ") \
    X(ZX_STATEMENT_LLIST, 225, "LLIST ") \
    X(ZX_STATEMENT_STOP, 226, "STOP ") \
    X(ZX_STATEMENT_READ, 227, "READ ") \
    X(ZX_STATEMENT_DATA, 228, "DATA ") \
    X(ZX_STATEMENT_RESTORE, 229, "RESTORE ") \
    X(ZX_STATEMENT_NEW, 230, "NEW ") \
    X(ZX_STATEMENT_BORDER, 231, "BORDER ") \
    X(ZX_STATEMENT_CONTINUE, 232, "CONTINUE ") \
    X(ZX_STATEMENT_DIM, 233, "DIM ") \
    X(ZX_STATEMENT_REM, 234, "REM ") \
    X(ZX_STATEMENT_FOR, 235, "FOR ") \
    X(ZX_STATEMENT_GO_TO, 236, "GO TO ") \
    X(ZX_STATEMENT_GO_SUB, 237, "GO SUB ") \
    X(ZX_STATEMENT_INPUT, 238, "INPUT ") \
    X(ZX_STATEMENT_LOAD, 239, "LOAD ") \
    X(ZX_STATEMENT_LIST, 240, "LIST ") \
    X(ZX_STATEMENT_LET, 241, "LET ") \
    X(ZX_STATEMENT_PAUSE, 242, "PAUSE ") \
    X(ZX_STATEMENT_NEXT, 243, "NEXT ") \
    X(ZX_STATEMENT_POKE, 244, "POKE ") \
    X(ZX_STATEMENT_PRINT, 245, "PRINT ") \
    X(ZX_STATEMENT_PLOT, 246, "PLOT ") \
    X(ZX_STATEMENT_RUN, 247, "RUN ") \
    X(ZX_STATEMENT_SAVE, 248, "SAVE ") \
    X(ZX_STATEMENT_RANDOMIZE, 249, "RANDOMIZE ") \
    X(ZX_STATEMENT_IF, 250, "IF ") \
    X(ZX_STATEMENT_CLS, 251, "CLS ") \
    X(ZX_STATEMENT_DRAW, 252, "DRAW ") \
    X(ZX_STATEMENT_CLEAR, 253, "CLEAR ") \
    X(ZX_STATEMENT_RETURN, 254, "RETURN ") \
    X(ZX_STATEMENT_COPY, 255, "COPY ")

typedef enum {
#define X(name, id, str) name = id,
    ZX_TOKEN_MASTER_LIST
    #undef X
} ZxTokenId;

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
bool is_zx_break(uint8_t c);
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
