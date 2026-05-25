//
// Created by Marcel on 22-05-2026.
//

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "functions.h"

#include "characters.h"
#include "errors.h"
#include "expressions.h"
#include "helpers.h"
#include "machine.h"

static ZxError zx_function_abs(ZxMachine machine, const uint8_t *function, size_t function_size, double *result) {
    if (function_size <= 1) {
        return ERR_SYNTAX_ERROR; // Geen argument meegegeven aan ABS
    }
    size_t output_size = function_size - 1;
    uint8_t expression[output_size];
    memcpy(expression, function + 1, output_size);

    double number;
    ZxError err = solve_expression_to_double(machine, expression, output_size, &number, 256);

    if (err != ERR_OK) {
        return err;
    }
    *result = number < 0 ? -number : number;
    return ERR_OK;

}

ZxError zx_num_function_call(ZxMachine machine, const uint8_t *function, const size_t function_size, double *result) {
    switch (function[0]) {
        case 189:  //ABS
            return zx_function_abs(machine, function, function_size, result);

    }

    return ERR_NOT_IMPLEMENTED;
}
