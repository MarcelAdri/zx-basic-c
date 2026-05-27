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
    if (ctx->last_error != ERR_OK) {
        return 0;
    }
    while (is_zx_plus_character(ctx->buffer[ctx->cursor]) ||
        is_zx_minus_character(ctx->buffer[ctx->cursor])) {
        char operator = ctx->buffer[ctx->cursor];
        ctx->cursor++;
        double right_value = parse_term(ctx);
        if (ctx->last_error != ERR_OK) {
            return 0;
        }
        if (is_zx_plus_character(operator)) {
            left_value += right_value;
        }
        if (is_zx_minus_character(operator)) {
            left_value -= right_value;
        }
    }
    ctx->last_error = ERR_OK;
    return left_value;
}
double parse_term(ParserContext *ctx) {
    double left_value = parse_power(ctx);
    if (ctx->last_error != ERR_OK) {
        return 0;
    }
    while (is_zx_asterisk_character(ctx->buffer[ctx->cursor]) ||
        is_zx_slash_character(ctx->buffer[ctx->cursor])) {
        char operator = ctx->buffer[ctx->cursor];
        ctx->cursor++;
        double right_value = parse_power(ctx);
        if (ctx->last_error != ERR_OK) {
            return 0;
        }
        if (is_zx_asterisk_character(operator)) {
            left_value *= right_value;
        }
        if (is_zx_slash_character(operator)) {
            if (right_value == 0) {
                ctx->last_error = ERR_DIVISION_BY_ZERO;
                return 0;
            }
            left_value /= right_value;
        }
    }
    ctx->last_error = ERR_OK;
    return left_value;
}
double parse_power(ParserContext *ctx) {
    double left_value = parse_factor(ctx);
    if (ctx->last_error != ERR_OK) {
        return 0;
    }
    while (is_zx_power_character(ctx->buffer[ctx->cursor])) {
        char operator = ctx->buffer[ctx->cursor];
        ctx->cursor++;
        double right_value = parse_factor(ctx);
        if (ctx->last_error != ERR_OK) {
            return 0;
        }
        if (is_zx_power_character(operator)) {
            if (left_value == 0.0 && right_value < 0.0) {
                ctx->last_error = ERR_DIVISION_BY_ZERO;
                return 0; // Stop direct
            }
            if (left_value < 0 && floor(right_value) != right_value) {
                ctx->last_error = ERR_INVALID_ARGUMENT; // Wiskundige fout
                return 0;
            }
            left_value = pow(left_value, right_value);
            if (isinf(left_value)) {
                ctx->last_error = ERR_OUT_OF_RANGE;
                return 0;
            }
            if (isnan(left_value)) {
                ctx->last_error = ERR_INVALID_ARGUMENT;
                return 0;
            }
            ctx->last_error = ERR_OK;
            return left_value;
        }

    }
    ctx->last_error = ERR_OK;
    return left_value;
}
double parse_factor(ParserContext *ctx) {
    uint8_t token = ctx->buffer[ctx->cursor];
    ZxError err;

    if (token == get_token_from_key('(', KEYMAP_MODE_LITERAL)) {
        ctx->cursor++;
        double value = parse_expression(ctx);
        if (ctx->last_error != ERR_OK) {
            return 0;
        }
        if (ctx->buffer[ctx->cursor] != get_token_from_key(')', KEYMAP_MODE_LITERAL)) {
            ctx->last_error = ERR_UNCLOSED_BRACKETS;
            return 0;
        }
        ctx->cursor++;
        ctx->last_error = ERR_OK;
        return value;
    }
    if (token == '+' || token == '-') {
        
        ctx->cursor++; // Sla het plus- of minteken over

        double value = parse_factor(ctx);

        if (ctx->last_error != ERR_OK) {
            return 0;
        }

        ctx->last_error = ERR_OK;

        return (token == '-') ? -value : value;
    }

    if (is_num_function_num_arg(token)) {
        ctx->cursor++;
        double argument = parse_expression(ctx);
        if (ctx->last_error != ERR_OK) {
            return 0;
        }
        double value;
        err = zx_num_function_call(token, argument, NULL, &value);
        if (err != ERR_OK) {
            ctx->last_error = err;
            return 0;
        }
        ctx->last_error = ERR_OK;
        return value;
    }
    if (is_num_function_no_arg(token)) {
        ctx->cursor++;
        double value;
        err = zx_num_function_call(token, 0, NULL, &value);
        if (err != ERR_OK) {
            ctx->last_error = err;
            return 0;
        }
        ctx->last_error = ERR_OK;
        return value;
    }
    if (is_num_function_str_arg(token)) {
        ctx->cursor++;
        char string_arg[2048];
        err = parse_string_literal_with_quotes(ctx->buffer + ctx->cursor, ctx->size - ctx->cursor, string_arg, 2048);
        if (err != ERR_OK) {
            ctx->last_error = err;
            return 0;
        }
        double value;
        err = zx_num_function_call(token, 0, string_arg, &value);
        if (err != ERR_OK) {
            ctx->last_error = err;
            return 0;
        }
        ctx->last_error = ERR_OK;
        return value;
    }
    if (is_zx_number_start_character(token)) {
        double value;
        size_t bytes_read;
        err = parse_number_to_double(ctx->buffer + ctx->cursor, ctx->size - ctx->cursor, &value, 32, &bytes_read);
        if (err != ERR_OK) {
            ctx->last_error = err;
            return 0;
        }
        ctx->cursor += bytes_read;
        ctx->last_error = ERR_OK;
        return value;
    }
    if (is_zx_alpha(token)) {
        double value;
        char variable_name[MAX_VAR_NAME_LEN];
        size_t bytes_read;
        err = parse_variable_name(ctx->buffer + ctx->cursor, ctx->size - ctx->cursor, variable_name, &bytes_read);
        if (err != ERR_OK) {
            ctx->last_error = err;
            return 0;
        }
        ctx->cursor += bytes_read;
        err = machine_get_numeric(ctx->machine, variable_name, &value);
        if (err != ERR_OK) {
            ctx->last_error = err;
            return 0;
        }
        ctx->last_error = ERR_OK;
        return value;
    }
    ctx->last_error = ERR_INVALID_EXPRESSION;
    return 0;
}


ZxError solve_expression_to_double(ZxMachine machine, const uint8_t *expression, size_t expression_size, double *result, const size_t result_size, size_t *bytes_read) {
    ZxError err;
    ParserContext ctx = {0};
    ctx.machine = machine;
    ctx.buffer = expression;
    ctx.size = expression_size;
    ctx.cursor = 0;

    double value = parse_expression(&ctx);
    if (ctx.last_error != ERR_OK) {
        return ctx.last_error;
    }
    if (bytes_read != NULL) {
        *bytes_read = ctx.cursor;
    }
    *result = value;
    return ERR_OK;

}

ZxError solve_expression_to_string(ZxMachine machine, const uint8_t *expression, size_t expression_size, char *result, const size_t result_size, size_t *bytes_read) {
    ZxError err;

    size_t i = 0;
    while (is_zx_space(expression[i]) && i < expression_size) {
        i++;
    }

    if (expression[i] == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
        err = parse_string_literal(expression, expression_size - i, result, result_size, bytes_read);
        if (err != ERR_OK) {
            return err;
        }
        return ERR_OK;
    }
    if (is_zx_alpha(expression[i])) {
        char variable_name[MAX_VAR_NAME_LEN];
        err = parse_variable_name(expression, expression_size, variable_name, bytes_read);
        if (err != ERR_OK) {
            return err;
        }
        double value;
        err = machine_get_numeric(machine, variable_name, &value);
        if (err != ERR_OK) {
            return err;
        }
        char result_string[32];
        err = formatted_number(value, result_string, 32);
        if (err != ERR_OK) {
            return err;
        }
        snprintf(result, result_size, "%s", result_string);
        return ERR_OK;

    }

    double value;
    err = solve_expression_to_double(machine, expression, expression_size, &value, 32, bytes_read);
    if (err != ERR_OK) {
        return err;
    }
    return formatted_number(value, result, result_size);

}

