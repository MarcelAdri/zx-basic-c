//
// Created by Marcel on 18-05-2026.
//

#include "ast.h"
#include "errors.h"
#include "machine.h"

static ZxError execute_cmd_print(ZxMachine *machine, Command *cmd) {
    machine_print_output(*machine, cmd->data.print_cmd.expression_string);
    return ERR_OK;
}

ZxError execute(ZxMachine *machine, const char **input) {
    Command cmd = {0};
    const int error = command_from_string(input, &cmd);
    if (error != ERR_OK) {
        return error;
    }
    switch (cmd.type) {
        case CMD_PRINT:
            return execute_cmd_print(machine, &cmd);
    }

    return ERR_UNKNOWN_COMMAND;
}

