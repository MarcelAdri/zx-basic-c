//
// Created by Marcel on 22-05-2026.
//

#include <stdint.h>
#include "functions.h"
#include "characters.h"
#include "errors.h"

// De exacte 5-byte ROM waarde van PI op de ZX Spectrum (ROM adres 1A70)
#define ZX_ROM_PI 3.14159265

static uint32_t generate_random_int(ZxMachine machine);


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
static ZxError zx_function_pi(ZxValue *result) {
    return zx_assign_number(ZX_ROM_PI, result);
}
static ZxError zx_function_rnd(ZxMachine machine, ZxValue *result) {
    uint32_t x = generate_random_int(machine);
    double res = (double)x/4294967296.0;
    return zx_assign_number(res, result);
}
static ZxError zx_function_screen_s(ZxMachine machine, const ZxValue y, const ZxValue x, ZxValue *result) {
    double x_val, y_val;
    ZxError err = zx_get_number(x, &x_val);
    if (err != ERR_0_OK) {
        return err;
    }
    err = zx_get_number(y, &y_val);
    if (err != ERR_0_OK) {
        return err;
    }
    if (x_val < 0 || x_val > 31 || y_val < 0 || y_val > 21) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }
    const uint8_t *screen = machine_get_from_screen(machine, (uint8_t)y_val, (uint8_t)x_val);
    return zx_assign_string(screen, 1, result);
}

ZxError zx_function_call_no_arg(ZxMachine machine, const uint8_t function, ZxValue *result) {
    if (result == NULL || machine == NULL) {
        return ERR_UNKNOWN;
    }
    if (!is_no_arg_function(function)) {
        return ERR_UNKNOWN;
    }

    switch (function) {
        case ZX_FUN_INKEY_S:
            return ERR_NOT_YET_IMPLEMENTED;
        case ZX_FUN_PI:
            return zx_function_pi(result);
        case ZX_FUN_RND:
            return zx_function_rnd(machine, result);
    }

    return ERR_NOT_YET_IMPLEMENTED;
}
ZxError zx_function_call_1_arg(ZxMachine machine, const uint8_t function, const ZxValue argument, ZxValue *result) {
    if (result == NULL || machine == NULL) {
        return ERR_UNKNOWN;
    }
    if (!is_argument_function(function)) {
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
        case ZX_FUN_ABS:
            return zx_function_abs(argument, result);
        case ZX_FUN_CHR_S:
            return zx_function_chr_string(argument, result);
    }

    return ERR_NOT_YET_IMPLEMENTED;
}
ZxError zx_function_call_2_arg(ZxMachine machine, const uint8_t function, const ZxValue argument1, const ZxValue argument2, ZxValue *result) {
    if (result == NULL || machine == NULL) {
        return ERR_UNKNOWN;
    }
    if (!is_coordinate_function(function)) {
        return ERR_UNKNOWN;
    }
    if (argument1.type != ZX_TYPE_NUMBER || argument2.type != ZX_TYPE_NUMBER) {
        return ERR_A_INVALID_ARGUMENT;
    }

    switch (function) {
        case ZX_FUN_POINT:
            return ERR_NOT_YET_IMPLEMENTED;
        case ZX_FUN_SCREEN_S:
            return zx_function_screen_s(machine, argument1, argument2, result);
    }

    return ERR_NOT_YET_IMPLEMENTED;
}

static uint32_t generate_random_int(ZxMachine machine) {
    uint32_t x = machine_get_rng_state(machine);
    if (x == 0) {
        x = 12345;
    }
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    machine_set_rng_state(machine, x);
    return x;
}
