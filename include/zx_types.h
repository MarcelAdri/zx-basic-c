//
// Created by Marcel on 28-05-2026.
//

#ifndef ZX_BASIC_C_ZX_TYPES_H
#define ZX_BASIC_C_ZX_TYPES_H
#include <stdint.h>
#include <stdbool.h>

#include "errors.h"

#define ZX_TRUE  1.0
#define ZX_FALSE 0.0
#define MAX_VAR_NAME_LEN 100

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

typedef struct {
    bool exists;                // Is deze array ge-DIM-med?
    uint8_t num_dimensions;     // Aantal dimensies
    uint16_t *dimension_sizes;  // De groottes per dimensie (bijv. [10, 20])
    double *data;               // De platte array met getallen
} ZxNumericArray;

typedef struct {
    bool exists;                // Bestaat deze string of array überhaupt?
    bool is_dimmed;             // false = gewone string, true = character matrix (array)

    // Dynamische data-pointer
    uint8_t *data;              // Platte byte-array
    size_t len;                 // Bij gewone string: actuele lengte. Bij DIM: totale vlakke grootte.

    // Alleen relevant als is_dimmed == true
    uint8_t num_dimensions;
    uint16_t *dimension_sizes;
} ZxStringSlot;

typedef struct {
    char name[MAX_VAR_NAME_LEN];       // We reserveren max. 99 tekens voor de naam (+ '\0')
    double value;
} NumericVariable;

typedef struct {
    uint16_t return_line;
    uint8_t return_statement;
    double end_value;
    double step_value;
} ZxLoopControl;

typedef struct {
    uint16_t return_line;
    uint8_t return_statement;
} ZxGoSub;

void zx_init_value(ZxValue *val);
ZxError zx_assign_string(const uint8_t *text, size_t length, ZxValue *val);
ZxError zx_assign_number(double num, ZxValue *val);
ZxValueType type_of(ZxValue val);
ZxError zx_get_number(ZxValue val, double *out_val);
ZxError zx_get_int(ZxValue val, int min_val, int max_val, int *out_val);
ZxError zx_get_string(ZxValue val, uint8_t **out_text, size_t *out_length);
void zx_free_string(ZxValue *val);
ZxError zx_get_numeric_value(const NumericVariable *var, ZxValue *val);
ZxError zx_set_numeric_value(NumericVariable *var, ZxValue val);
void zx_init_numeric_array(ZxNumericArray *arr);
void zx_free_numeric_array(ZxNumericArray *arr);
ZxError zx_dim_numeric_array(uint8_t num_dimensions, const uint16_t *dimension_sizes, ZxNumericArray *arr);
ZxError zx_get_numeric_array_element(const ZxNumericArray *arr, const uint16_t *indices, uint8_t num_indices_passed, ZxValue *val);
ZxError zx_set_numeric_array_element(ZxNumericArray *arr, const uint16_t *indices, uint8_t num_indices_passed, ZxValue val);
void zx_init_string_slot(ZxStringSlot *slot);
void zx_free_string_slot(ZxStringSlot *slot);
ZxError zx_dim_string_slot(uint8_t num_dimensions, const uint16_t *dimension_sizes, ZxStringSlot *slot);
ZxError zx_get_string_element(const ZxStringSlot *slot, const uint16_t *indices, uint8_t num_indices_passed, int32_t desired_len, ZxValue *val);
ZxError zx_set_string_element(ZxStringSlot* slot, const uint16_t* indices, size_t num_indices_passed, int32_t desired_len, ZxValue val);
void loop_init(ZxLoopControl *loop_control);
void loop_set(ZxLoopControl *loop_control, uint16_t return_line, uint8_t return_statement, double end_value, double step_value);
void set_go_sub_stack(ZxGoSub *go_sub_stack, uint16_t line, uint8_t statement);

#endif //ZX_BASIC_C_ZX_TYPES_H
