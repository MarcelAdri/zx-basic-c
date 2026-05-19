//
// Created by Marcel on 17-05-2026.
//

#ifndef ZX_BASIC_C_ERRORS_H
#define ZX_BASIC_C_ERRORS_H
#include <stddef.h>

typedef enum {
    ERR_OK = 0,
    ERR_DIVISION_BY_ZERO,
    ERR_INVALID_EXPRESSION,
    ERR_MEM_ALLOCATION,
    ERR_OUT_OF_RANGE,
    ERR_SYNTAX_ERROR,
    ERR_UNCLOSED_QUOTES,
    ERR_UNDEFINED_VARIABLE,
    ERR_UNKNOWN,
    ERR_UNKNOWN_COMMAND,

} ZxError;

void error_message(ZxError error, char *message, size_t message_size);

#endif //ZX_BASIC_C_ERRORS_H
