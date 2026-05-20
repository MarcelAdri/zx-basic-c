//
// Created by Marcel on 18-05-2026.
//

#include <ctype.h>
#include <stdio.h>

#include "errors.h"
#include "machine.h"
#include "helpers.h"

ZxError solve_expression_to_float(ZxMachine machine, const char **expression, float *result, const size_t result_size) {
    ZxError err;

    if (**expression == '"') {
        return ERR_INVALID_NUMBER;
    }

    if (isalpha(**expression)) {
        char var_name[MAX_VAR_NAME_LEN];

        err = parse_variable_name(expression, var_name);
        if (err != ERR_OK) {
            return err;
        }

        float value;

        err = machine_get_numeric(machine, var_name, &value);
        if (err != ERR_OK) {
            return err;
        }
        *result = value;
        return ERR_OK;
    }

    if (isdigit(**expression) || **expression == '-') {
        float value;
        err = parse_number_to_float(expression, &value, result_size);
        if (err != ERR_OK) {
            return err;
        }
        *result = value;
        return ERR_OK;
    }

    return ERR_INVALID_NUMBER;
}

ZxError solve_expression_to_string(ZxMachine machine, const char **expression, char *result, const size_t result_size) {
    ZxError err;

    if (**expression == '"') {
        char text[result_size];
        err = parse_string_literal(expression, text, result_size);
        if (err != ERR_OK) {
            return err;
        }
        snprintf(result, result_size, "%s", text);
        return ERR_OK;
    }


    if (isalpha(**expression)) {
        char var_name[MAX_VAR_NAME_LEN];

        err = parse_variable_name(expression, var_name);
        if (err != ERR_OK) {
            return err;
        }

        float value;

        err = machine_get_numeric(machine, var_name, &value);
        if (err != ERR_OK) {
            return err;
        }
        snprintf(result, result_size, "%f", value);
        return ERR_OK;
    }

    if (isdigit(**expression) || **expression == '-') {
        float value;
        err = parse_number_to_float(expression, &value, result_size);
        if (err != ERR_OK) {
            return err;
        }
        snprintf(result, result_size, "%f", value);
        return ERR_OK;
    }

    return ERR_INVALID_EXPRESSION;
}
