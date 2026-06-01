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

ZxError parse_expression(ParserContext *ctx, ZxValue *out_value);
ZxError parse_term(ParserContext *ctx, ZxValue *out_value);
ZxError parse_power(ParserContext *ctx, ZxValue *out_value);
ZxError parse_factor(ParserContext *ctx, ZxValue *out_value);

static ZxError zx_calculate(uint8_t operator, ZxValue *left, ZxValue *right) {
    if (left == NULL || right == NULL) return ERR_UNKNOWN;

    if (operator == '+' && left->type == ZX_TYPE_STRING && right->type == ZX_TYPE_STRING) {
        return ERR_NOT_YET_IMPLEMENTED; // TODO: zx_concat_strings(left, right)
    }

    double lv, rv;
    ZxError err = zx_get_number(*left, &lv);
    if (err != ERR_0_OK) return err;

    err = zx_get_number(*right, &rv);
    if (err != ERR_0_OK) return err;

    switch (operator) {
        case '+': lv += rv; break;
        case '-': lv -= rv; break;
        case '*': lv *= rv; break;
        case '/': lv /= rv; break;
        case 128: lv = pow(lv, rv); break;
        default: return ERR_C_NONSENSE_IN_BASIC;
    }

    return zx_assign_number(lv, left);
}

ZxError parse_expression(ParserContext *ctx, ZxValue *out_value) {
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
ZxError parse_term(ParserContext *ctx, ZxValue *out_value) {
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
ZxError parse_power(ParserContext *ctx, ZxValue *out_value) {
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
ZxError parse_factor(ParserContext *ctx, ZxValue *out_value) {
    if (ctx == NULL || out_value == NULL) return ERR_UNKNOWN;
    ZxError err;
    uint8_t token = ctx->buffer[ctx->cursor];

    ZxValue value;
    zx_init_value(&value);
    ZxValue argument;
    zx_init_value(&argument);

    if (token == get_token_from_key('(', KEYMAP_MODE_LITERAL)) {
        ctx->cursor++;

        err = parse_expression(ctx, &value);
        if (err != ERR_0_OK) goto error_cleanup;

        if (ctx->buffer[ctx->cursor] != get_token_from_key(')', KEYMAP_MODE_LITERAL)) {
            return ERR_C_NONSENSE_IN_BASIC;
        }
        ctx->cursor++;
        *out_value = value;
        zx_free_string(&argument);
        return ERR_0_OK;
    }
    if (token == '+' || token == '-') {
        
        ctx->cursor++;

        err = parse_factor(ctx, &value);
        if (err != ERR_0_OK) goto error_cleanup;

        double value_number;
        err = zx_get_number(value, &value_number);
        if (err != ERR_0_OK) goto error_cleanup;
        value_number = (token == '-') ? -value_number : value_number;
        zx_free_string(&value);
        zx_free_string(&argument);
        return zx_assign_number(value_number, out_value);
    }

    if (is_function(token)) {
        ctx->cursor++;
        ZxValue arg;
        zx_init_value(&arg);
        err = parse_factor(ctx, &arg);
        if (err != ERR_0_OK) goto error_cleanup;
        err = zx_function_call(token, arg, out_value);
        zx_free_string(&arg);
    }
    if (is_zx_number_start_character(token)) {
        size_t bytes_read;
        err = parse_number_to_double(ctx->buffer + ctx->cursor, ctx->size - ctx->cursor, out_value, &bytes_read);
        if (err != ERR_0_OK) goto error_cleanup;
        ctx->cursor += bytes_read;
        return ERR_0_OK;
    }
    if (is_zx_alpha(token)) {
        char variable_name[MAX_VAR_NAME_LEN];
        size_t bytes_read;
        err = parse_variable_name(ctx->buffer + ctx->cursor, ctx->size - ctx->cursor, variable_name, &bytes_read);
        if (err != ERR_0_OK) goto error_cleanup;
        ctx->cursor += bytes_read;
        if (strlen(variable_name) == 2 && variable_name[1] == get_token_from_key('$', KEYMAP_MODE_LITERAL)) {
            int index = name_to_index(get_token_from_key(variable_name[0], KEYMAP_MODE_LITERAL));
            if (index == -1) {
                err = ERR_C_NONSENSE_IN_BASIC;
                goto error_cleanup;
            }
            err = machine_get_string(ctx->machine, index, out_value);
            goto error_cleanup;
        }
        err = machine_get_numeric(ctx->machine, variable_name, out_value);
        goto error_cleanup;
    }
    if (token == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
        size_t bytes_read;
        err = parse_string_literal(ctx->buffer + ctx->cursor, ctx->size - ctx->cursor, out_value, &bytes_read);
        if (err != ERR_0_OK) goto error_cleanup;
        ctx->cursor += bytes_read;
        return ERR_0_OK;
    }

    error_cleanup:
        zx_free_string(&value);
        zx_free_string(&argument);
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

    ZxError err = parse_expression(&ctx, result);
    if (err != ERR_0_OK) return err;
    *bytes_read = ctx.cursor;
    return ERR_0_OK;
}

