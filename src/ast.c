//
// Created by Marcel on 16-05-2026.
//
#include <string.h>
#include "ast.h"
#include "errors.h"


static ZxError cmd_print(const char **input, Command *out_command) {
    out_command->type = CMD_PRINT;
    *input += 5;

    while (**input == ' ') {
        (*input)++;
    }

    if (**input == '"') {
        size_t len = 0;
        (*input)++;
        while (**input != '"' && **input != '\0' &&
            len < sizeof(out_command->data.print_cmd.expression_string) - 1) {
            out_command->data.print_cmd.expression_string[len] = **input;
            len++;
            (*input)++;
            }

        if (**input == '"') {
            out_command->data.print_cmd.expression_string[len] = '\0';
            (*input)++;
            return ERR_OK;
        }
        return ERR_UNCLOSED_QUOTES;
    }
    out_command->data.print_cmd.expression_string[0] = '\0';
    return ERR_SYNTAX_ERROR;
}

ZxError command_from_string(const char **input, Command *out_command) {
    *out_command = (Command){0};

    while (**input == ' ') {
        (*input)++;
    }

    if (**input == '\0' || **input == '\n') {
        return ERR_SYNTAX_ERROR;
    }

    if (strncmp(*input, "PRINT", 5) == 0) {
        return cmd_print(input, out_command);
    }

    return ERR_UNKNOWN_COMMAND;
}

