//
// Created by Marcel on 18-05-2026.
//
#include "errors.h"

#include <stdio.h>

void error_message(const ZxError error, char *message, const size_t message_size) {
    const char *text;

    switch (error) {
        case ERR_DIVISION_BY_ZERO:
            text = "Division by zero.";
            break;
        case ERR_MEM_ALLOCATION:
            text = "Problem while allocating.";
            break;
        case ERR_OUT_OF_RANGE:
            text = "Out of range error.";
            break;
        case ERR_SYNTAX_ERROR:
            text = "Syntax error.";
            break;
        case ERR_UNCLOSED_QUOTES:
            text = "Quotes are openend, but never closed.";
            break;
        case ERR_UNDEFINED_VARIABLE:
            text = "Variable not found.";
            break;
        case ERR_UNKNOWN_COMMAND:
            text = "Unknown command.";
            break;
        default:
            text = "Unknown error.";
            break;
    }

    snprintf(message, message_size, "ERROR: %s", text);

}
