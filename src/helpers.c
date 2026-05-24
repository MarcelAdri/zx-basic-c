//
// Created by Marcel on 19-05-2026.
//

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "helpers.h"
#include "characters.h"
#include "errors.h"
#include "machine.h"

ZxError make_float(const char *text, float *out_float) {
    char *end = NULL;
    errno = 0;

    const float value = strtof(text, &end);

    if (end == text) {
        return ERR_INVALID_NUMBER; // geen geldig getal gevonden
    }

    while (*end == ' ') {
        end++;
    }

    if (*end != '\0') {
        return ERR_INVALID_EXPRESSION; // resttekst na het getal
    }

    if (errno == ERANGE) {
        return ERR_OUT_OF_RANGE; // buiten bereik
    }

    if (out_float != NULL) {
        *out_float = value;
    }
    return ERR_OK;
}
ZxError parse_number_to_string(const uint8_t *expression, size_t expression_size, char *number_string, const size_t result_size) {
    size_t i = 0;
    while (is_zx_space(expression[i]) && i < expression_size) {
        i++;
    }

    if (is_zx_number_start_character(expression[i])) {
        size_t len = 0;
        while (is_zx_number_character(expression[i]) &&
            len < result_size - 1 &&
            i < expression_size) {
            number_string[len] = *get_content_from_token(expression[i]);
            len++;
            i++;
            }
        if (len > 0) {
            number_string[len] = '\0';
            return make_float(number_string, NULL);
        }
        return ERR_INVALID_EXPRESSION;
    }
    return ERR_INVALID_EXPRESSION;
}

ZxError parse_number_to_float(const uint8_t *expression, size_t expression_size, float *number, const size_t result_size) {
    char number_string[result_size];

    const ZxError err = parse_number_to_string(expression, expression_size, number_string, result_size);
    if (err != ERR_OK) {
        return err;
    }

    return make_float(number_string, number);
}

ZxError parse_string_literal(const uint8_t *expression, size_t expression_size, char *literal, const size_t result_size) {

    size_t i = 0;
    while (is_zx_space(expression[i]) && i < expression_size) {
        i++;
    }

    if (expression[i] == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
        size_t len = 0;
        i++;
        while (expression[i] != get_token_from_key('"', KEYMAP_MODE_LITERAL) &&
            len < result_size - 1 &&
            i < expression_size) {
            if (!is_zx_printable_character(expression[i])) {
                return ERR_INVALID_STRING_LITERAL;
            }
            literal[len] = *get_content_from_token(expression[i]);
            len++;
            i++;
        }

        if (expression[i] == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
            literal[len] = '\0';
            return ERR_OK;
        }
        return ERR_UNCLOSED_QUOTES;
    }
    return ERR_INVALID_STRING_LITERAL;
}
ZxError parse_string_literal_with_quotes(const uint8_t *expression, size_t expression_size, char *literal, const size_t result_size) {
    const ZxError err = parse_string_literal(expression, expression_size, literal, result_size);
    if (err != ERR_OK) {
        return err;
    }

    size_t len = result_size;

    while (len > 0) {
        if (literal[len] == '\0'){
            if (len == result_size) {
                len--;
            } else if (len == result_size - 1) {
                literal[len + 1] = '\0';
                literal[len] = '"';
                len--;
            } else {
                literal[len + 2] = '\0';
                literal[len + 1] = '"';
                len--;
            }
        } else {
            literal[len + 1] = literal[len];
            len--;
        }
    }
    literal[0] = '"';
    return ERR_OK;
}

ZxError parse_variable_name(const uint8_t *expression, size_t expression_size, char *variable_name) {
    size_t i = 0;
    while (is_zx_space(expression[i]) && i < expression_size) {
        i++;
    }

    if (i == expression_size - 1) {
        return ERR_SYNTAX_ERROR;
    }

    if (is_zx_alpha(expression[i]) && i < expression_size - 1) {
        if (expression[i+1] == get_token_from_key('$', KEYMAP_MODE_LITERAL)) {
            variable_name[0] = *get_content_from_token(expression[i]);
            variable_name[1] = '$';
            variable_name[2] = '\0';
            return ERR_NOT_IMPLEMENTED; //TODO implement string variables
        }

        size_t len = 0;
        while ((is_zx_alnum(expression[i]) || is_zx_space(expression[i]))
            && i < expression_size &&  len < MAX_VAR_NAME_LEN - 1) {
            variable_name[len] = *get_content_from_token(expression[i]);
            len++;
            i++;
            }
        variable_name[len] = '\0';

        return ERR_OK;
    }
    return ERR_INVALID_VARIABLE_NAME;
}
