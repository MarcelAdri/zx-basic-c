//
// Created by Marcel on 19-05-2026.
//

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

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
ZxError parse_number_to_string(const char **input, char *number_string, const size_t result_size) {
    while (**input == ' ') {
        (*input)++;
    }
    if (isdigit(**input) || **input == '-' || **input == '.' || **input == '+') {
        size_t len = 0;
        while ((isdigit(**input) ||
            **input == '+' ||
            **input == '-' ||
            **input == '.' ||
            **input == 'e' ||
            **input == 'E') &&
            len < result_size - 1 &&
            **input != '\0') {
            number_string[len] = **input;
            len++;
            (*input)++;
            }
        if (len > 0) {
            number_string[len] = '\0';
            return make_float(number_string, NULL);
        }
        return ERR_INVALID_EXPRESSION;
    }
    return ERR_INVALID_EXPRESSION;
}

ZxError parse_number_to_float(const char **input, float *number, const size_t result_size) {
    char number_string[result_size];

    const ZxError err = parse_number_to_string(input, number_string, result_size);
    if (err != ERR_OK) {
        return err;
    }

    return make_float(number_string, number);
}

ZxError parse_string_literal(const char **input, char *literal, const size_t result_size) {
    while (**input == ' ') {
        (*input)++;
    }
    if (**input == '"') {
        size_t len = 0;
        (*input)++;
        while (**input != '"' && **input != '\0' &&
            len < result_size - 1) {
            literal[len] = **input;
            len++;
            (*input)++;
            }

        if (**input == '"') {
            literal[len] = '\0';
            (*input)++;
            return ERR_OK;
        }
        return ERR_UNCLOSED_QUOTES;
    }
    return ERR_INVALID_STRING_LITERAL;
}
ZxError parse_string_literal_with_quotes(const char **input, char *literal, const size_t result_size) {
    const ZxError err = parse_string_literal(input, literal, result_size);
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

ZxError parse_variable_name(const char **input, char *variable_name) {
    while (**input == ' ') {
        (*input)++;
    }

    if (**input == '\0') {
        return ERR_SYNTAX_ERROR;
    }

    if (isalpha(**input)) {
        const char *second = *input + 1;
        if (*second == '$') {
            variable_name[0] = **input;
            variable_name[1] = '$';
            variable_name[2] = '\0';
            (*input) += 3;
            return ERR_INVALID_VARIABLE_NAME; //TODO implement string variables
        }

        size_t len = 0;
        while ((isalnum(**input) || isspace(**input))
            && **input != '\0' &&  len < MAX_VAR_NAME_LEN - 1) {
            variable_name[len] = **input;
            len++;
            (*input)++;
            }
        variable_name[len] = '\0';

        return ERR_OK;
    }
    return ERR_INVALID_VARIABLE_NAME;
}
