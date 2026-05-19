//
// Created by Marcel on 16-05-2026.
//
#include <string.h>
#include "ast.h"

#include <ctype.h>

#include "errors.h"
#include "helpers.h"
#include "machine.h"


static ZxError cmd_print(const char **input, Command *out_command) {
    out_command->type = CMD_PRINT;
    *input += 5;

    while (**input == ' ') {
        (*input)++;
    }

    if (**input == '"') {
        size_t len = 0;
        out_command->data.print_cmd.expression_string[len] = **input;
        len++;
        (*input)++;
        while (**input != '"' && **input != '\0' &&
            len < sizeof(out_command->data.print_cmd.expression_string) - 2) {
            out_command->data.print_cmd.expression_string[len] = **input;
            len++;
            (*input)++;
            }

        if (**input == '"') {
            out_command->data.print_cmd.expression_string[len] = '\"';
            out_command->data.print_cmd.expression_string[len + 1] = '\0';
            (*input) += 2;
            return ERR_OK;
        }
        return ERR_UNCLOSED_QUOTES;
    }

    if (isalpha(**input)) {
        const char *second = *input + 1;
        if (*second == '$') {
            out_command->data.print_cmd.expression_string[0] = **input;
            out_command->data.print_cmd.expression_string[1] = '$';
            out_command->data.print_cmd.expression_string[2] = '\0';
            (*input) += 3;
            return ERR_OK;
        }

        size_t len = 0;
        while (isalnum(**input) && **input != '\0' && len < sizeof(out_command->data.print_cmd.expression_string) - 1 &&
            len < MAX_VAR_NAME_LEN - 1) {
            out_command->data.print_cmd.expression_string[len] = **input;
            len++;
            (*input)++;
        }
        out_command->data.print_cmd.expression_string[len] = '\0';
        return ERR_OK;
    }

    if (isdigit(**input) || **input == '-') {
        size_t len = 0;
        while ((isdigit(**input) || **input == '-' || **input == 'e')
            && **input != '\0' && len < sizeof(out_command->data.print_cmd.expression_string) - 1 &&
            len < MAX_VAR_NAME_LEN - 1) {
            out_command->data.print_cmd.expression_string[len] = **input;
            len++;
            (*input)++;
        }
        out_command->data.print_cmd.expression_string[len] = '\0';
        if (!is_valid_number(out_command->data.print_cmd.expression_string)) {
            return ERR_INVALID_EXPRESSION;
        }
        return ERR_OK;
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

