//
// Created by Marcel on 18-05-2026.
//

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "errors.h"
#include "machine.h"
#include "helpers.h"



ZxError solve_expression_to_string(ZxMachine *machine, const char **expression, char *result, const size_t result_size) {
    while (**expression == ' ') {
        (*expression)++;
    }

    if (**expression == '"') {
        size_t len = 0;
        (*expression)++;
        while (**expression != '"' && **expression != '\0' &&
            len < result_size - 1) {
            result[len] = **expression;
            len++;
            (*expression)++;
            }

        if (**expression == '"') {
            result[len] = '\0';
            (*expression)++;
            return ERR_OK;
        }
        return ERR_UNCLOSED_QUOTES;
    }

    if (isalpha(**expression)) {
        const char *second = *expression + 1;
        if (*second == '$') {
            (*expression)++;
            return ERR_INVALID_EXPRESSION; //TODO implement string variables
        }
        char var_name[MAX_VAR_NAME_LEN];
        size_t len = 0;
        while (isalnum(**expression) && **expression != '\0' && len < MAX_VAR_NAME_LEN - 1) {
            var_name[len] = **expression;
            len++;
            (*expression)++;
        }
        var_name[len] = '\0';
        float value;

        const ZxError err = machine_get_numeric(*machine, var_name, &value);
        if (err != ERR_OK) {
            return err;
        }
        snprintf(result, result_size, "%f", value);
        return ERR_OK;
    }

    if (isdigit(**expression) || **expression == '-') {
        size_t len = 0;
        char number[result_size];
        while ((isdigit(**expression) ||
            **expression == '-' ||
            **expression == '.' ||
            **expression == 'e') &&
            len < result_size - 1) {
                number[len] = **expression;
                len++;
                (*expression)++;
            }
        if (len > 0) {
            number[len] = '\0';
        } else {
            return ERR_INVALID_EXPRESSION;
        }

        if (!is_valid_number(number)) {
            return ERR_INVALID_EXPRESSION;
        }
        snprintf(result, result_size, "%s", number);
        return ERR_OK;
    }


    return ERR_INVALID_EXPRESSION;
}
