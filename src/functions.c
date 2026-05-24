//
// Created by Marcel on 22-05-2026.
//

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "functions.h"
#include "errors.h"

ZxError function_to_string (const Function *input, char *output) {
    if (input == NULL || output == NULL) {
        return ERR_INVALID_ARGUMENT;
    }
    switch (input->type) {
        case FUNCTION_INT:
            snprintf(output, MAX_FUNCTION_NAME_LENGTH, "%s", FUNCTION_INT_NAME);
            return ERR_OK;
        default:
            return ERR_INVALID_FUNCTION_TYPE;
    }
}

ZxError function_from_string (const char *input, FunctionType *output) {
    if (input == NULL || output == NULL) {
        return ERR_INVALID_ARGUMENT;
    }
    if (strcmp(input, FUNCTION_INT_NAME) == 0) {
        *output = FUNCTION_INT;
        return ERR_OK;
    }
    return ERR_INVALID_FUNCTION_TYPE;
}

bool is_num_function(const Function *input) {
    if (input == NULL) {
        return false;
    }
    if (input->type == FUNCTION_INT) {
        return true;
    }
    return false;
}

bool is_string_function(const Function *input) {
    if (input == NULL) {
        return false;
    }
    //TODO: future string functions
    return false;
}