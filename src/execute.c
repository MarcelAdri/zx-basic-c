//
// Created by Marcel on 18-05-2026.
//

#include <stdint.h>
#include <string.h>
#include "execute.h"

#include <stdio.h>

#include "errors.h"
#include "machine.h"
#include "expressions.h"
#include "characters.h"
#include "helpers.h"

static ZxError execute_cmd_cls(ZxMachine machine) {
    machine_clear_text_screen(machine);
    return ERR_0_OK;
}
static ZxError execute_cmd_let(ZxMachine machine, const uint8_t *cmd, size_t output_size) {
    bool in_variable_name = true;
    bool in_expression = false;
    uint8_t variable_name[MAX_VAR_NAME_LEN] = {0};
    uint8_t expr[256] = {0};
    char var_name[MAX_VAR_NAME_LEN];
    ZxError err;
    size_t name_size = 0;
    size_t expr_size = 0;
    for (size_t i = 1; i < output_size; i++) {
        if (in_variable_name && name_size < MAX_VAR_NAME_LEN - 1) {
            if (cmd[i] == get_token_from_key('=', KEYMAP_MODE_LITERAL)) {
                in_variable_name = false;
                in_expression = true;
                size_t dummy_bytes_read;
                err = parse_variable_name(variable_name, output_size, var_name, &dummy_bytes_read);
                if (err != ERR_0_OK) {
                    return err;
                }
                continue;
            }
            if (is_zx_alnum(cmd[i]) || is_zx_space(cmd[i]) || cmd[i] == get_token_from_key('$', KEYMAP_MODE_LITERAL)) {
                variable_name[name_size] = cmd[i];
                name_size++;
                continue;
            }
            return ERR_2_VARIABLE_NOT_FOUND;
        }
        if (in_expression && expr_size < sizeof(expr) - 1) {
            expr[expr_size] = cmd[i];
            expr_size++;
        }
    }
    if (in_variable_name) {
        return ERR_2_VARIABLE_NOT_FOUND;
    }
    size_t bytes_read;
    ZxValue result;
    zx_init_value(&result);
    err = solve_expression(machine,
        expr, expr_size, &result, &bytes_read);
    if (err != ERR_0_OK) {
        zx_free_string(&result);
        return err;
    }
    if (var_name[1] == '$') {
        if (result.type != ZX_TYPE_STRING) {
            zx_free_string(&result);
            return ERR_C_NONSENSE_IN_BASIC;
        }
        err = machine_set_string(machine, var_name[0], &result);
        zx_free_string(&result);
        return err;
    }
    if (result.type != ZX_TYPE_NUMBER) {
        zx_free_string(&result);
        return ERR_C_NONSENSE_IN_BASIC;
    }
    err = machine_set_numeric(machine, var_name, result);
    zx_free_string(&result);
    return err;
}
static ZxError execute_cmd_list(ZxMachine machine, const uint8_t *cmd, size_t output_size) {
    uint16_t start_line = 0;
    ZxError err;

    if (output_size > 1) {
        ZxValue line_number;
        zx_init_value(&line_number);
        size_t dummy_bytes_read;
        double start_line_value;
        err = solve_expression(machine, cmd + 1, output_size - 1, &line_number, &dummy_bytes_read);
        if (err != ERR_0_OK) return err;
        err = zx_get_number(line_number, &start_line_value);
        zx_free_string(&line_number);
        if (err != ERR_0_OK) return err;
        if (start_line_value < 0 || start_line_value > 9999) return ERR_B_INTEGER_OUT_OF_RANGE;
        start_line = (uint16_t)start_line_value;
    }
    return list_program(&machine, start_line, false);
}
static ZxError execute_cmd_new(ZxMachine machine) {
    machine_reset(machine);
    return ERR_0_OK;
}
static ZxError execute_cmd_print(ZxMachine machine, const uint8_t *cmd, size_t output_size) {
    if (output_size <= 1) {
        machine_next_line(machine);
        return ERR_0_OK;
    }

    size_t cursor = 1;
    bool print_newline = true; // Standaard printen we een enter op het eind

    while (cursor < output_size) {
        print_newline = true;
        ZxValue result;
        zx_init_value(&result);

        size_t bytes_read = 0;
        ZxError err = solve_expression(machine, cmd + cursor, output_size - cursor, &result, &bytes_read);
        if (err != ERR_0_OK) {
            zx_free_string(&result);
            return err;
        }
        machine_print_value(machine, result);
        zx_free_string(&result);
        cursor += bytes_read;

        while (cursor < output_size && is_zx_space(cmd[cursor])) {
            cursor++;
        }

        if (cursor >= output_size) break;

        // Kijk wat het scheidingsteken is
        uint8_t separator = cmd[cursor];

        if (separator == get_token_from_key(';', KEYMAP_MODE_LITERAL)) {
            print_newline = false; // Bij ";" onderdrukken we de enter
            cursor++;
        }
        else if (separator == get_token_from_key(',', KEYMAP_MODE_LITERAL)) {
            int current_x = machine_get_text_cursor_x(machine);
            if (current_x < 16) {
                machine_set_text_cursor_x(machine, 16);
            } else {
                machine_next_line(machine);
            }

            print_newline = false;
            cursor++;
        } else if (separator == get_token_from_key('\'', KEYMAP_MODE_LITERAL)) {
            machine_next_line(machine);

            print_newline = false;
            cursor++;
        }
        else {
            return ERR_C_NONSENSE_IN_BASIC;
        }
    }

    if (print_newline) {
        machine_next_line(machine);
    }

    return ERR_0_OK;
}

ZxError execute(ZxMachine machine, const uint8_t *input, const size_t input_size) {
    if (machine == NULL || input == NULL || input_size == 0) {
        return ERR_UNKNOWN;
    }
    size_t output_size = 0;
    size_t output_counter = 0;
    size_t input_counter = 0;
    bool in_string_literal = false;
    uint8_t command[input_size];
    uint8_t statement_counter = 1;

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
                machine_set_location(machine, 0, statement_counter);
                switch (command[0]) {
                    case ZX_STATEMENT_CLS:
                        if (output_size != 1) {
                            return ERR_C_NONSENSE_IN_BASIC;
                        }
                        err = execute_cmd_cls(machine);
                        break;
                    case ZX_STATEMENT_LET:
                        err = execute_cmd_let(machine, command, output_size);
                        break;
                    case ZX_STATEMENT_LIST:
                        err = execute_cmd_list(machine, command, output_size);
                        break;
                    case ZX_STATEMENT_NEW:
                        err = execute_cmd_new(machine);
                        break;
                    case ZX_STATEMENT_PRINT:
                        err = execute_cmd_print(machine, command, output_size);
                        break;
                    default:
                        return ERR_NOT_YET_IMPLEMENTED;
                }
                if (err != ERR_0_OK) {
                    return err;
                }
            }

            output_counter = 0;
            statement_counter++;

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

    return ERR_0_OK;
}

