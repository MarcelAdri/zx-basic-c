//
// Created by Marcel on 22-05-2026.
//

#include <stdint.h>
#include "functions.h"
#include "characters.h"
#include "errors.h"

static ZxError zx_function_abs(const double argument, double *result) {
    *result = argument < 0 ? -argument : argument;
    return ERR_OK;

}

ZxError zx_num_function_call(const uint8_t function, const double num_argument, const char *string_arg, double *result) {
    if (result == NULL) {
        return ERR_INVALID_ARGUMENT;
    }
    if (is_num_function_str_arg(function) && string_arg == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    switch (function) {
        case 189:  //ABS
            return zx_function_abs(num_argument, result);

    }

    return ERR_NOT_IMPLEMENTED;
}
