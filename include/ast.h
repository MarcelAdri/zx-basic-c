//
// Created by Marcel on 16-05-2026.
//

#ifndef ZX_BASIC_C_AST_H
#define ZX_BASIC_C_AST_H
#include "machine.h"

typedef enum {
    CMD_LET,
    CMD_PRINT,
} CommandType;

typedef struct {
    char expression_string[256];
} CommandPrint;

typedef struct {
    char var_name[MAX_VAR_NAME_LEN];
    char expression_string[256];
} CommandLet;

typedef struct {
    int line_number;
    CommandType type;
    union {
        CommandLet cmd_let;
        CommandPrint cmd_print;
    } data;
} Command;

#include "errors.h"

ZxError command_from_string(const char **input, Command *out_command);

#endif //ZX_BASIC_C_AST_H
