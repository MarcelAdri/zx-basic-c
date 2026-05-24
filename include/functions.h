//
// Created by marcel on 22-05-2026.
//

#ifndef ZX_BASIC_C_FUNCTIONS_H
#define ZX_BASIC_C_FUNCTIONS_H
#include <stddef.h>

const size_t MAX_FUNCTION_NAME_LENGTH = 4;
const char *FUNCTION_INT_NAME = "INT";

typedef enum FunctionType
{
    FUNCTION_INT,
} FunctionType;

typedef struct StringArgument {
    char argument_string[256];
} StringArgument;

typedef struct NumArgument {
    float argument_number;
} NumArgument;

typedef struct Function {
    FunctionType type;
    union {
        NumArgument num_arg;
        StringArgument str_arg;
    } argument;
} Function;

#endif //ZX_BASIC_C_FUNCTIONS_H
