//
// Created by Marcel on 18-05-2026.
//

#include <stdint.h>
#include <string.h>
#include "execute.h"

#include <stdio.h>
#include <math.h>
#include <time.h>

#include "errors.h"
#include "machine.h"
#include "expressions.h"
#include "characters.h"
#include "helpers.h"
#include "main.h"

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
                err = parse_variable_name(variable_name, name_size, var_name, &dummy_bytes_read);
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
static ZxError execute_cmd_load(ZxMachine machine, const uint8_t *cmd, size_t output_size) {
    if (output_size <= 1) {
        return ERR_C_NONSENSE_IN_BASIC;
    }
    ZxValue arg;
    zx_init_value(&arg);
    size_t dummy_bytes_read;
    ZxError err = solve_expression(machine, cmd + 1, output_size - 1, &arg, &dummy_bytes_read);
    if (err != ERR_0_OK) {
        zx_free_string(&arg);
        return err;
    }
    if (arg.type != ZX_TYPE_STRING) {
        zx_free_string(&arg);
        return ERR_C_NONSENSE_IN_BASIC;
    }
    uint8_t *arg_text = NULL;
    size_t arg_text_len = 0;
    zx_get_string(arg, &arg_text, &arg_text_len);
    zx_free_string(&arg);
    if (arg_text_len > 10) {
        return ERR_F_INVALID_FILENAME;
    }
    UI_trigger_load(machine);
    return ERR_0_OK;
}
static ZxError execute_cmd_new(ZxMachine machine) {
    machine_reset(machine);
    return ERR_0_OK;
}
static ZxError execute_cmd_pause(ZxMachine machine, const uint8_t *cmd, size_t output_size) {
    if (output_size <= 1) {
        return ERR_C_NONSENSE_IN_BASIC;
    }
    if (machine_get_wait_reason(machine) != ZX_WAIT_NONE) {
        return ERR_C_NONSENSE_IN_BASIC;
    }
    ZxError err;
    size_t dummy_bytes_read;
    ZxValue argument;
    zx_init_value(&argument);
    err = solve_expression(machine, cmd + 1, output_size - 1, &argument, &dummy_bytes_read);
    if (err != ERR_0_OK) {
        zx_free_string(&argument);
        return err;
    }
    if (argument.type != ZX_TYPE_NUMBER) {
        zx_free_string(&argument);
        return ERR_C_NONSENSE_IN_BASIC;
    }
    double argument_number;
    err = zx_get_number(argument, &argument_number);
    if (err != ERR_0_OK) {
        zx_free_string(&argument);
        return err;
    }
    int argument_int = (int)round(argument_number);
    zx_free_string(&argument);

    if (argument_int < 0 || argument_int > 65535) return ERR_B_INTEGER_OUT_OF_RANGE;

    machine_set_wait_reason(machine, ZX_WAIT_PAUSE);
    machine_set_pause_length(machine, argument_int);
    machine_set_pause_start_frame(machine, machine_get_frames(machine));
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
static ZxError execute_cmd_randomize(ZxMachine machine, const uint8_t *cmd, size_t output_size) {
    int rng_state;
    ZxError err;
    if (output_size <= 1) {
        rng_state = 0;
    } else {
        ZxValue value;
        zx_init_value(&value);
        size_t dummy_bytes_read;
        err = solve_expression(machine, cmd + 1, output_size - 1, &value, &dummy_bytes_read);
        if (err != ERR_0_OK) {
            zx_free_string(&value);
            return err;
        }
        if (value.type != ZX_TYPE_NUMBER) {
            zx_free_string(&value);
            return ERR_C_NONSENSE_IN_BASIC;
        }
        double value_number;
        err = zx_get_number(value, &value_number);
        if (err != ERR_0_OK) return err;

        rng_state = (int)round(value_number);
        zx_free_string(&value);


        if (rng_state < 0 || rng_state > 65535) {
            return ERR_B_INTEGER_OUT_OF_RANGE;
        }
    }
    if (rng_state == 0) {
        rng_state = (int)(machine_get_frames(machine) % 65536);
        if (rng_state == 0) rng_state = 1;
    }

    machine_set_rng_state(machine, rng_state);

    return ERR_0_OK;
}
static ZxError execute_cmd_run(ZxMachine machine, const uint8_t *cmd, size_t output_size) {
    machine_clear_variables(machine);

    if (output_size <= 1) {
        machine_set_current_line(machine, 1);
    } else {
        ZxValue arg;
        zx_init_value(&arg);
        size_t dummy_bytes_read;

        ZxError err = solve_expression(machine, cmd + 1, output_size - 1, &arg, &dummy_bytes_read);
        if (err != ERR_0_OK) {
            zx_free_string(&arg);
            return err;
        }
        if (arg.type != ZX_TYPE_NUMBER) {
            zx_free_string(&arg);
            return ERR_C_NONSENSE_IN_BASIC;
        }

        double line_num;
        zx_get_number(arg, &line_num);
        zx_free_string(&arg); // Kan direct na het uitlezen!

        if (line_num < 1) {
            line_num = 1;
        }
        if (line_num > 9999) {
            return ERR_B_INTEGER_OUT_OF_RANGE;
        }

        machine_set_current_line(machine, (uint16_t)line_num);
    }

    machine_set_current_statement(machine, 1);

    machine_set_state(machine, ZX_STATE_RUNNING);

    return ERR_0_OK;
}
static ZxError execute_cmd_save(ZxMachine machine, const uint8_t *cmd, size_t output_size) {
    if (output_size <= 1) {
        return ERR_C_NONSENSE_IN_BASIC;
    }
    ZxValue arg;
    zx_init_value(&arg);
    size_t dummy_bytes_read;
    ZxError err = solve_expression(machine, cmd + 1, output_size - 1, &arg, &dummy_bytes_read);
    if (err != ERR_0_OK) {
        zx_free_string(&arg);
        return err;
    }
    if (arg.type != ZX_TYPE_STRING) {
        zx_free_string(&arg);
        return ERR_C_NONSENSE_IN_BASIC;
    }
    uint8_t *arg_text = NULL;
    size_t arg_text_len = 0;
    zx_get_string(arg, &arg_text, &arg_text_len);
    if (arg_text_len < 1 || arg_text_len > 10) {
        zx_free_string(&arg);
        return ERR_F_INVALID_FILENAME;
    }
    char filename[11] = {0};
    size_t j = 0;
    for (size_t i = 0; i < arg_text_len; i++) {
        if (!is_zx_alnum(arg_text[i])) {
            filename[j++] = '_';
        } else {
            filename[j++] = (char)arg_text[i];
        }
    }
    filename[j] = '\0';
    zx_free_string(&arg);
    UI_trigger_save(machine, filename);
    return ERR_0_OK;
}

ZxError execute(ZxMachine machine, const uint8_t *input, const size_t input_size) {
    if (machine == NULL || input == NULL || input_size == 0) {
        return ERR_UNKNOWN;
    }
    switch (input[0]) {
        case ZX_STATEMENT_CLS:
            if (input_size != 1) {
                return ERR_C_NONSENSE_IN_BASIC;
            }
            return execute_cmd_cls(machine);
        case ZX_STATEMENT_LET:
            return execute_cmd_let(machine, input, input_size);
        case ZX_STATEMENT_LIST:
            return execute_cmd_list(machine, input, input_size);
        case ZX_STATEMENT_LOAD:
            return execute_cmd_load(machine, input, input_size);
        case ZX_STATEMENT_NEW:
            if (input_size != 1) {
                return ERR_C_NONSENSE_IN_BASIC;
            }
            return execute_cmd_new(machine);
        case ZX_STATEMENT_PAUSE:
            return execute_cmd_pause(machine, input, input_size);
        case ZX_STATEMENT_PRINT:
            return execute_cmd_print(machine, input, input_size);
        case ZX_STATEMENT_RANDOMIZE:
            return execute_cmd_randomize(machine, input, input_size);
        case ZX_STATEMENT_RUN:
            return execute_cmd_run(machine, input, input_size);
        case ZX_STATEMENT_SAVE:
            return execute_cmd_save(machine, input, input_size);
        default:
            return ERR_NOT_YET_IMPLEMENTED;
    }
}
ZxError execute_single_step(ZxMachine machine) {
    uint16_t current_line = machine_get_current_line(machine);
    uint8_t current_statement = machine_get_current_statement(machine);

    const uint8_t *line_buffer = NULL;
    size_t line_size = 0;

    // 1. Waar halen we de code vandaan?
    if (current_line == 0) {
        line_buffer = machine_get_direct_buffer(machine, &line_size);
    } else {
        line_buffer = machine_retrieve_program_line(machine, &current_line, &line_size);

        // BUGFIX 1: Synchroniseer de eventueel vooruitgespoelde regel direct terug naar de machine!
        machine_set_current_line(machine, current_line);
    }

    // 2. We hebben niets meer? Dan zijn we natuurlijk klaar met dit programma/commando!
    if (line_buffer == NULL || line_size == 0) {
        machine_set_state(machine, ZX_STATE_IDLE);
        return ERR_0_OK;
    }

    // 3. DE SLICER: Zoek het juiste 'hapklare brok' op basis van current_statement
    const uint8_t *chunk = NULL;
    size_t chunk_size = 0;
    extract_statement(line_buffer, line_size, current_statement, &chunk, &chunk_size);

    // 4. Zijn er geen statements meer op deze regel? Ga naar de volgende regel!
    if (chunk_size == 0) {
        if (current_line == 0) {
            // Direct mode is klaar!
            machine_set_state(machine, ZX_STATE_IDLE);

            // BUGFIX 2: Zet pointer terug naar het laatst uitgevoerde statement voor historische accuraatheid
            machine_set_current_statement(machine, current_statement - 1);
            return ERR_0_OK;
        } else {
            // Programma mode: Zoek de volgende regel in het geheugen!
            uint16_t next_line = machine_get_next_line(machine, current_line);
            if (next_line == 0) {
                machine_set_state(machine, ZX_STATE_IDLE); // Einde programma!

                // BUGFIX 2: Zet pointer terug naar het laatst uitgevoerde statement!
                machine_set_current_statement(machine, current_statement - 1);
            } else {
                machine_set_current_line(machine, next_line);
                machine_set_current_statement(machine, 1);
            }
            return ERR_0_OK; // Blijf vrolijk doordraaien in de batch
        }
    }

    // 5. Voer het hapklare brok uit!
    ZxError err = execute(machine, chunk, chunk_size);

    // 6. De administratie: Schuif de pointer op voor de volgende ronde!
    if (err == ERR_0_OK && chunk[0] != ZX_STATEMENT_RUN) {
        machine_set_current_statement(machine, current_statement + 1);
    }

    return err;
}

