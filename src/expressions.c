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



static ZxError parse_expression(ParserContext *ctx, ZxValue *out_value);
static ZxError parse_logical_and(ParserContext *ctx, ZxValue *out_value);
static ZxError parse_logical_not(ParserContext *ctx, ZxValue *out_value);
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
        case ZX_OP_PLUS: lv += rv; break;
        case ZX_OP_MINUS: lv -= rv; break;
        case ZX_OP_MULTIPLY: lv *= rv; break;
        case ZX_OP_DIVIDE: lv /= rv; break;
        case ZX_OP_POWER: lv = pow(lv, rv); break;
        case ZX_OP_LESS: lv = (lv < rv) ? ZX_TRUE : ZX_FALSE; break;
        case ZX_OP_EQUAL: lv = (lv == rv) ? ZX_TRUE : ZX_FALSE; break;
        case ZX_OP_GREATER: lv = (lv > rv) ? ZX_TRUE : ZX_FALSE; break;
        case ZX_OP_LESS_EQ: lv = (lv <= rv) ? ZX_TRUE : ZX_FALSE; break;
        case ZX_OP_GTR_EQ: lv = (lv >= rv) ? ZX_TRUE : ZX_FALSE; break;
        case ZX_OP_NOT_EQ: lv = (lv != rv) ? ZX_TRUE : ZX_FALSE; break;
        case ZX_OP_AND: lv = (rv == ZX_FALSE) ? ZX_FALSE : lv; break;
        case ZX_OP_OR: lv = (rv == ZX_FALSE) ? lv : ZX_TRUE; break;
        case ZX_OP_NOT: lv = (lv == ZX_FALSE) ? ZX_TRUE : ZX_FALSE; break;

        default: return ERR_C_NONSENSE_IN_BASIC;
    }

    return zx_assign_number(lv, left);

}
static void zx_skip_spaces(ParserContext *ctx) {
    while (ctx->cursor < ctx->size && is_zx_space(ctx->buffer[ctx->cursor])) {
        ctx->cursor++;
    }
}
static ZxError parse_array_index_string(ParserContext *ctx, uint16_t *indices, uint8_t *num_indices_passed, int32_t *desired_len) {
    if (ctx == NULL || indices == NULL || num_indices_passed == NULL || desired_len == NULL) {
        return ERR_UNKNOWN;
    }

    ZxError err;
    size_t index_counter = 0;
    *desired_len = SLICE_NO_TO;

    while (ctx->cursor < ctx->size) {
        zx_skip_spaces(ctx);

        // =========================================================================
        // KRONKEL 1: Directe 'TO' bij de start van een dimensie (bijv. A$( TO 5) of A$(2, TO 5))
        // =========================================================================
        if (ctx->buffer[ctx->cursor] == ZX_TOKEN_TO) {
            uint16_t begin = 1; // Slicen zonder startwaarde begint altijd op karakter 1
            if (index_counter > 10) return ERR_3_SUBSCRIPT_WRONG;
            indices[index_counter++] = begin;

            ctx->cursor++; // Skip 'TO'
            zx_skip_spaces(ctx);

            // Als er nóg een expressie achter TO staat (bijv. TO 5)
            if (ctx->cursor < ctx->size && ctx->buffer[ctx->cursor] != get_token_from_key(')', KEYMAP_MODE_LITERAL)) {
                ZxValue eind;
                zx_init_value(&eind);
                err = parse_expression(ctx, &eind);
                if (err != ERR_0_OK) { zx_free_string(&eind); return err; }

                double tmp_eind;
                err = zx_get_number(eind, &tmp_eind);
                zx_free_string(&eind);
                if (err != ERR_0_OK) return err;

                // Sinclair-beveiliging: als het einde vóór het begin ligt, is de lengte gewoon 0
                *desired_len = (tmp_eind < begin) ? 0 : ((int32_t)tmp_eind - begin + 1);
            } else {
                *desired_len = SLICE_OPEN_TO; // Geval: A$(4 TO ) -> Slicen tot het absolute einde
            }

            *num_indices_passed = index_counter;
            return ERR_0_OK; // Slicing is ALTIJD de afsluiter van een string-opvraging!
        }

        // =========================================================================
        // NORMALE INDEX VERWERKING (Los de numerieke expressie op)
        // =========================================================================
        ZxValue index_val;
        zx_init_value(&index_val);
        err = parse_expression(ctx, &index_val);
        if (err != ERR_0_OK) { zx_free_string(&index_val); return err; }

        double index_num;
        err = zx_get_number(index_val, &index_num);
        zx_free_string(&index_val);
        if (err != ERR_0_OK) return err;

        uint16_t parsed_idx = (uint16_t)index_num;
        if (index_counter > 10) return ERR_3_SUBSCRIPT_WRONG;
        indices[index_counter++] = parsed_idx;

        zx_skip_spaces(ctx);
        if (ctx->cursor >= ctx->size) return ERR_C_NONSENSE_IN_BASIC;

        // =========================================================================
        // DE VALSTRIK-KLEP: Inspecteer direct de syntactische opvolger
        // =========================================================================
        uint8_t next_token = ctx->buffer[ctx->cursor];

        if (next_token == get_token_from_key(',', KEYMAP_MODE_LITERAL)) {
            ctx->cursor++; // Skip de komma en ga vrolijk door naar de volgende dimensie
            continue;
        }

        if (next_token == ZX_TOKEN_TO) {
            ctx->cursor++; // Skip 'TO'
            zx_skip_spaces(ctx);

            // Als er een eindwaarde is opgegeven (bijv. 4 TO 6)
            if (ctx->cursor < ctx->size && ctx->buffer[ctx->cursor] != get_token_from_key(')', KEYMAP_MODE_LITERAL)) {
                ZxValue eind;
                zx_init_value(&eind);
                err = parse_expression(ctx, &eind);
                if (err != ERR_0_OK) { zx_free_string(&eind); return err; }

                double tmp_eind;
                err = zx_get_number(eind, &tmp_eind);
                zx_free_string(&eind);
                if (err != ERR_0_OK) return err;

                *desired_len = (tmp_eind < parsed_idx) ? 0 : ((int32_t)tmp_eind - parsed_idx + 1);
            } else {
                *desired_len = SLICE_OPEN_TO; // Geval: A$(2, 4 TO )
            }

            *num_indices_passed = index_counter;
            return ERR_0_OK; // Klaar!
        }

        if (next_token == get_token_from_key(')', KEYMAP_MODE_LITERAL)) {
            // We zijn op de sluitende haak gestuit zonder slicer (bijv. A$(2, 3) of A$(2))
            *num_indices_passed = index_counter;
            return ERR_0_OK; // parse_factor consumeert de ')' straks netjes
        }

        // Als er na een expressie géén komma, TO of sluithaak staat, is het pure wartaal!
        return ERR_C_NONSENSE_IN_BASIC;
    }

    return ERR_C_NONSENSE_IN_BASIC;
}
static ZxError parse_array_index_numeric(ParserContext *ctx, uint16_t *indices, uint8_t *num_indices_passed) {
    if (ctx == NULL || indices == NULL || num_indices_passed == NULL) {
        return ERR_UNKNOWN;
    }

    ZxError err;
    size_t index_counter = 0;

    while (ctx->cursor < ctx->size) {
        zx_skip_spaces(ctx);

        // =========================================================================
        // NORMALE INDEX VERWERKING (Los de numerieke expressie op)
        // =========================================================================
        ZxValue index_val;
        zx_init_value(&index_val);
        err = parse_expression(ctx, &index_val);
        if (err != ERR_0_OK) { zx_free_string(&index_val); return err; }

        double index_num;
        err = zx_get_number(index_val, &index_num);
        zx_free_string(&index_val);
        if (err != ERR_0_OK) return err;

        uint16_t parsed_idx = (uint16_t)index_num;
        if (index_counter > 10) return ERR_3_SUBSCRIPT_WRONG;
        indices[index_counter++] = parsed_idx;

        zx_skip_spaces(ctx);
        if (ctx->cursor >= ctx->size) return ERR_C_NONSENSE_IN_BASIC;

        // =========================================================================
        // DE VALSTRIK-KLEP: Inspecteer direct de syntactische opvolger
        // =========================================================================
        uint8_t next_token = ctx->buffer[ctx->cursor];

        if (next_token == get_token_from_key(',', KEYMAP_MODE_LITERAL)) {
            ctx->cursor++; // Skip de komma en ga vrolijk door naar de volgende dimensie
            continue;
        }

        if (next_token == get_token_from_key(')', KEYMAP_MODE_LITERAL)) {

            *num_indices_passed = index_counter;
            return ERR_0_OK; // parse_factor consumeert de ')' straks netjes
        }

        // Als er na een expressie géén komma of sluithaak staat, is het pure wartaal!
        return ERR_C_NONSENSE_IN_BASIC;
    }

    return ERR_C_NONSENSE_IN_BASIC;
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

    err = parse_logical_not(ctx, &left_value);
    if (err != ERR_0_OK) goto error_cleanup;

    while (ctx->buffer[ctx->cursor] == ZX_OP_AND) {

        uint8_t operator = ctx->buffer[ctx->cursor];
        ctx->cursor++;
        zx_skip_spaces(ctx);

        err = parse_logical_not(ctx, &right_value);
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
static ZxError parse_logical_not(ParserContext *ctx, ZxValue *out_value) {
    if (ctx == NULL || out_value == NULL) return ERR_UNKNOWN;
    ZxError err;

    if (ctx->buffer[ctx->cursor] == ZX_OP_NOT) {
        ctx->cursor++;
        zx_skip_spaces(ctx);

        ZxValue operand_value;
        zx_init_value(&operand_value);

        err = parse_logical_not(ctx, &operand_value);
        if (err != ERR_0_OK) {
            zx_free_string(&operand_value);
            return err;
        }

        ZxValue dummy_right;
        zx_init_value(&dummy_right);
        err = zx_assign_number(0, &dummy_right);
        if (err != ERR_0_OK) {
            zx_free_string(&operand_value);
            zx_free_string(&dummy_right);
            return err;
        }
        err = zx_calculate(ZX_OP_NOT, &operand_value, &dummy_right);
        zx_free_string(&dummy_right);
        if (err != ERR_0_OK) {
            zx_free_string(&operand_value);
            zx_free_string(&dummy_right);
            return err;
        }

        *out_value = operand_value;
        return ERR_0_OK;
    }

    // 3. Staat er geen NOT? Dan stromen we plichtsgetrouw door
    // naar de hogere prioriteit: de relationele operatoren (=, <, >, etc.)
    return parse_relational(ctx, out_value);
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
    if (ctx->buffer[ctx->cursor] == ZX_TOKEN_BIN) {
        ctx->cursor++;
        zx_skip_spaces(ctx);

        uint32_t total_digits = 0;
        uint32_t bin_value = 0;
        uint32_t significant_bits = 0;
        bool started_significant = false;

        while (ctx->buffer[ctx->cursor] == get_token_from_key('0', KEYMAP_MODE_LITERAL) ||
            ctx->buffer[ctx->cursor] == get_token_from_key('1', KEYMAP_MODE_LITERAL)) {
            char bit = ctx->buffer[ctx->cursor];
            ctx->cursor++;
            total_digits++;

            if (bit == get_token_from_key('1', KEYMAP_MODE_LITERAL)) {
                started_significant = true;
            }

            if (started_significant) {
                significant_bits++;
            }

            bin_value = (bin_value << 1) | (bit - '0');
        }

        if (total_digits == 0) {
            return ERR_C_NONSENSE_IN_BASIC;
        }

        if (significant_bits > 16 || bin_value > 65535) {
            return ERR_6_NUMBER_TOO_BIG;
        }

        return zx_assign_number((double)bin_value, out_value);
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
        ZxError err;
        size_t bytes_read;
        char variable_name[MAX_VAR_NAME_LEN];
        uint16_t indices[10] = {0};
        uint8_t num_indices_passed = 0;
        int32_t desired_len = 0;

        err = zx_parse_variable_reference(ctx->machine,
            ctx->buffer + ctx->cursor,
            ctx->size - ctx->cursor,
            &bytes_read,
            variable_name,
            indices,
            &num_indices_passed,
            &desired_len);
        if (err != ERR_0_OK) goto error_cleanup;

        ctx->cursor += bytes_read;
        zx_skip_spaces(ctx);

        return machine_get_variable(ctx->machine, variable_name, indices, num_indices_passed, desired_len, out_value);
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
ZxError zx_parse_variable_reference(ZxMachine machine, const uint8_t *buffer, size_t size, size_t *bytes_read, char *out_var_name, uint16_t *out_indices, uint8_t *out_num_indices, int32_t *out_desired_len) {
    if (buffer == NULL || bytes_read == NULL || out_var_name == NULL) return ERR_UNKNOWN;

    // Sla de interne brug: maak een tijdelijke context aan voor de interne lussen
    ParserContext ctx;
    ctx.machine = machine;
    ctx.buffer = buffer;
    ctx.size = size;
    ctx.cursor = 0;

    // 1. Parse de naam (gebruik je bestaande interne functie)
    size_t name_bytes;
    ZxError err = parse_variable_name(ctx.buffer, ctx.size, out_var_name, &name_bytes);
    if (err != ERR_0_OK) return err;
    ctx.cursor += name_bytes;
    zx_skip_spaces(&ctx);

    // 2. Indien van toepassing: parse de indices via de interne helpers
    if (strlen(out_var_name) == 2 && out_var_name[1] == get_token_from_key('$', KEYMAP_MODE_LITERAL)) {
        // String of String-array
        if (ctx.cursor < ctx.size && ctx.buffer[ctx.cursor] == get_token_from_key('(', KEYMAP_MODE_LITERAL)) {
            ctx.cursor++; // Skip '('
            zx_skip_spaces(&ctx);

            err = parse_array_index_string(&ctx, out_indices, out_num_indices, out_desired_len);
            if (err != ERR_0_OK) return err;

            if (ctx.cursor >= ctx.size || ctx.buffer[ctx.cursor] != get_token_from_key(')', KEYMAP_MODE_LITERAL)) {
                return ERR_C_NONSENSE_IN_BASIC;
            }
            ctx.cursor++; // Skip ')'
        }
    } else if (strlen(out_var_name) == 1) {
        // Numerieke array
        if (ctx.cursor < ctx.size && ctx.buffer[ctx.cursor] == get_token_from_key('(', KEYMAP_MODE_LITERAL)) {
            ctx.cursor++; // Skip '('
            zx_skip_spaces(&ctx);

            err = parse_array_index_numeric(&ctx, out_indices, out_num_indices);
            if (err != ERR_0_OK) return err;

            if (ctx.cursor >= ctx.size || ctx.buffer[ctx.cursor] != get_token_from_key(')', KEYMAP_MODE_LITERAL)) {
                return ERR_C_NONSENSE_IN_BASIC;
            }
            ctx.cursor++; // Skip ')'
        }
    }

    // Vertel de beller exact hoever we in het rauwe buffer zijn doorgeschoven!
    *bytes_read = ctx.cursor;
    return ERR_0_OK;
}
