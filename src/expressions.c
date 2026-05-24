//
// Created by Marcel on 18-05-2026.
//

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>

#include "expressions.h"
#include "characters.h"
#include "errors.h"
#include "machine.h"
#include "helpers.h"

ZxError solve_expression_to_double(ZxMachine machine, const uint8_t *expression, size_t expression_size, double *result, const size_t result_size) {
    ZxError err;
    size_t i = 0;
    while (is_zx_space(expression[i])) {
        i++;
    }

    if (expression[i] == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
        return ERR_INVALID_NUMBER;
    }

    if (is_zx_alpha(expression[i])) {
        char var_name[MAX_VAR_NAME_LEN];

        err = parse_variable_name(expression, expression_size, var_name);
        if (err != ERR_OK) {
            return err;
        }

        double value;

        err = machine_get_numeric(machine, var_name, &value);
        if (err != ERR_OK) {
            return err;
        }
        *result = value;
        return ERR_OK;
    }

    if (is_zx_number_start_character(expression[i])) {
        double value;
        err = parse_number_to_double(expression, expression_size, &value, result_size);
        if (err != ERR_OK) {
            return err;
        }
        *result = value;
        return ERR_OK;
    }

    return ERR_INVALID_NUMBER;
}

ZxError solve_expression_to_string(ZxMachine machine, const uint8_t *expression, size_t expression_size, char *result, const size_t result_size) {
    ZxError err;
    bool is_literal = false;
    bool is_variable = false;
    bool is_number = false;

    size_t i = 0;
    while (is_zx_space(expression[i]) && i < expression_size) {
        i++;
    }

    if (expression[i] == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
        return parse_string_literal(expression, expression_size - i, result, result_size);
    }
    if (is_zx_alpha(expression[i])) {
        char variable_name[MAX_VAR_NAME_LEN];
        err = parse_variable_name(expression, expression_size, variable_name);
        if (err != ERR_OK) {
            return err;
        }
        double value;
        err = machine_get_numeric(machine, variable_name, &value);
        if (err != ERR_OK) {
            return err;
        }
        snprintf(result, result_size, "%f", value);
        return ERR_OK;

    }
    if (is_zx_number_start_character(expression[i])) {
        return parse_number_to_string(expression, expression_size, result, result_size);
    }

    return ERR_INVALID_EXPRESSION;
}

