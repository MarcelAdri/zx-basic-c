//
// Created by Marcel on 22-05-2026.
//

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "functions.h"
#include "characters.h"
#include "errors.h"
#include "machine.h"
#include "expressions.h"

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
static ZxError zx_function_acs(ZxMachine machine, const ZxValue argument, ZxValue *result) {
    double arg;
    ZxError err = zx_get_number(argument, &arg);
    if (err != ERR_0_OK) {
        return err;
    }
    if (arg < -1.0 || arg > 1.0) {
        return ERR_A_INVALID_ARGUMENT;
    }

    //Calculate (pi / 2) + ASN(-arg)

    //retrieve pi
    ZxValue pi;
    zx_init_value(&pi);
    err = zx_function_call_no_arg(machine, ZX_FUN_PI, &pi);
    if (err != ERR_0_OK) {
        return err;
    }
    double pi_value;
    err = zx_get_number(pi, &pi_value);
    if (err != ERR_0_OK) {
        return err;
    }

    //Make -arg
    ZxValue neg_arg;
    zx_init_value(&neg_arg);
    err = zx_assign_number(-arg, &neg_arg);
    if (err != ERR_0_OK) {
        return err;
    }

    //retrieve ASN(-arg)
    ZxValue asn_arg_neg;
    zx_init_value(&asn_arg_neg);
    err = zx_function_call_1_arg(machine, ZX_FUN_ASN, neg_arg, &asn_arg_neg);
    if (err != ERR_0_OK) {
        return err;
    }
    double asn_arg_neg_value;
    err = zx_get_number(asn_arg_neg, &asn_arg_neg_value);
    if (err != ERR_0_OK) {
        return err;
    }

    //return calculation
    return zx_assign_number(pi_value / 2.0 + asn_arg_neg_value, result);
}
static ZxError zx_function_asn(ZxMachine machine, const ZxValue argument, ZxValue *result) {
    double arg;
    ZxError err = zx_get_number(argument, &arg);
    if (err != ERR_0_OK) {
        return err;
    }

    if (arg < -1.0 || arg > 1.0) {
        return ERR_A_INVALID_ARGUMENT;
    }

    //retrieve PI value
    ZxValue pi;
    zx_init_value(&pi);
    err = zx_function_call_no_arg(machine, ZX_FUN_PI, &pi);
    if (err != ERR_0_OK) {
        return err;
    }
    double pi_value;
    err = zx_get_number(pi, &pi_value);
    if (err != ERR_0_OK) {
        return err;
    }

    //Hardcoded avoidance of division by 0
    if (arg == 1.0) {
        return zx_assign_number(pi_value / 2.0, result);
    }
    if (arg == -1.0) {
        return zx_assign_number(-pi_value / 2.0, result);
    }

    //ASN: ATN(arg / SQR(1 - arg*arg)
    ZxValue one_minus_x2, sqrt_val, division_val;
    zx_init_value(&one_minus_x2);
    zx_init_value(&sqrt_val);
    zx_init_value(&division_val);

    //calculate (1 - arg*arg)
    err = zx_assign_number(1.0 - arg*arg, &one_minus_x2);
    if (err != ERR_0_OK) {
        return err;
    }

    //calculate sqrt(1 - arg*arg)
    err = zx_function_call_1_arg(machine, ZX_FUN_SQR, one_minus_x2, &sqrt_val);
    if (err != ERR_0_OK) {
        return err;
    }
    double sqrt_value;
    err = zx_get_number(sqrt_val, &sqrt_value);
    if (err != ERR_0_OK) {
        return err;
    }

    //calculate arg / SQR(1 - arg*arg)
    err = zx_assign_number(arg / sqrt_value, &division_val);
    if (err != ERR_0_OK) {
        return err;
    }

    //calculate ATN(division_value)
    return zx_function_call_1_arg(machine, ZX_FUN_ATN, division_val, result);
}
static ZxError zx_function_atn(const ZxValue argument, ZxValue *result) {
    double arg;
    ZxError err = zx_get_number(argument, &arg);
    if (err != ERR_0_OK) {
        return err;
    }
    return zx_assign_number(atan(arg), result);
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
static ZxError zx_function_cos(const ZxValue argument, ZxValue *result) {
    double arg;
    ZxError err = zx_get_number(argument, &arg);
    if (err != ERR_0_OK) {
        return err;
    }
    return zx_assign_number(cos(arg), result);
}
static ZxError zx_function_exp(const ZxValue argument, ZxValue *result) {
    double arg;
    ZxError err = zx_get_number(argument, &arg);
    if (err != ERR_0_OK) {
        return err;
    }

    return zx_assign_number(exp(arg), result);
}
static ZxError zx_function_int(const ZxValue argument, ZxValue *result) {
    double arg;
    ZxError err = zx_get_number(argument, &arg);
    if (err != ERR_0_OK) {
        return err;
    }

    return zx_assign_number(floor(arg), result);
}
static ZxError zx_function_len(const ZxValue argument, ZxValue *result) {
    uint8_t *text = NULL;
    size_t text_len = 0;
    zx_get_string(argument, &text, &text_len);
    return zx_assign_number((double)text_len, result);
}
static ZxError zx_function_ln(const ZxValue argument, ZxValue *result) {
    double arg;
    ZxError err = zx_get_number(argument, &arg);
    if (err != ERR_0_OK) {
        return err;
    }
    if (arg <= 0) {
        return ERR_A_INVALID_ARGUMENT;
    }
    return zx_assign_number(log(arg), result);
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
static ZxError zx_function_sgn(const ZxValue argument, ZxValue *result) {
    double arg;
    ZxError err = zx_get_number(argument, &arg);
    if (err != ERR_0_OK) {
        return err;
    }
    return zx_assign_number(arg < 0 ? -1 : arg > 0 ? 1 : 0, result);
}
static ZxError zx_function_sin(const ZxValue argument, ZxValue *result) {
    double arg;
    ZxError err = zx_get_number(argument, &arg);
    if (err != ERR_0_OK) {
        return err;
    }
    return zx_assign_number(sin(arg), result);
}
static ZxError zx_function_sqr(const ZxValue argument, ZxValue *result) {
    double arg;
    ZxError err = zx_get_number(argument, &arg);
    if (err != ERR_0_OK) {
        return err;
    }
    if (arg < 0) {
        return ERR_A_INVALID_ARGUMENT;
    }
    return zx_assign_number(sqrt(arg), result);
}
static ZxError zx_function_tan(ZxMachine machine, const ZxValue argument, ZxValue *result) {
    double arg;
    ZxError err = zx_get_number(argument, &arg);
    if (err != ERR_0_OK) {
        return err;
    }
    ZxValue arg_sin;
    zx_init_value(&arg_sin);
    err = zx_function_call_1_arg(machine, ZX_FUN_SIN, argument, &arg_sin);
    if (err != ERR_0_OK) {
        return err;
    }
    double arg_sin_value;
    err = zx_get_number(arg_sin, &arg_sin_value);
    if (err != ERR_0_OK) {
        return err;
    }
    ZxValue arg_cos;
    zx_init_value(&arg_cos);
    err = zx_function_call_1_arg(machine, ZX_FUN_COS, argument, &arg_cos);
    if (err != ERR_0_OK) {
        return err;
    }
    double arg_cos_value;
    err = zx_get_number(arg_cos, &arg_cos_value);
    if (err != ERR_0_OK) {
        return err;
    }

    if (arg_cos_value == 0) {
        return ERR_6_NUMBER_TOO_BIG;
    }

    return zx_assign_number(arg_sin_value / arg_cos_value, result);
}
static ZxError zx_function_val_s_string(ZxMachine machine, const ZxValue argument, ZxValue *result) {
    uint8_t *expr_text = NULL;
    size_t expr_len = 0;
    zx_get_string(argument, &expr_text, &expr_len);
    printf("Internal expr: %.*s\n", (int)expr_len, expr_text);

    ZxValue eval_result;
    zx_init_value(&eval_result);
    size_t bytes_read = 0;

    ZxError err = solve_expression(machine, expr_text, expr_len, &eval_result, &bytes_read);
    if (err != ERR_0_OK) {
        return ERR_C_NONSENSE_IN_BASIC;
    }

    if (eval_result.type != ZX_TYPE_STRING) {
        zx_free_string(&eval_result);
        return ERR_C_NONSENSE_IN_BASIC; // Oeps, de string leverde een getal op!
    }

    *result = eval_result;
    return ERR_0_OK;
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
        case ZX_FUN_ACS:
            return zx_function_acs(machine, argument, result);
        case ZX_FUN_ASN:
            return zx_function_asn(machine, argument, result);
        case ZX_FUN_ATN:
            return zx_function_atn(argument, result);
        case ZX_FUN_CHR_S:
            return zx_function_chr_string(argument, result);
        case ZX_FUN_COS:
            return zx_function_cos(argument, result);
        case ZX_FUN_EXP:
            return zx_function_exp(argument, result);
        case ZX_FUN_INT:
            return zx_function_int(argument, result);
        case ZX_FUN_LEN:
            return zx_function_len(argument, result);
        case ZX_FUN_LN:
            return zx_function_ln(argument, result);
        case ZX_FUN_SGN:
            return zx_function_sgn(argument, result);
        case ZX_FUN_SIN:
            return zx_function_sin(argument, result);
        case ZX_FUN_SQR:
            return zx_function_sqr(argument, result);
        case ZX_FUN_TAN:
            return zx_function_tan(machine, argument, result);
        case ZX_FUN_VAL_S:
            return zx_function_val_s_string(machine, argument, result);
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
        case ZX_FUN_ATTR:
            return ERR_NOT_YET_IMPLEMENTED;
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
