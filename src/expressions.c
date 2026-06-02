//
// Created by Marcel on 18-05-2026.
//

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "expressions.h"

#include <stdlib.h>
#include <string.h>

#include "characters.h"
#include "errors.h"
#include "functions.h"
#include "machine.h"
#include "helpers.h"

typedef struct {
    ZxMachine machine;
    const uint8_t *buffer;
    size_t size;
    size_t cursor;
} ParserContext;

static ZxError parse_expression(ParserContext *ctx, ZxValue *out_value);
static ZxError parse_logical_and(ParserContext *ctx, ZxValue *out_value);
static ZxError parse_relational(ParserContext *ctx, ZxValue *out_value);
static ZxError parse_arithmetic(ParserContext *ctx, ZxValue *out_value);
static ZxError parse_term(ParserContext *ctx, ZxValue *out_value);
static ZxError parse_power(ParserContext *ctx, ZxValue *out_value);
static ZxError parse_factor(ParserContext *ctx, ZxValue *out_value);

// Retourneert 0 als ze gelijk zijn, < 0 als left kleiner is, > 0 als left groter is
static ZxError zx_compare_strings(int *result, const ZxValue *left, const ZxValue *right) {
    if (left == NULL || right == NULL) return ERR_UNKNOWN;
    if (left->type != ZX_TYPE_STRING || right->type != ZX_TYPE_STRING) {
        return ERR_C_NONSENSE_IN_BASIC;
    }

    uint8_t *left_text = NULL;
    uint8_t *right_text = NULL;
    size_t left_len = 0;
    size_t right_len = 0;
    ZxError err = zx_get_string(*left, &left_text, &left_len);
    if (err != ERR_0_OK) return err;
    err = zx_get_string(*right, &right_text, &right_len);
    if (err != ERR_0_OK) return err;

    const size_t min_len = (left_len < right_len) ? left_len : right_len;

    const int cmp = memcmp(left_text, right_text, min_len);

    if (cmp != 0) {
        *result = cmp;
        return ERR_0_OK;
    }

    if (left_len < right_len) *result = -1;
    if (left_len > right_len) *result = 1;
    if (left_len == right_len) *result = 0;
    return ERR_0_OK;
}

static ZxError zx_calculate(uint8_t operator, ZxValue *left, ZxValue *right) {
    if (left == NULL || right == NULL) return ERR_UNKNOWN;
    ZxError err;

    if (is_zx_plus_character(operator) && left->type == ZX_TYPE_STRING && right->type == ZX_TYPE_STRING) {
        uint8_t *left_text = NULL;
        uint8_t *right_text = NULL;
        size_t left_length = 0;
        size_t right_length = 0;
        err = zx_get_string(*left, &left_text, &left_length);
        if (err != ERR_0_OK) return err;
        err = zx_get_string(*right, &right_text, &right_length);
        if (err != ERR_0_OK) return err;
        uint8_t *new_text = malloc(left_length + right_length);
        if (new_text == NULL) return ERR_4_OUT_OF_MEMORY;
        memcpy(new_text, left_text, left_length);
        memcpy(new_text + left_length, right_text, right_length);
        err = zx_assign_string(new_text, left_length + right_length, left);
        free(new_text);
        return err;
    }
    if (operator == ZX_OP_AND && left->type == ZX_TYPE_STRING && right->type == ZX_TYPE_NUMBER) {
        double rv;
        err = zx_get_number(*right, &rv);
        if (err != ERR_0_OK) return err;

        if (rv == ZX_FALSE) {
            return zx_assign_string((uint8_t *)"", 0, left);
        }
        return ERR_0_OK;
    }
    if (left->type == ZX_TYPE_STRING && is_zx_relational_character(operator)) {
        int cmp;
        err= zx_compare_strings(&cmp, left, right);
        if (err != ERR_0_OK) return err;

        double result;
        switch (operator) {
            case ZX_OP_LESS: result = (cmp < 0) ? ZX_TRUE : ZX_FALSE; break;    //<
            case ZX_OP_EQUAL: result = (cmp == 0) ? ZX_TRUE : ZX_FALSE; break;   //=
            case ZX_OP_GREATER: result = (cmp > 0) ? ZX_TRUE : ZX_FALSE; break;    //>
            case ZX_OP_LESS_EQ: result = (cmp <= 0) ? ZX_TRUE : ZX_FALSE; break;  //<=
            case ZX_OP_GTR_EQ: result = (cmp >= 0) ? ZX_TRUE : ZX_FALSE; break;  //>=
            case ZX_OP_NOT_EQ: result = (cmp != 0) ? ZX_TRUE : ZX_FALSE; break;  //<>


            default: return ERR_C_NONSENSE_IN_BASIC;
        }
        return zx_assign_number(result, left);
    }
    if (left->type == ZX_TYPE_STRING || right->type == ZX_TYPE_STRING) {
        return ERR_C_NONSENSE_IN_BASIC;
    }

    double lv, rv;
    err = zx_get_number(*left, &lv);
    if (err != ERR_0_OK) return err;

    err = zx_get_number(*right, &rv);
    if (err != ERR_0_OK) return err;

    switch (operator) {
        case ZX_OP_PLUS: lv += rv; break;                                            //+
        case ZX_OP_MINUS: lv -= rv; break;                                            //-
        case ZX_OP_MULTIPLY: lv *= rv; break;                                            //*
        case ZX_OP_DIVIDE: lv /= rv; break;                                            // /
        case ZX_OP_POWER: lv = pow(lv, rv); break;                               //↑
        case ZX_OP_LESS: lv = (lv < rv) ? ZX_TRUE : ZX_FALSE; break;                 //<
        case ZX_OP_EQUAL: lv = (lv == rv) ? ZX_TRUE : ZX_FALSE; break;                //=
        case ZX_OP_GREATER: lv = (lv > rv) ? ZX_TRUE : ZX_FALSE; break;                 //>
        case ZX_OP_LESS_EQ: lv = (lv <= rv) ? ZX_TRUE : ZX_FALSE; break;               //<=
        case ZX_OP_GTR_EQ: lv = (lv >= rv) ? ZX_TRUE : ZX_FALSE; break;               //>=
        case ZX_OP_NOT_EQ: lv = (lv != rv) ? ZX_TRUE : ZX_FALSE; break;               //<>
        case ZX_OP_AND: lv = (rv == ZX_FALSE) ? ZX_FALSE : lv; break;                 //AND
        case ZX_OP_OR: lv = (rv == ZX_FALSE) ? lv : ZX_TRUE; break;                   //OR

        default: return ERR_C_NONSENSE_IN_BASIC;
    }

    return zx_assign_number(lv, left);

}
static void zx_skip_spaces(ParserContext *ctx) {
    while (ctx->cursor < ctx->size && is_zx_space(ctx->buffer[ctx->cursor])) {
        ctx->cursor++;
    }
}
static ZxError parse_expression(ParserContext *ctx, ZxValue *out_value) {
    if (ctx == NULL || out_value == NULL) return ERR_UNKNOWN;
    ZxError err;

    ZxValue left_value;
    zx_init_value(&left_value);
    ZxValue right_value;
    zx_init_value(&right_value);

    err = parse_logical_and(ctx, &left_value);
    if (err != ERR_0_OK) goto error_cleanup;

    while (ctx->buffer[ctx->cursor] == ZX_OP_OR) {

        uint8_t operator = ctx->buffer[ctx->cursor];
        ctx->cursor++;
        zx_skip_spaces(ctx);

        err = parse_logical_and(ctx, &right_value);
        if (err != ERR_0_OK) goto error_cleanup;

        err = zx_calculate(operator, &left_value, &right_value);
        if (err != ERR_0_OK) goto error_cleanup;

        zx_free_string(&right_value);
    }

    *out_value = left_value;
    zx_free_string(&right_value);
    return ERR_0_OK;

    error_cleanup:
        zx_free_string(&left_value);
    zx_free_string(&right_value);
    return err;
}
static ZxError parse_logical_and(ParserContext *ctx, ZxValue *out_value) {
    if (ctx == NULL || out_value == NULL) return ERR_UNKNOWN;
    ZxError err;

    ZxValue left_value;
    zx_init_value(&left_value);
    ZxValue right_value;
    zx_init_value(&right_value);

    err = parse_relational(ctx, &left_value);
    if (err != ERR_0_OK) goto error_cleanup;

    while (ctx->buffer[ctx->cursor] == ZX_OP_AND) {

        uint8_t operator = ctx->buffer[ctx->cursor];
        ctx->cursor++;
        zx_skip_spaces(ctx);

        err = parse_relational(ctx, &right_value);
        if (err != ERR_0_OK) goto error_cleanup;

        err = zx_calculate(operator, &left_value, &right_value);
        if (err != ERR_0_OK) goto error_cleanup;

        zx_free_string(&right_value);
    }

    *out_value = left_value;
    zx_free_string(&right_value);
    return ERR_0_OK;

    error_cleanup:
        zx_free_string(&left_value);
        zx_free_string(&right_value);
        return err;
}
static ZxError parse_relational(ParserContext *ctx, ZxValue *out_value) {
    if (ctx == NULL || out_value == NULL) return ERR_UNKNOWN;
    ZxError err;

    ZxValue left_value;
    zx_init_value(&left_value);
    ZxValue right_value;
    zx_init_value(&right_value);

    err = parse_arithmetic(ctx, &left_value);
    if (err != ERR_0_OK) goto error_cleanup;

    while (is_zx_relational_character(ctx->buffer[ctx->cursor])) {

        uint8_t operator = ctx->buffer[ctx->cursor];
        ctx->cursor++;
        zx_skip_spaces(ctx);

        err = parse_arithmetic(ctx, &right_value);
        if (err != ERR_0_OK) goto error_cleanup;

        err = zx_calculate(operator, &left_value, &right_value);
        if (err != ERR_0_OK) goto error_cleanup;

        zx_free_string(&right_value);
           }

    *out_value = left_value;
    zx_free_string(&right_value);
    return ERR_0_OK;

    error_cleanup:
        zx_free_string(&left_value);
        zx_free_string(&right_value);
        return err;
}
static ZxError parse_arithmetic(ParserContext *ctx, ZxValue *out_value) {
    if (ctx == NULL || out_value == NULL) return ERR_UNKNOWN;
    ZxError err;

    ZxValue left_value;
    zx_init_value(&left_value);
    ZxValue right_value;
    zx_init_value(&right_value);

    err = parse_term(ctx, &left_value);
    if (err != ERR_0_OK) goto error_cleanup;

    while (is_zx_plus_character(ctx->buffer[ctx->cursor]) ||
           is_zx_minus_character(ctx->buffer[ctx->cursor])) {

        uint8_t operator = ctx->buffer[ctx->cursor];
        ctx->cursor++;
        zx_skip_spaces(ctx);

        err = parse_term(ctx, &right_value);
        if (err != ERR_0_OK) goto error_cleanup;

        err = zx_calculate(operator, &left_value, &right_value);
        if (err != ERR_0_OK) goto error_cleanup;

        zx_free_string(&right_value);
    }

    *out_value = left_value;
    zx_free_string(&right_value);
    return ERR_0_OK;

    error_cleanup:
        zx_free_string(&left_value);
        zx_free_string(&right_value);
        return err;
}
static ZxError parse_term(ParserContext *ctx, ZxValue *out_value) {
    if (ctx == NULL || out_value == NULL) return ERR_UNKNOWN;
    ZxError err;

    ZxValue left_value;
    zx_init_value(&left_value);
    ZxValue right_value;
    zx_init_value(&right_value);

    err = parse_power(ctx, &left_value);
    if (err != ERR_0_OK) goto error_cleanup;

    while (is_zx_asterisk_character(ctx->buffer[ctx->cursor]) ||
        is_zx_slash_character(ctx->buffer[ctx->cursor])) {
        uint8_t operator = ctx->buffer[ctx->cursor];
        ctx->cursor++;
        zx_skip_spaces(ctx);

        err = parse_power(ctx, &right_value);
        if (err != ERR_0_OK) goto error_cleanup;

        err = zx_calculate(operator, &left_value, &right_value);
        if (err != ERR_0_OK) goto error_cleanup;

        zx_free_string(&right_value);
    }

    *out_value = left_value;
    zx_free_string(&right_value);
    return ERR_0_OK;

    error_cleanup:
        zx_free_string(&left_value);
        zx_free_string(&right_value);
        return err;
}
static ZxError parse_power(ParserContext *ctx, ZxValue *out_value) {
    if (ctx == NULL || out_value == NULL) return ERR_UNKNOWN;
    ZxError err;

    ZxValue left_value;
    zx_init_value(&left_value);
    ZxValue right_value;
    zx_init_value(&right_value);

    err = parse_factor(ctx, &left_value);
    if (err != ERR_0_OK) goto error_cleanup;

    while (is_zx_power_character(ctx->buffer[ctx->cursor])) {
        uint8_t operator = ctx->buffer[ctx->cursor];
        ctx->cursor++;
        zx_skip_spaces(ctx);

        err = parse_factor(ctx, &right_value);
        if (err != ERR_0_OK) goto error_cleanup;

        err = zx_calculate(operator, &left_value, &right_value);
        if (err != ERR_0_OK) goto error_cleanup;

        zx_free_string(&right_value);
    }
    *out_value = left_value;
    zx_free_string(&right_value);
    return ERR_0_OK;

    error_cleanup:
        zx_free_string(&left_value);
        zx_free_string(&right_value);
        return err;
}
static ZxError parse_factor(ParserContext *ctx, ZxValue *out_value) {
    if (ctx == NULL || out_value == NULL) return ERR_UNKNOWN;
    ZxError err;
    uint8_t token = ctx->buffer[ctx->cursor];

    ZxValue value;
    zx_init_value(&value);

    if (token == get_token_from_key('(', KEYMAP_MODE_LITERAL)) {
        ctx->cursor++;
        zx_skip_spaces(ctx);

        err = parse_expression(ctx, &value);
        if (err != ERR_0_OK) goto error_cleanup;

        if (ctx->buffer[ctx->cursor] != get_token_from_key(')', KEYMAP_MODE_LITERAL)) {
            err = ERR_C_NONSENSE_IN_BASIC;
            goto error_cleanup;
        }
        ctx->cursor++;
        zx_skip_spaces(ctx);
        *out_value = value;
        return ERR_0_OK;
    }
    if (token == '+' || token == '-') {
        
        ctx->cursor++;
        zx_skip_spaces(ctx);

        err = parse_factor(ctx, &value);
        if (err != ERR_0_OK) goto error_cleanup;

        double value_number;
        err = zx_get_number(value, &value_number);
        if (err != ERR_0_OK) goto error_cleanup;
        value_number = (token == '-') ? -value_number : value_number;
        zx_free_string(&value);
        return zx_assign_number(value_number, out_value);
    }

    if (is_function(token)) {
        ctx->cursor++;
        zx_skip_spaces(ctx);

        if (is_argument_function(token)) {
            ZxValue arg;
            zx_init_value(&arg);
            err = parse_factor(ctx, &arg);
            if (err != ERR_0_OK) goto error_cleanup;
            err = zx_function_call_1_arg(ctx->machine, token, arg, out_value);
            zx_free_string(&arg);
            return err;
        }
        if (is_no_arg_function(token)) {
            err = zx_function_call_no_arg(ctx->machine, token, out_value);
            return err;
        }
        if (is_coordinate_function(token)) {
            if (ctx->buffer[ctx->cursor] != get_token_from_key('(', KEYMAP_MODE_LITERAL)) {
                err = ERR_C_NONSENSE_IN_BASIC;
                goto error_cleanup;
            }
            ctx->cursor++;
            zx_skip_spaces(ctx);

            ZxValue arg1, arg2;
            zx_init_value(&arg1);
            zx_init_value(&arg2);

            err = parse_expression(ctx, &arg1);
            if (err != ERR_0_OK) goto multi_arg_cleanup;

            if (ctx->buffer[ctx->cursor] != ',') {
                err = ERR_C_NONSENSE_IN_BASIC;
                goto multi_arg_cleanup;
            }
            ctx->cursor++;
            zx_skip_spaces(ctx);

            err = parse_expression(ctx, &arg2);
            if (err != ERR_0_OK) goto multi_arg_cleanup;

            if (ctx->buffer[ctx->cursor] != get_token_from_key(')', KEYMAP_MODE_LITERAL)) {
                err = ERR_C_NONSENSE_IN_BASIC;
                goto multi_arg_cleanup;
            }
            ctx->cursor++;
            zx_skip_spaces(ctx);

            err = zx_function_call_2_arg(ctx->machine, token, arg1, arg2, out_value);
            if (err != ERR_0_OK) goto multi_arg_cleanup;
            zx_free_string(&arg1);
            zx_free_string(&arg2);
            return ERR_0_OK;

            multi_arg_cleanup:
                zx_free_string(&arg1);
                zx_free_string(&arg2);
                goto error_cleanup;
        }
        err = ERR_C_NONSENSE_IN_BASIC;
        goto error_cleanup;
    }
    if (is_zx_number_start_character(token)) {
        size_t bytes_read;
        err = parse_number_to_double(ctx->buffer + ctx->cursor, ctx->size - ctx->cursor, out_value, &bytes_read);
        if (err != ERR_0_OK) goto error_cleanup;
        ctx->cursor += bytes_read;
        zx_skip_spaces(ctx);
        return ERR_0_OK;
    }
    if (is_zx_alpha(token)) {
        char variable_name[MAX_VAR_NAME_LEN];
        size_t bytes_read;
        err = parse_variable_name(ctx->buffer + ctx->cursor, ctx->size - ctx->cursor, variable_name, &bytes_read);
        if (err != ERR_0_OK) goto error_cleanup;
        ctx->cursor += bytes_read;
        zx_skip_spaces(ctx);
        if (strlen(variable_name) == 2 && variable_name[1] == get_token_from_key('$', KEYMAP_MODE_LITERAL)) {
            return machine_get_string(ctx->machine, variable_name[0], out_value);
        }
        return machine_get_numeric(ctx->machine, variable_name, out_value);
    }
    if (token == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
        size_t bytes_read;
        err = parse_string_literal(ctx->buffer + ctx->cursor, ctx->size - ctx->cursor, out_value, &bytes_read);
        if (err != ERR_0_OK) goto error_cleanup;
        ctx->cursor += bytes_read;
        zx_skip_spaces(ctx);
        return ERR_0_OK;
    }
    err = ERR_C_NONSENSE_IN_BASIC;

    error_cleanup:
        zx_free_string(&value);
        return err;
}

ZxError solve_expression(ZxMachine machine, const uint8_t *expression, size_t expression_size, ZxValue *result, size_t *bytes_read) {
    if (expression == NULL || result == NULL || bytes_read == NULL || machine == NULL) return ERR_UNKNOWN;

    ParserContext ctx = {0};
    ctx.machine = machine;
    ctx.buffer = expression;
    ctx.size = expression_size;
    ctx.cursor = 0;

    size_t i = 0;
    while (i < expression_size && is_zx_space(expression[i])) {
        i++;
    }

    if (i >= expression_size) {
        return ERR_C_NONSENSE_IN_BASIC;
    }
    ctx.cursor = i;

    ZxError err = parse_expression(&ctx, result);
    if (err != ERR_0_OK) return err;
    *bytes_read = ctx.cursor;
    return ERR_0_OK;
}

