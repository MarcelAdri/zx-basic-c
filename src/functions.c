//
// Created by Marcel on 22-05-2026.
//

#include <stdint.h>
#include "functions.h"
#include "characters.h"
#include "errors.h"

static ZxError zx_function_abs(const ZxValue argument, ZxValue *result) {
    double arg;
    ZxError err = zx_get_number(argument, &arg);
    if (err != ERR_0_OK) {
        return err;
    }
    double res = arg < 0 ? -arg : arg;

    return zx_assign_number(res, result);

}
static ZxError zx_function_chr_string(const ZxValue argument, ZxValue *result) {
    double arg;
    ZxError err = zx_get_number(argument, &arg);
    if (err != ERR_0_OK) {
        return err;
    }
    if (arg < 0 || arg > 255) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }
    const uint8_t text = (uint8_t) arg;
    return zx_assign_string(&text, 1, result);
}

ZxError zx_function_call(const uint8_t function, const ZxValue argument, ZxValue *result) {
    if (result == NULL) {
        return ERR_UNKNOWN;
    }
    if ((is_num_function_num_arg(function) ||
        is_string_function_num_argument(function)) &&
        argument.type != ZX_TYPE_NUMBER) {
        return ERR_A_INVALID_ARGUMENT;
    }
    if ((is_num_function_str_arg(function) ||
        is_string_function_str_argument(function)) &&
        argument.type != ZX_TYPE_STRING) {
        return ERR_A_INVALID_ARGUMENT;
    }

    switch (function) {
        case 189:  //ABS
            return zx_function_abs(argument, result);
        case 194:  //CHR$
            return zx_function_chr_string(argument, result);

    }

    return ERR_NOT_YET_IMPLEMENTED;
}
