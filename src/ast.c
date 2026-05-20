//
// Created by Marcel on 16-05-2026.
//
#include <string.h>
#include "ast.h"

#include <ctype.h>
#include <stdio.h>

#include "errors.h"
#include "helpers.h"
#include "machine.h"

static ZxError cmd_let(const char **input, Command *out_command) {
    ZxError err;
    out_command->type = CMD_LET;
    *input += 3;

    while (**input == ' ') {
        (*input)++;
    }
    err = parse_variable_name(input, out_command->data.cmd_let.var_name);
    if (err != ERR_OK) {
        return err;
    }

    while (**input == ' ') {
        (*input)++;
    }
    if (**input != '=') {
        return ERR_SYNTAX_ERROR;
    }
    (*input)++;
    while (**input == ' ') {
        (*input)++;
    }

    if (**input == '"') {
        return parse_string_literal_with_quotes(input, out_command->data.cmd_let.expression_string, sizeof(out_command->data.cmd_let.expression_string));
    }

    if (isalpha(**input)) {
        char var_name[MAX_VAR_NAME_LEN];
        err = parse_variable_name(input, var_name);
        if (err != ERR_OK) {
            return err;
        }
        snprintf(out_command->data.cmd_let.expression_string, sizeof(out_command->data.cmd_let.expression_string), "%s", var_name);
        return ERR_OK;
    }

    if (isdigit(**input) || **input == '-') {
        float value;
        err = make_float(*input, &value);
        if (err != ERR_OK) {
            return err;
        }
        snprintf(out_command->data.cmd_let.expression_string, sizeof(out_command->data.cmd_let.expression_string), "%f", value);
        return ERR_OK;
    }

    return ERR_INVALID_EXPRESSION;

}

static ZxError cmd_print(const char **input, Command *out_command) {
    ZxError err;
    out_command->type = CMD_PRINT;
    *input += 5;

    while (**input == ' ') {
        (*input)++;
    }

    if (**input == '"') {
        return parse_string_literal_with_quotes(input, out_command->data.cmd_print.expression_string, sizeof(out_command->data.cmd_print.expression_string));
    }

    if (isalpha(**input)) {
        char var_name[MAX_VAR_NAME_LEN];
        err = parse_variable_name(input, var_name);
        if (err != ERR_OK) {
            return err;
        }
        snprintf(out_command->data.cmd_print.expression_string, sizeof(out_command->data.cmd_print.expression_string), "%s", var_name);
        return ERR_OK;
    }

    if (isdigit(**input) || **input == '-') {
        float value;
        err = make_float(*input, &value);
        if (err != ERR_OK) {
            return err;
        }
        snprintf(out_command->data.cmd_print.expression_string, sizeof(out_command->data.cmd_print.expression_string), "%f", value);
        return ERR_OK;
    }


    out_command->data.cmd_print.expression_string[0] = '\0';
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
    if (strncmp(*input, "LET", 3) == 0) {
        return cmd_let(input, out_command);
    }

    return ERR_UNKNOWN_COMMAND;
}

