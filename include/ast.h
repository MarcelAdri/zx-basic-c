//
// Created by Marcel on 16-05-2026.
//

#ifndef ZX_BASIC_C_AST_H
#define ZX_BASIC_C_AST_H

typedef enum {
    CMD_ERROR,
    CMD_PRINT
} CommandType;

typedef struct {
    char expression_string[256];
} CommandErrorData;

typedef struct {
    char expression_string[256];
} CommandPrintData;

typedef struct {
    int line_number;
    CommandType type;
    union {
        CommandErrorData error_cmd;
        CommandPrintData print_cmd;
    } data;
} Command;

Command from_string(const char **input);

#endif //ZX_BASIC_C_AST_H
