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
            if (index_counter > 10) return ERR_4_OUT_OF_MEMORY;
            indices[index_counter++] = begin;

            ctx->cursor++; // Skip 'TO'
            zx_skip_spaces(ctx);

            // Als er nóg een expressie achter TO staat (bijv. TO 5)
            if (ctx->cursor < ctx->size && ctx->buffer[ctx->cursor] != ZX_CHAR_BRACKET_CLOSE) {
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

        if (next_token == ZX_CHAR_COMMA) {
            ctx->cursor++; // Skip de komma en ga vrolijk door naar de volgende dimensie
            continue;
        }

        if (next_token == ZX_TOKEN_TO) {
            ctx->cursor++; // Skip 'TO'
            zx_skip_spaces(ctx);

            // Als er een eindwaarde is opgegeven (bijv. 4 TO 6)
            if (ctx->cursor < ctx->size && ctx->buffer[ctx->cursor] != ZX_CHAR_BRACKET_CLOSE) {
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

        if (next_token == ZX_CHAR_BRACKET_CLOSE) {
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
        if (index_counter > 10) return ERR_4_OUT_OF_MEMORY;
        indices[index_counter++] = parsed_idx;

        zx_skip_spaces(ctx);
        if (ctx->cursor >= ctx->size) return ERR_C_NONSENSE_IN_BASIC;

        // =========================================================================
        // DE VALSTRIK-KLEP: Inspecteer direct de syntactische opvolger
        // =========================================================================
        uint8_t next_token = ctx->buffer[ctx->cursor];

        if (next_token == ZX_CHAR_COMMA) {
            ctx->cursor++; // Skip de komma en ga vrolijk door naar de volgende dimensie
            continue;
        }

        if (next_token == ZX_CHAR_BRACKET_CLOSE) {

            *num_indices_passed = index_counter;
            return ERR_0_OK; // parse_factor consumeert de ')' straks netjes
        }

        // Als er na een expressie géén komma of sluithaak staat, is het pure wartaal!
        return ERR_C_NONSENSE_IN_BASIC;
    }
    return ERR_C_NONSENSE_IN_BASIC;
}
static ZxError parse_function_definition(ParserContext *ctx, ZxValue *result_out) {
    if (ctx == NULL || result_out == NULL) return ERR_UNKNOWN;

    // 1. Lees de functienaam bij de CALL-site (bijv. "a" of "a$")
    size_t bytes_read = 0;
    char var_name[MAX_VAR_NAME_LEN] = {0};
    ZxError err = parse_variable_name(ctx->buffer + ctx->cursor, ctx->size - ctx->cursor, var_name, &bytes_read);
    if (err != ERR_0_OK) return err;

    size_t name_len = strlen(var_name);
    if (name_len < 1 || name_len > 2) {
        return ERR_C_NONSENSE_IN_BASIC; // Meteen afkeuren als de naam te lang is!
    }
    if (name_len == 2 && var_name[1] != ZX_CHAR_DOLLAR) {
        return ERR_C_NONSENSE_IN_BASIC; // 2 tekens mag uitsluitend als het tweede teken een '$' is
    }

    ctx->cursor += bytes_read;
    zx_skip_spaces(ctx);
    if (ctx->cursor >= ctx->size) return ERR_C_NONSENSE_IN_BASIC;

    // 2. KOGELVRIJE WALKTHROUGH: Zoek DEF FN op in het geheugen
    uint16_t search_line = 1;
    bool def_found = false;
    const uint8_t *def_chunk = NULL;
    size_t def_chunk_size = 0;
    size_t def_cursor = 0;

    while (search_line < 10000) {
        size_t line_size = 0;
        uint16_t actual_line = search_line;
        const uint8_t *line_buf = machine_retrieve_program_line(ctx->machine, &actual_line, &line_size);
        if (line_buf == NULL) break;

        search_line = actual_line;
        uint8_t stmt_idx = 1; // Statements beginnen bij 1

        while (true) {
            const uint8_t *chunk = NULL;
            size_t chunk_sz = 0;
            extract_statement(line_buf, line_size, stmt_idx, &chunk, &chunk_sz);
            if (chunk == NULL || chunk_sz == 0) break; // Einde van deze regel

            if (chunk[0] == ZX_STATEMENT_DEF_FN) {
                size_t ptr = 1;
                while (ptr < chunk_sz && is_zx_space(chunk[ptr])) ptr++;

                char fun_name[MAX_VAR_NAME_LEN] = {0};
                size_t name_bytes = 0;
                if (parse_variable_name(chunk + ptr, chunk_sz - ptr, fun_name, &name_bytes) == ERR_0_OK) {
                    if (strcmp(var_name, fun_name) == 0) {
                        def_found = true;
                        def_chunk = chunk;
                        def_chunk_size = chunk_sz;
                        def_cursor = ptr + name_bytes;
                        break;
                    }
                }
            }
            stmt_idx++;
        }

        if (def_found) break;
        search_line++;
    }

    if (!def_found) return ERR_P_FN_WITHOUT_DEF;

    // 3. PARSE PARAMETERS IN DE DEFINITIE (bijv. "(x, y$)")
    char def_arg[52][3] = {0};
    size_t def_num_args = 0;

    while (def_cursor < def_chunk_size && is_zx_space(def_chunk[def_cursor])) def_cursor++;
    if (def_cursor >= def_chunk_size || def_chunk[def_cursor] != ZX_CHAR_BRACKET_OPEN) {
        return ERR_C_NONSENSE_IN_BASIC;
    }
    def_cursor++; // Skip '('

    while (def_cursor < def_chunk_size && def_chunk[def_cursor] != ZX_CHAR_BRACKET_CLOSE) {
        while (def_cursor < def_chunk_size && is_zx_space(def_chunk[def_cursor])) def_cursor++;
        if (def_cursor >= def_chunk_size) return ERR_C_NONSENSE_IN_BASIC;

        char param_name[MAX_VAR_NAME_LEN] = {0};
        size_t param_bytes = 0;
        err = parse_variable_name(def_chunk + def_cursor, def_chunk_size - def_cursor, param_name, &param_bytes);
        if (err != ERR_0_OK) return err;

        strncpy(def_arg[def_num_args], param_name, 2);
        def_num_args++;
        def_cursor += param_bytes;

        while (def_cursor < def_chunk_size && is_zx_space(def_chunk[def_cursor])) def_cursor++;
        if (def_cursor < def_chunk_size && def_chunk[def_cursor] == ZX_CHAR_COMMA) {
            def_cursor++; // Skip ','
        }
    }

    if (def_cursor >= def_chunk_size || def_chunk[def_cursor] != ZX_CHAR_BRACKET_CLOSE) {
        return ERR_C_NONSENSE_IN_BASIC;
    }
    def_cursor++; // Skip ')'

    // 4. PARSE ARGUMENTEN BIJ DE CALL-SITE (bijv. "(10, A$)")
    if (ctx->buffer[ctx->cursor] != ZX_CHAR_BRACKET_OPEN) {
        return ERR_C_NONSENSE_IN_BASIC;
    }
    ctx->cursor++; // Skip '('

    ZxValue call_args[52];
    ZxValue old_values[52];
    bool existed[52];
    for (size_t i = 0; i < 52; i++) {
        zx_init_value(&call_args[i]);
        zx_init_value(&old_values[i]);
    }

    size_t call_args_size = 0;

    while (ctx->cursor < ctx->size && ctx->buffer[ctx->cursor] != ZX_CHAR_BRACKET_CLOSE) {
        zx_skip_spaces(ctx);

        // Gebruik de reguliere expressie-parser voor elk argument
        err = parse_expression(ctx, &call_args[call_args_size]);
        if (err != ERR_0_OK) goto error_cleanup;

        call_args_size++;

        zx_skip_spaces(ctx);
        if (ctx->cursor < ctx->size && ctx->buffer[ctx->cursor] == ZX_CHAR_COMMA) {
            ctx->cursor++; // Skip ','
        }
    }

    if (ctx->cursor >= ctx->size || ctx->buffer[ctx->cursor] != ZX_CHAR_BRACKET_CLOSE) {
        err = ERR_C_NONSENSE_IN_BASIC;
        goto error_cleanup;
    }
    ctx->cursor++; // Skip ')'

    // 5. VALIDEER AANTAL EN TYPES VAN PARAMETERS
    if (def_num_args != call_args_size) {
        err = ERR_Q_PARAMETER_ERROR;
        goto error_cleanup;
    }

    for (size_t i = 0; i < call_args_size; i++) {
        if ((strlen(def_arg[i]) == 1 && call_args[i].type != ZX_TYPE_NUMBER) ||
            (strlen(def_arg[i]) == 2 && call_args[i].type != ZX_TYPE_STRING)) {
            err = ERR_Q_PARAMETER_ERROR;
            goto error_cleanup;
        }
    }

    // 6. VARIABLE SHADOWING (Sla oude waarden op & injecteer parameters)
    for (size_t i = 0; i < call_args_size; i++) {
        if (machine_get_variable(ctx->machine, def_arg[i], NULL, 0, 0, &old_values[i]) == ERR_0_OK) {
            existed[i] = true;
        } else {
            existed[i] = false;
        }
        machine_set_variable(ctx->machine, def_arg[i], NULL, 0, 0, call_args[i]);
    }

    // 7. EVALUEER DE FUNCTIE-BODY (Zoek '=' in def_chunk)
    while (def_cursor < def_chunk_size && def_chunk[def_cursor] != ZX_OP_EQUAL) {
        def_cursor++;
    }
    if (def_cursor >= def_chunk_size) {
        err = ERR_C_NONSENSE_IN_BASIC;
        goto restore_and_cleanup;
    }
    def_cursor++; // Skip '='

    size_t dummy_bytes = 0;
    err = solve_expression(ctx->machine, def_chunk + def_cursor, def_chunk_size - def_cursor, result_out, &dummy_bytes);

restore_and_cleanup:
    // 8. HERSTEL OUDE VARIABELE-TOESTAND
    for (size_t i = 0; i < call_args_size; i++) {
        if (existed[i]) {
            machine_set_variable(ctx->machine, def_arg[i], NULL, 0, 0, old_values[i]);
        }
        zx_free_string(&old_values[i]);
        zx_free_string(&call_args[i]);
    }

    return err;

error_cleanup:
    for (size_t i = 0; i < 52; i++) {
        zx_free_string(&call_args[i]);
        zx_free_string(&old_values[i]);
    }
    return err;
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

    if (token == ZX_CHAR_BRACKET_OPEN) {
        ctx->cursor++;
        zx_skip_spaces(ctx);

        err = parse_expression(ctx, &value);
        if (err != ERR_0_OK) goto error_cleanup;

        if (ctx->buffer[ctx->cursor] != ZX_CHAR_BRACKET_CLOSE) {
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

        while (ctx->buffer[ctx->cursor] == ZX_DIGIT_ZERO ||
            ctx->buffer[ctx->cursor] == ZX_DIGIT_ONE) {
            char bit = ctx->buffer[ctx->cursor];
            ctx->cursor++;
            total_digits++;

            if (bit == ZX_DIGIT_ONE) {
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
            if (ctx->buffer[ctx->cursor] != ZX_CHAR_BRACKET_OPEN) {
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

            if (ctx->buffer[ctx->cursor] != ZX_CHAR_BRACKET_CLOSE) {
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
    if (token == ZX_FUN_FN) {
        ctx->cursor++;
        zx_skip_spaces(ctx);
        if (ctx->cursor >= ctx->size) return ERR_C_NONSENSE_IN_BASIC;

        return parse_function_definition(ctx, out_value);
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

    if (token == ZX_CHAR_QUOTES) {
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
    if (strlen(out_var_name) == 2 && out_var_name[1] == ZX_CHAR_DOLLAR) {
        // String of String-array
        if (ctx.cursor < ctx.size && ctx.buffer[ctx.cursor] == ZX_CHAR_BRACKET_OPEN) {
            ctx.cursor++; // Skip '('
            zx_skip_spaces(&ctx);

            err = parse_array_index_string(&ctx, out_indices, out_num_indices, out_desired_len);
            if (err != ERR_0_OK) return err;

            if (ctx.cursor >= ctx.size || ctx.buffer[ctx.cursor] != ZX_CHAR_BRACKET_CLOSE) {
                return ERR_C_NONSENSE_IN_BASIC;
            }
            ctx.cursor++; // Skip ')'
            zx_skip_spaces(&ctx);

            // =========================================================================
            // DE KRONKEL-UPDATE: Gekoppelde slicing-notatie afvangen (bijv. A$(2)(7) of A$(2)(7 TO 9))
            // =========================================================================
            while (ctx.cursor < ctx.size && ctx.buffer[ctx.cursor] == ZX_CHAR_BRACKET_OPEN) {
                ctx.cursor++; // Skip de opeenvolgende '('
                zx_skip_spaces(&ctx);

                uint16_t next_indices[10] = {0};
                uint8_t next_num_indices = 0;
                int32_t next_desired_len = SLICE_NO_TO;

                // Ontleed de inhoud van de tweede (of opeenvolgende) haakjes
                err = parse_array_index_string(&ctx, next_indices, &next_num_indices, &next_desired_len);
                if (err != ERR_0_OK) return err;

                if (ctx.cursor >= ctx.size || ctx.buffer[ctx.cursor] != ZX_CHAR_BRACKET_CLOSE) {
                    return ERR_C_NONSENSE_IN_BASIC;
                }
                ctx.cursor++; // Skip ')'
                zx_skip_spaces(&ctx);

                // Combineer de losse indexering tot de uiteindelijke multi-dimensionale array-referentie
                if (next_num_indices == 1) {
                    if (*out_num_indices < 10) {
                        out_indices[*out_num_indices] = next_indices[0];
                        (*out_num_indices)++;
                        *out_desired_len = next_desired_len;
                    } else {
                        return ERR_3_SUBSCRIPT_WRONG;
                    }
                } else {
                    return ERR_C_NONSENSE_IN_BASIC;
                }
            }
        }
    } else if (strlen(out_var_name) == 1) {
        // Numerieke array
        if (ctx.cursor < ctx.size && ctx.buffer[ctx.cursor] == ZX_CHAR_BRACKET_OPEN) {
            ctx.cursor++; // Skip '('
            zx_skip_spaces(&ctx);

            err = parse_array_index_numeric(&ctx, out_indices, out_num_indices);
            if (err != ERR_0_OK) return err;

            if (ctx.cursor >= ctx.size || ctx.buffer[ctx.cursor] != ZX_CHAR_BRACKET_CLOSE) {
                return ERR_C_NONSENSE_IN_BASIC;
            }
            ctx.cursor++; // Skip ')'
        }
    }

    // Vertel de beller exact hoever we in het rauwe buffer zijn doorgeschoven!
    *bytes_read = ctx.cursor;
    return ERR_0_OK;
}
ZxError zx_parse_variable_for_dim(ZxMachine machine, const uint8_t *buffer, size_t size, size_t *bytes_read, char *out_var_name, uint16_t *out_indices, uint8_t *out_num_indices) {
    if (buffer == NULL || bytes_read == NULL || out_var_name == NULL) return ERR_UNKNOWN;

    ParserContext ctx;
    ctx.machine = machine;
    ctx.buffer = buffer;
    ctx.size = size;
    ctx.cursor = 0;

    size_t name_bytes;
    ZxError err = parse_variable_name(ctx.buffer, ctx.size, out_var_name, &name_bytes);
    if (err != ERR_0_OK) return err;
    size_t name_len = strlen(out_var_name);
    if ((name_len > 2) || (name_len == 2 && out_var_name[1] != ZX_CHAR_DOLLAR)) {
        return ERR_C_NONSENSE_IN_BASIC;
    }
    ctx.cursor += name_bytes;
    zx_skip_spaces(&ctx);

    if (ctx.cursor < ctx.size && ctx.buffer[ctx.cursor] == ZX_CHAR_BRACKET_OPEN) {
        ctx.cursor++;
        zx_skip_spaces(&ctx);

        err = parse_array_index_numeric(&ctx, out_indices, out_num_indices);
        if (err != ERR_0_OK) return err;

        if (ctx.cursor >= ctx.size || ctx.buffer[ctx.cursor] != ZX_CHAR_BRACKET_CLOSE) {
            return ERR_C_NONSENSE_IN_BASIC;
        }
        ctx.cursor++;
    } else {
        return ERR_C_NONSENSE_IN_BASIC;
    }


    *bytes_read = ctx.cursor;
    return ERR_0_OK;
}
