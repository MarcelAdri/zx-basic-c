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
    ZxError last_error;
} ParserContext;

double parse_expression(ParserContext *ctx);
double parse_term(ParserContext *ctx);
double parse_power(ParserContext *ctx);
double parse_factor(ParserContext *ctx);

double parse_expression(ParserContext *ctx) {
    double left_value = parse_term(ctx);
    if (ctx->last_error != ERR_0_OK) {
        return 0;
    }
    while (is_zx_plus_character(ctx->buffer[ctx->cursor]) ||
        is_zx_minus_character(ctx->buffer[ctx->cursor])) {
        char operator = ctx->buffer[ctx->cursor];
        ctx->cursor++;
        double right_value = parse_term(ctx);
        if (ctx->last_error != ERR_0_OK) {
            return 0;
        }
        if (is_zx_plus_character(operator)) {
            left_value += right_value;
        }
        if (is_zx_minus_character(operator)) {
            left_value -= right_value;
        }
    }
    ctx->last_error = ERR_0_OK;
    return left_value;
}
double parse_term(ParserContext *ctx) {
    double left_value = parse_power(ctx);
    if (ctx->last_error != ERR_0_OK) {
        return 0;
    }
    while (is_zx_asterisk_character(ctx->buffer[ctx->cursor]) ||
        is_zx_slash_character(ctx->buffer[ctx->cursor])) {
        char operator = ctx->buffer[ctx->cursor];
        ctx->cursor++;
        double right_value = parse_power(ctx);
        if (ctx->last_error != ERR_0_OK) {
            return 0;
        }
        if (is_zx_asterisk_character(operator)) {
            left_value *= right_value;
        }
        if (is_zx_slash_character(operator)) {
            left_value /= right_value;
        }
    }
    ctx->last_error = ERR_0_OK;
    return left_value;
}
double parse_power(ParserContext *ctx) {
    double left_value = parse_factor(ctx);
    if (ctx->last_error != ERR_0_OK) {
        return 0;
    }
    while (is_zx_power_character(ctx->buffer[ctx->cursor])) {
        char operator = ctx->buffer[ctx->cursor];
        ctx->cursor++;
        double right_value = parse_factor(ctx);
        if (ctx->last_error != ERR_0_OK) {
            return 0;
        }
        if (is_zx_power_character(operator)) {
            left_value = pow(left_value, right_value);
            ctx->last_error = ERR_0_OK;
            return left_value;
        }

    }
    ctx->last_error = ERR_0_OK;
    return left_value;
}
double parse_factor(ParserContext *ctx) {
    uint8_t token = ctx->buffer[ctx->cursor];
    ZxError err;

    if (token == get_token_from_key('(', KEYMAP_MODE_LITERAL)) {
        ctx->cursor++;
        double value = parse_expression(ctx);
        if (ctx->last_error != ERR_0_OK) {
            return 0;
        }
        if (ctx->buffer[ctx->cursor] != get_token_from_key(')', KEYMAP_MODE_LITERAL)) {
            ctx->last_error = ERR_C_NONSENSE_IN_BASIC;
            return 0;
        }
        ctx->cursor++;
        ctx->last_error = ERR_0_OK;
        return value;
    }
    if (token == '+' || token == '-') {
        
        ctx->cursor++; // Sla het plus- of minteken over

        double value = parse_factor(ctx);

        if (ctx->last_error != ERR_0_OK) {
            return 0;
        }

        ctx->last_error = ERR_0_OK;

        return (token == '-') ? -value : value;
    }

    if (is_num_function_num_arg(token)) {
        ctx->cursor++;
        double argument = parse_factor(ctx);
        if (ctx->last_error != ERR_0_OK) {
            return 0;
        }
        ZxValue arg;
        zx_init_value(&arg);
        err = zx_assign_number(argument, &arg);
        if (err != ERR_0_OK) {
            zx_free_string(&arg);
            ctx->last_error = err;
            return 0;
        }
        ZxValue zx_value;
        zx_init_value(&zx_value);
        err = zx_function_call(token, arg, &zx_value);
        if (err != ERR_0_OK) {
            zx_free_string(&arg);
            zx_free_string(&zx_value);
            ctx->last_error = err;
            return 0;
        }
        double value;
        err = zx_get_number(zx_value, &value);
        if (err != ERR_0_OK) {
            zx_free_string(&arg);
            zx_free_string(&zx_value);
            ctx->last_error = err;
            return 0;
        }
        zx_free_string(&arg);
        zx_free_string(&zx_value);
        ctx->last_error = ERR_0_OK;
        return value;
    }
    if (is_num_function_no_arg(token)) {
        ctx->cursor++;
        ZxValue zx_value;
        zx_init_value(&zx_value);
        ZxValue arg;
        zx_init_value(&arg);
        err = zx_function_call(token, arg, &zx_value);
        if (err != ERR_0_OK) {
            zx_free_string(&arg);
            zx_free_string(&zx_value);
            ctx->last_error = err;
            return 0;
        }
        double value;
        err = zx_get_number(zx_value, &value);
        if (err != ERR_0_OK) {
            zx_free_string(&arg);
            zx_free_string(&zx_value);
            ctx->last_error = err;
            return 0;
        }
        zx_free_string(&arg);
        zx_free_string(&zx_value);
        ctx->last_error = ERR_0_OK;
        return value;
    }
    if (is_num_function_str_arg(token)) {
        ctx->cursor++;
        ZxValue arg;
        zx_init_value(&arg);
        err = zx_assign_string(ctx->buffer + ctx->cursor, ctx->size - ctx->cursor, &arg);
        if (err != ERR_0_OK) {
            ctx->last_error = err;
            zx_free_string(&arg);
            return 0;
        }
        ZxValue zx_value;
        zx_init_value(&zx_value);
        err = zx_function_call(token, arg, &zx_value);
        if (err != ERR_0_OK) {
            zx_free_string(&arg);
            zx_free_string(&zx_value);
            ctx->last_error = err;
            return 0;
        }
        double value;
        err = zx_get_number(zx_value, &value);
        if (err != ERR_0_OK) {
            zx_free_string(&arg);
            zx_free_string(&zx_value);
            ctx->last_error = err;
            return 0;
        }
        zx_free_string(&arg);
        zx_free_string(&zx_value);
        ctx->last_error = ERR_0_OK;
        return value;
    }
    if (is_zx_number_start_character(token)) {
        double value;
        size_t bytes_read;
        err = parse_number_to_double(ctx->buffer + ctx->cursor, ctx->size - ctx->cursor, &value, 32, &bytes_read);
        if (err != ERR_0_OK) {
            ctx->last_error = err;
            return 0;
        }
        ctx->cursor += bytes_read;
        ctx->last_error = ERR_0_OK;
        return value;
    }
    if (is_zx_alpha(token)) {
        double value;
        ZxValue zx_value;
        zx_init_value(&zx_value);
        char variable_name[MAX_VAR_NAME_LEN];
        size_t bytes_read;
        err = parse_variable_name(ctx->buffer + ctx->cursor, ctx->size - ctx->cursor, variable_name, &bytes_read);
        if (err != ERR_0_OK) {
            zx_free_string(&zx_value);
            ctx->last_error = err;
            return 0;
        }
        ctx->cursor += bytes_read;
        err = machine_get_numeric(ctx->machine, variable_name, &zx_value);
        if (err != ERR_0_OK) {
            zx_free_string(&zx_value);
            ctx->last_error = err;
            return 0;
        }
        err = zx_get_number(zx_value, &value);
        if (err != ERR_0_OK) {
            zx_free_string(&zx_value);
            ctx->last_error = err;
            return 0;
        }
        zx_free_string(&zx_value);
        ctx->last_error = ERR_0_OK;
        return value;
    }
    ctx->last_error = ERR_C_NONSENSE_IN_BASIC;
    return 0;
}
ZxError solve_expression(ZxMachine machine, const uint8_t *expression, size_t expression_size, ZxValue *result, size_t *bytes_read) {
    // Veiligheidscheck (altijd goed om bovenaan te hebben)
    if (expression == NULL || result == NULL) {
        return ERR_UNKNOWN;
    }

    ZxError err;
    ParserContext ctx = {0};
    ctx.machine = machine;
    ctx.buffer = expression;
    ctx.size = expression_size;
    ctx.cursor = 0;

    // 1. Skip spaties
    size_t i = 0;
    while (i < expression_size && is_zx_space(expression[i])) {
        i++;
    }

    // Is het einde van de expressie al bereikt?
    if (i >= expression_size) {
        return ERR_C_NONSENSE_IN_BASIC;
    }

    // 2. String literals
    if (expression[i] == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
        size_t string_bytes_read = 0;

        // LET OP: expression + i doorgeven!
        err = parse_string_literal(expression + i, expression_size - i, result, &string_bytes_read);
        if (err != ERR_0_OK) {
            return err;
        }

        if (bytes_read != NULL) {
            // Tel de overgeslagen spaties op bij de gelezen string bytes
            *bytes_read = i + string_bytes_read;
        }
        return ERR_0_OK;
    }

    // 3. String variabelen
    if (is_zx_alpha(expression[i])) {

        if (i + 1 < expression_size && expression[i + 1] == get_token_from_key('$', KEYMAP_MODE_LITERAL)) {
            return ERR_NOT_YET_IMPLEMENTED;
        }
    }

    // 4. Fallback: Het moet een wiskundige expressie of numerieke variabele zijn
    double value = parse_expression(&ctx);
    if (ctx.last_error != ERR_0_OK) {
        return ctx.last_error;
    }

    if (bytes_read != NULL) {
        // parse_expression is vanaf 0 begonnen, dus ctx.cursor klopt precies voor de originele expression pointer
        *bytes_read = ctx.cursor;
    }

    return zx_assign_number(value, result);
}

