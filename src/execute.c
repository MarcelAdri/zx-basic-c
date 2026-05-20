//
// Created by Marcel on 18-05-2026.
//

#include "ast.h"
#include "errors.h"
#include "machine.h"
#include "expressions.h"

static ZxError execute_cmd_let(ZxMachine machine, Command *cmd) {
    float value;
    const char *name = cmd->data.cmd_let.var_name;
    const char *expression = cmd->data.cmd_let.expression_string;
    const ZxError err = solve_expression_to_float(machine,
        &expression, &value, sizeof(cmd->data.cmd_let.expression_string));
    if (err != ERR_OK) {
        return err;
    }
    return machine_set_numeric(machine, name, value);
}

static ZxError execute_cmd_print(ZxMachine machine, Command *cmd) {
    char result[256] = {0};
    const char *expression = cmd->data.cmd_print.expression_string;
    const ZxError err = solve_expression_to_string(machine,
        &expression, result, sizeof(result));
    if (err == ERR_OK) {
        machine_print_output(machine, result);
    }

    return err;
}

ZxError execute(ZxMachine machine, const char **input) {
    Command cmd = {0};
    const ZxError error = command_from_string(input, &cmd);
    if (error != ERR_OK) {
        return error;
    }
    switch (cmd.type) {
        case CMD_LET:
            return execute_cmd_let(machine, &cmd);
        case CMD_PRINT:
            return execute_cmd_print(machine, &cmd);
    }

    return ERR_UNKNOWN_COMMAND;
}

