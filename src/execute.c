//
// Created by Marcel on 18-05-2026.
//

#include <stdint.h>
#include <string.h>
#include "execute.h"
#include "errors.h"
#include "machine.h"
#include "expressions.h"
#include "characters.h"
#include "helpers.h"

static ZxError execute_cmd_let(ZxMachine machine, const uint8_t *cmd, size_t output_size) {
    bool in_variable_name = true;
    bool in_expression = false;
    uint8_t variable_name[MAX_VAR_NAME_LEN] = {0};
    uint8_t expr[256] = {0};
    char var_name[MAX_VAR_NAME_LEN];
    ZxError err;
    double value;
    size_t name_size = 0;
    size_t expr_size = 0;
    for (size_t i = 1; i < output_size; i++) {
        if (in_variable_name && name_size < MAX_VAR_NAME_LEN - 1) {
            if (cmd[i] == get_token_from_key('=', KEYMAP_MODE_LITERAL)) {
                in_variable_name = false;
                in_expression = true;
                err = parse_variable_name(variable_name, output_size, var_name, 0);
                if (err != ERR_OK) {
                    return err;
                }
                continue;
            }
            if (is_zx_alnum(cmd[i]) || is_zx_space(cmd[i])) {
                variable_name[name_size] = cmd[i];
                name_size++;
                continue;
            }
            return ERR_INVALID_VARIABLE_NAME;
        }
        if (in_expression && expr_size < sizeof(expr) - 1) {
            expr[expr_size] = cmd[i];
            expr_size++;
        }
    }
    if (in_variable_name) {
        return ERR_SYNTAX_ERROR;
    }
    err = solve_expression_to_double(machine,
        expr, expr_size, &value, 255);
    if (err != ERR_OK) {
        return err;
    }
    return machine_set_numeric(machine, var_name, value);
}

static ZxError execute_cmd_print(ZxMachine machine, const uint8_t *cmd, size_t output_size) {
    if (output_size <= 1) { // Alleen 'PRINT' getypt zonder argumenten
        machine_print_output(machine, "");
        return ERR_OK;
    }
    char result[256] = {0};
    uint8_t expression[output_size - 1];
    memcpy(expression, cmd + 1, output_size - 1);

    const ZxError err = solve_expression_to_string(machine,
         expression, sizeof(expression), result, sizeof(result));
    if (err == ERR_OK) {
        machine_print_output(machine, result);
    }
    return err;
}

ZxError execute(ZxMachine machine, const uint8_t *input, const size_t input_size) {
    if (machine == NULL || input == NULL || input_size == 0) {
        return ERR_INVALID_ARGUMENT;
    }
    size_t output_size = 0;
    size_t output_counter = 0;
    size_t input_counter = 0;
    bool in_string_literal = false;
    uint8_t command[input_size];
    ZxError err;
    while (input_counter <= input_size) {
        if (input[input_counter] == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
            in_string_literal = !in_string_literal;
        }
        if (input_counter == input_size ||
                (!in_string_literal &&
                input[input_counter] == get_token_from_key(':', KEYMAP_MODE_LITERAL))) {

            output_size = output_counter;

            if (output_size > 0) {
                switch (command[0]) {
                    case 245: //PRINT
                        err = execute_cmd_print(machine, command, output_size);
                        break;
                    case 241: //LET
                        err = execute_cmd_let(machine, command, output_size);
                        break;
                    default:
                        return ERR_NOT_IMPLEMENTED;
                }
                if (err != ERR_OK) {
                    return err;
                }
            }

            output_counter = 0;

            if (input_counter == input_size) {
                break;
            }

            input_counter++;
            continue;
        }

        command[output_counter] = input[input_counter];
        input_counter++;
        output_counter++;
    }

    return ERR_OK;
}

