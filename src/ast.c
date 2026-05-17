//
// Created by Marcel on 16-05-2026.
//
#include "ast.h"
#include <string.h>

static Command cmd_print(const char **input) {
    Command command = {0};
    command.type = CMD_PRINT;
    *input += 5;

    while (**input == ' ') {
        (*input)++;
    }

    if (**input == '"') {
        size_t len = 0;
        (*input)++;
        while (**input != '"' && **input != '\0' &&
            len < sizeof(command.data.print_cmd.expression_string) - 1) {
            command.data.print_cmd.expression_string[len] = **input;
            len++;
            (*input)++;
            }

        if (**input == '"') {
            command.data.print_cmd.expression_string[len] = '\0';
            (*input)++;
            return command;
        } else {
            command.type = CMD_ERROR;
            strncpy(command.data.error_cmd.expression_string, "String niet afgesloten!", sizeof(command.data.error_cmd.expression_string) - 1);
            command.data.error_cmd.expression_string[sizeof(command.data.error_cmd.expression_string) - 1] = '\0';
            return command;
        }
    } else {
        command.data.print_cmd.expression_string[0] = '\0';
        return command;
    }
}

Command from_string(const char **input) {
    Command command = {0};

    while (**input == ' ') {
        (*input)++;
    }

    if (**input == '\0' || **input == '\n') {
        return command;
    }

    if (strncmp(*input, "PRINT", 5) == 0) {
        return cmd_print(input);
    } else {
        command.type = CMD_ERROR;
        strncpy(command.data.error_cmd.expression_string, "Alleen PRINT wordt ondersteund.", sizeof(command.data.error_cmd.expression_string) - 1);
        command.data.error_cmd.expression_string[sizeof(command.data.error_cmd.expression_string) - 1] = '\0';
        return command;
    }
}

