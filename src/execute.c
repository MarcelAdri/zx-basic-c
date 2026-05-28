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

static ZxError execute_cmd_let(ZxMachine machine, const uint8_t *cmd, size_t output_size) {
    bool in_variable_name = true;
    bool in_expression = false;
    uint8_t variable_name[MAX_VAR_NAME_LEN] = {0};
    uint8_t expr[256] = {0};
    char var_name[MAX_VAR_NAME_LEN];
    ZxError err;
    ZxValue value;
    zx_init_value(&value);
    size_t name_size = 0;
    size_t expr_size = 0;
    for (size_t i = 1; i < output_size; i++) {
        if (in_variable_name && name_size < MAX_VAR_NAME_LEN - 1) {
            if (cmd[i] == get_token_from_key('=', KEYMAP_MODE_LITERAL)) {
                in_variable_name = false;
                in_expression = true;
                err = parse_variable_name(variable_name, output_size, var_name, 0);
                if (err != ERR_0_OK) {
                    return err;
                }
                continue;
            }
            if (is_zx_alnum(cmd[i]) || is_zx_space(cmd[i])) {
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
    err = solve_expression_to_number(machine,
        expr, expr_size, &value, 255, &bytes_read);
    if (err != ERR_0_OK) {
        return err;
    }
    return machine_set_numeric(machine, var_name, value);
}

static ZxError execute_cmd_print(ZxMachine machine, const uint8_t *cmd, size_t output_size) {
    if (output_size <= 1) {
        machine_print_to_text(machine, "\n"); // Lege print doet alleen een enter!
        return ERR_0_OK;
    }

    size_t cursor = 1;
    bool print_newline = true; // Standaard printen we een enter op het eind

    while (cursor < output_size) {
        print_newline = true;
        char result[256] = {0};

        // Let op: je moet solve_expression_to_string zo aanpassen dat hij
        // net als je nieuwe recursieve parser (via bytes_read of ctx) teruggeeft
        // hoeveel bytes hij heeft opgeslokt, anders weet je niet waar ; of , staat!
        size_t bytes_read = 0;
        ZxError err = solve_expression_to_string(machine, cmd + cursor, output_size - cursor, result, sizeof(result), &bytes_read);
        if (err != ERR_0_OK) return err;
        machine_print_to_text(machine, result);
        cursor += bytes_read;

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
                int spaces_needed = 16 - current_x;
                char space_buffer[32];

                snprintf(space_buffer, sizeof(space_buffer), "%*s", spaces_needed, "");

                machine_print_to_text(machine, space_buffer);
            } else {
                machine_print_to_text(machine, "\n");
            }

            print_newline = false;
            cursor++;
        } else if (separator == get_token_from_key('\'', KEYMAP_MODE_LITERAL)) {
            machine_print_to_text(machine, "\n");

            print_newline = false;
            cursor++;
        }
        else {
            return ERR_C_NONSENSE_IN_BASIC;
        }
    }

    if (print_newline) {
        machine_print_to_text(machine, "\n");
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
                    case 245: //PRINT
                        err = execute_cmd_print(machine, command, output_size);
                        break;
                    case 241: //LET
                        err = execute_cmd_let(machine, command, output_size);
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

