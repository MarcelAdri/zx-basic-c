//
// Created by Marcel on 28-05-2026.
//

#ifndef ZX_BASIC_C_ZX_TYPES_H
#define ZX_BASIC_C_ZX_TYPES_H
#include <stdint.h>

#include "errors.h"

#define ZX_TRUE  1.0
#define ZX_FALSE 0.0

typedef enum {
    ZX_TYPE_NUMBER,
    ZX_TYPE_STRING,
    ZX_TYPE_UNDEFINED,
} ZxValueType;

typedef struct {
    uint8_t *text; // Ruwe bytes (kan kleurcodes of \0 bevatten)
    size_t length; // Exacte lengte
} ZxString;

typedef struct {
    ZxValueType type;
    union {
        double number;
        ZxString string;
    } data;
} ZxValue;

void zx_init_value(ZxValue *val);
ZxError zx_assign_string(const uint8_t *text, size_t length, ZxValue *val);
ZxError zx_assign_number(double num, ZxValue *val);
ZxError zx_get_number(ZxValue val, double *out_val);
ZxError zx_get_int(ZxValue val, int min_val, int max_val, int *out_val);
ZxError zx_get_string(ZxValue val, uint8_t **out_text, size_t *out_length);
ZxError zx_to_string(const uint8_t *input, size_t input_length, char *output, size_t out_length);
void zx_free_string(ZxValue *val);

#endif //ZX_BASIC_C_ZX_TYPES_H
