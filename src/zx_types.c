//
// Created by Marcel on 28-05-2026.
//

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "zx_types.h"

#include <stdlib.h>
#include <string.h>

#include "characters.h"
#include "errors.h"

static ZxError zx_make_number(double val, ZxValue *out_value) {
    //zx_max_number is a cached value for the maximum representable number in ZX Spectrum BASIC (calculated 5byte float)
    static double zx_max_number = 0;
    static bool zx_max_number_set = false;

    if (!zx_max_number_set) {
        zx_max_number = (1.0 - pow(2, -32)) * pow(2, 127);
        zx_max_number_set = true;
    }

    //zx_min_number is a cached value for the smallest representable number in ZX Spectrum BASIC (calculated 5byte float)
    static double zx_min_number = 0;
    static bool zx_min_number_set = false;
    if (!zx_min_number_set) {
        zx_min_number = pow(2, -128);
        zx_min_number_set = true;
    }
    if (isnan(val)) {

        return ERR_A_INVALID_ARGUMENT;
    }
    if (isinf(val) || val > zx_max_number || val < -zx_max_number) {
        return ERR_6_NUMBER_TOO_BIG;
    }
    if ((val > 0 && val < zx_min_number) || (val < 0 && val > -zx_min_number)) {
        val = 0;
    }

    //limit precision to 9 decimals
    char buf[32];
    sprintf(buf, "%.9g", val);
    double quirky_value = strtod(buf, NULL);


    out_value->type = ZX_TYPE_NUMBER;
    out_value->data.number = quirky_value;
    return ERR_0_OK;
}

ZxError zx_get_number(const ZxValue val, double *out_val) {
    if (val.type != ZX_TYPE_NUMBER) {
        return ERR_C_NONSENSE_IN_BASIC;
    }
    *out_val = val.data.number;
    return ERR_0_OK;
}
ZxError zx_get_int(ZxValue val, int min_val, int max_val, int *out_val) {
    if (val.type != ZX_TYPE_NUMBER) {
        return ERR_C_NONSENSE_IN_BASIC;
    }
    double truncated = trunc(val.data.number);
    if (truncated < min_val || truncated > max_val) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }
    *out_val = (int) truncated;
    return ERR_0_OK;
}
static ZxError zx_make_string(const uint8_t *text, const size_t length, ZxValue *out_value) {
    if (length > 0 && text == NULL) {
        return ERR_A_INVALID_ARGUMENT;
    }
    uint8_t *new_text = NULL;
    if (length > 0) {
        new_text = malloc(length);
        if (new_text == NULL) {
            return ERR_4_OUT_OF_MEMORY;
        }
        memcpy(new_text, text, length);

    }
    out_value->type = ZX_TYPE_STRING;
    out_value->data.string.text = new_text;
    out_value->data.string.length = length;
    return ERR_0_OK;
}
ZxError zx_get_string(const ZxValue val, uint8_t **out_text, size_t *out_length) {
    if (val.type != ZX_TYPE_STRING) {
        return ERR_C_NONSENSE_IN_BASIC;
    }
    *out_text = val.data.string.text;
    *out_length = val.data.string.length;
    return ERR_0_OK;
}
ZxError zx_to_string(const uint8_t *input, const size_t input_length, char *output, size_t out_length) {
    if (input == NULL || output == NULL) {
        return ERR_UNKNOWN;
    }
    if (input_length > out_length - 1) {
        return ERR_UNKNOWN;
    }
    for (size_t i = 0; i < input_length; i++) {
        if (is_zx_printable_character(input[i])) {
            output[i] = input[i];
            continue;
        }
        output[i] = '?'; //TODO: verfijnen met langere tokens
    }
    output[input_length] = '\0';
    return ERR_0_OK;
}

void zx_free_string(ZxValue *val) {
    if (val == NULL) return;

    if (val->type == ZX_TYPE_STRING) {
        if (val->data.string.text != NULL) {
            free(val->data.string.text);       // Geef het geheugen terug aan C
            val->data.string.text = NULL;      // Voorkom een 'dangling pointer'
        }
        val->data.string.length = 0;
    }

    val->type = ZX_TYPE_NUMBER;
    val->data.number = 0.0;
}
void zx_init_value(ZxValue *val) {
    // Zet een nieuwe variabele gegarandeerd in een veilige state
    val->type = ZX_TYPE_UNDEFINED;
    val->data.number = 0.0;
}

ZxError zx_assign_string(const uint8_t *text, size_t length, ZxValue *val) {
    // 1. Ruim eventuele oude data in deze variabele veilig op
    zx_free_string(val);

    // 2. Laat de verborgen helper het gevaarlijke allocatie-werk doen
    return zx_make_string(text, length, val);
}

ZxError zx_assign_number(double num, ZxValue *val) {
    // Ruim op voor het geval deze variabele hiervoor een string was!
    zx_free_string(val);

    return zx_make_number(num, val);
}