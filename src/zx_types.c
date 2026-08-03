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
#include "expressions.h"
#include "helpers.h"

//ZxValue
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

ZxValueType type_of(const ZxValue val) {
    return val.type;
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
//NumericVariables
ZxError zx_get_numeric_value(const NumericVariable *var, ZxValue *val) {
    if (var == NULL || val == NULL) return ERR_UNKNOWN;

    return zx_assign_number(var->value, val);
}
ZxError zx_set_numeric_value(NumericVariable *var, ZxValue val) {
    if (var == NULL) return ERR_UNKNOWN;

    ZxError err;
    double number;
    err = zx_get_number(val, &number);
    if (err != ERR_0_OK) return err;

    var->value = number;
    return ERR_0_OK;
}

//Numeric arrays
void zx_init_numeric_array(ZxNumericArray *arr) {
    if (arr == NULL) return;
    arr->exists = false;
    arr->num_dimensions = 0;
    arr->dimension_sizes = NULL;
    arr->data = NULL;
}
void zx_free_numeric_array(ZxNumericArray *arr) {
    if (arr == NULL) return;
    if (arr->exists) {
        free(arr->dimension_sizes);
        free(arr->data);
        zx_init_numeric_array(arr);
    }
}
ZxError zx_dim_numeric_array(uint8_t num_dimensions, const uint16_t *dimension_sizes, ZxNumericArray *arr) {
    if (arr == NULL) return ERR_UNKNOWN;
    if (num_dimensions == 0 || dimension_sizes == NULL) return ERR_C_NONSENSE_IN_BASIC;
    if (num_dimensions > 10) return ERR_B_INTEGER_OUT_OF_RANGE;

    if (arr->exists) {
        zx_free_numeric_array(arr);
    }
    size_t flat_dim = 1;
    for (uint8_t i = 0; i < num_dimensions; i++) {
        if (dimension_sizes[i] == 0) return ERR_B_INTEGER_OUT_OF_RANGE;
        flat_dim *= dimension_sizes[i];
    }
    arr->dimension_sizes = malloc(sizeof(uint16_t) * num_dimensions);
    if (arr->dimension_sizes == NULL) return ERR_4_OUT_OF_MEMORY;
    memcpy(arr->dimension_sizes, dimension_sizes, sizeof(uint16_t) * num_dimensions);

    arr->data = calloc(flat_dim, sizeof(double));
    if (arr->data == NULL) {
        free(arr->dimension_sizes);
        arr->dimension_sizes = NULL;
        return ERR_4_OUT_OF_MEMORY;
    }

    arr->num_dimensions = num_dimensions;
    arr->exists = true;
    return ERR_0_OK;
}
ZxError zx_get_numeric_array_element(const ZxNumericArray *arr, const uint16_t *indices, const uint8_t num_indices_passed, ZxValue *val) {
    if (arr == NULL || indices == NULL || val == NULL) return ERR_UNKNOWN;

    if (!arr->exists) return ERR_2_VARIABLE_NOT_FOUND;

    if (num_indices_passed != arr->num_dimensions) {
        return ERR_3_SUBSCRIPT_WRONG;
    }

    size_t flat_idx = 0;
    for (uint8_t i = 0; i < arr->num_dimensions; i++) {

        if (indices[i] < 1 || indices[i] > arr->dimension_sizes[i]) {
            return ERR_B_INTEGER_OUT_OF_RANGE;
        }

        size_t idx = indices[i] - 1;

        flat_idx = flat_idx * arr->dimension_sizes[i] + idx;
    }

    return zx_assign_number(arr->data[flat_idx], val);
}
ZxError zx_set_numeric_array_element(ZxNumericArray *arr, const uint16_t *indices, const uint8_t num_indices_passed, const ZxValue val) {
    if (arr == NULL || indices == NULL) return ERR_UNKNOWN;

    if (!arr->exists) return ERR_2_VARIABLE_NOT_FOUND;

    if (num_indices_passed != arr->num_dimensions) {
        return ERR_3_SUBSCRIPT_WRONG;
    }

    ZxError err;
    double value;
    err = zx_get_number(val, &value);
    if (err != ERR_0_OK) return err;

    size_t flat_idx = 0;
    for (uint8_t i = 0; i < arr->num_dimensions; i++) {

        if (indices[i] < 1 || indices[i] > arr->dimension_sizes[i]) {
            return ERR_B_INTEGER_OUT_OF_RANGE;
        }

        size_t idx = indices[i] - 1;

        flat_idx = flat_idx * arr->dimension_sizes[i] + idx;
    }

    arr->data[flat_idx] = value;
    return ERR_0_OK;
}

//Strings
void zx_init_string_slot(ZxStringSlot *slot) {
    if (slot == NULL) return;
    slot->exists = false;
    slot->is_dimmed = false;
    slot->data = NULL;
    slot->len = 0;
    slot->num_dimensions = 0;
    slot->dimension_sizes = NULL;
}
void zx_free_string_slot(ZxStringSlot *slot) {
    if (slot == NULL) return;
    if (slot->exists) {
        free(slot->dimension_sizes);
        free(slot->data);
        zx_init_string_slot(slot);
    }
}
ZxError zx_dim_string_slot(uint8_t num_dimensions, const uint16_t *dimension_sizes, ZxStringSlot *slot) {
    if (slot == NULL || num_dimensions == 0 || dimension_sizes == NULL) return ERR_UNKNOWN;
    if (num_dimensions > 10) return ERR_B_INTEGER_OUT_OF_RANGE;

    if (slot->exists) {
        zx_free_string_slot(slot);
    }
    size_t flat_dim = 1;
    for (uint8_t i = 0; i < num_dimensions; i++) {
        if (dimension_sizes[i] == 0) return ERR_B_INTEGER_OUT_OF_RANGE;
        flat_dim *= dimension_sizes[i];
    }
    slot->dimension_sizes = malloc(sizeof(uint16_t) * num_dimensions);
    if (slot->dimension_sizes == NULL) return ERR_4_OUT_OF_MEMORY;
    memcpy(slot->dimension_sizes, dimension_sizes, sizeof(uint16_t) * num_dimensions);

    slot->data = malloc(flat_dim);
    if (slot->data == NULL) {
        free(slot->dimension_sizes);
        slot->dimension_sizes = NULL;
        return ERR_4_OUT_OF_MEMORY;
    }
    memset(slot->data, 32, flat_dim);

    slot->len = flat_dim;
    slot->num_dimensions = num_dimensions;
    slot->exists = true;
    slot->is_dimmed = true;
    return ERR_0_OK;
}
ZxError zx_get_string_element(const ZxStringSlot *slot, const uint16_t *indices, const uint8_t num_indices_passed, int32_t desired_len, ZxValue *val) {
    if (slot == NULL || val == NULL) return ERR_UNKNOWN;
    if (slot->exists == false) return ERR_2_VARIABLE_NOT_FOUND;

    // =========================================================================
    // SCENARIO 1 & 2: GEWONE STRINGS (is_dimmed == false)
    // =========================================================================
    if (!slot->is_dimmed) {
        if (num_indices_passed > 1) return ERR_3_SUBSCRIPT_WRONG;

        size_t start_idx = 0;
        size_t string_len = slot->len;

        // Als er een index/slice is meegegeven (bijv. a$(4 TO 6))
        if (num_indices_passed == 1) {
            if (indices == NULL) return ERR_UNKNOWN;

            // Grensbewaking: begint de slice binnen de string?
            if (indices[0] < 1 || indices[0] > slot->len) {
                return ERR_B_INTEGER_OUT_OF_RANGE; // B Subscript wrong
            }

            start_idx = indices[0] - 1; // 0-based voor C

            // Als er een gewenste lengte is meegegeven, dwingen we die af
            if (desired_len >= 0) {
                // Check of de slice niet over de rechterkant van de string heen kiepert
                if (desired_len > 0 && (indices[0] + desired_len - 1 > slot->len)) {
                    return ERR_B_INTEGER_OUT_OF_RANGE;
                }
                string_len = desired_len;
            } else {
                // Sinclair-kronkel: a$(4) zonder TO betekent op de Spectrum: "vanaf karakter 4 tot het EIND"
                string_len = slot->len - start_idx;
            }
        }
        return zx_assign_string(&slot->data[start_idx], string_len, val);
    }

    // =========================================================================
    // SCENARIO 3 & 4: GEDIMENSONEERDE ARRAYS (is_dimmed == true)
    // =========================================================================
    // Spectrum check: je moet of N-1 indices meegeven (hele rij) of N indices (karakter/slice)
    if (indices == NULL) return ERR_UNKNOWN;
    if (num_indices_passed < slot->num_dimensions - 1 || num_indices_passed > slot->num_dimensions) {
        return ERR_3_SUBSCRIPT_WRONG;
    }

    size_t flat_idx = 0;
    for (uint8_t i = 0; i < num_indices_passed; i++) {
        if (indices[i] < 1 || indices[i] > slot->dimension_sizes[i]) {
            return ERR_B_INTEGER_OUT_OF_RANGE;
        }

        // VERDEDIGING: Als dit de allerlaatste dimensie is (de string-breedte),
        // en er is een gewenste slice-lengte, controleer dan of deze binnen de breedte past!
        if (i == slot->num_dimensions - 1 && desired_len > 0) {
            if (indices[i] + desired_len - 1 > slot->dimension_sizes[i]) {
                return ERR_B_INTEGER_OUT_OF_RANGE; // Slice loopt buiten de matrix-rij!
            }
        }

        size_t idx = indices[i] - 1;
        flat_idx = flat_idx * slot->dimension_sizes[i] + idx;
    }

    // --- PAS 2: Bepaal de doellengte (string_len) en corrigeer flat_idx ---
    size_t string_len = 1;

    if (num_indices_passed == slot->num_dimensions - 1) {
        // GEVAL 1: De gebruiker vraagt een HELE rij op (bijv. a$(3))
        size_t last_dim_size = slot->dimension_sizes[slot->num_dimensions - 1];
        flat_idx *= last_dim_size;   // Verschuif de pointer naar het begin van de gevraagde rij!
        string_len = last_dim_size;  // De lengte is de volledige breedte van de rij (10 tekens)
    }
    else if (num_indices_passed == slot->num_dimensions) {
        // GEVAL 2: De gebruiker vraagt een specifiek karakter of slice op (bijv. a$(3, 2 TO 7))
        size_t last_dim_idx = slot->num_dimensions - 1;
        size_t last_dim_size = slot->dimension_sizes[last_dim_idx];
        size_t char_pos_in_row = indices[last_dim_idx] - 1;

        if (desired_len >= 0) {
            string_len = desired_len;
        } else if (desired_len == SLICE_OPEN_TO) {
            string_len = last_dim_size - char_pos_in_row;
        } else {
            string_len = 1; // SLICE_NO_TO -> exact 1 karakter
        }
    }

    return zx_assign_string(&slot->data[flat_idx], string_len, val);
}
ZxError zx_set_string_element(ZxStringSlot* slot, const uint16_t* indices, const size_t num_indices_passed, const int32_t desired_len, const ZxValue val) {
    if (slot == NULL) return ERR_UNKNOWN;

    ZxError err;
    uint8_t *value_string = NULL;
    size_t value_length = 0;

    err = zx_get_string(val, &value_string, &value_length);
    if (err != ERR_0_OK) return err;

    // =========================================================================
    // SCENARIO 1 & 2: GEWONE STRINGS (is_dimmed == false)
    // =========================================================================
    if (!slot->is_dimmed) {
        if (num_indices_passed > 1) return ERR_3_SUBSCRIPT_WRONG;

        size_t start_idx = 0;
        size_t string_len = slot->len;

        // Als er een index/slice is meegegeven (bijv. a$(4 TO 6))
        if (num_indices_passed == 1) {
            if (!slot->exists) return ERR_2_VARIABLE_NOT_FOUND;
            if (indices == NULL) return ERR_UNKNOWN;

            // Grensbewaking: begint de slice binnen de string?
            if (indices[0] < 1 || indices[0] > slot->len) {
                return ERR_B_INTEGER_OUT_OF_RANGE; // B Subscript wrong
            }

            start_idx = indices[0] - 1; // 0-based voor C

            // Als er een gewenste lengte is meegegeven, dwingen we die af
            if (desired_len >= 0) {
                // Check of de slice niet over de rechterkant van de string heen kiepert
                if (desired_len > 0 && (indices[0] + desired_len - 1 > slot->len)) {
                    return ERR_B_INTEGER_OUT_OF_RANGE;
                }
                string_len = desired_len;
            } else {
                // Sinclair-kronkel: a$(4) zonder TO betekent op de Spectrum: "vanaf karakter 4 tot het EIND"
                string_len = slot->len - start_idx;
            }


            if (string_len <= value_length) {
                // Geval: Nieuwe tekst is langer of gelijk aan de slice -> Afkappen (Truncation)!
                memcpy(slot->data + start_idx, value_string, string_len);
            } else {
                // Geval: Nieuwe tekst is korter dan de slice -> Opvullen met spaties (Padding)!
                memcpy(slot->data + start_idx, value_string, value_length);
                memset(slot->data + start_idx + value_length, 32, string_len - value_length);
            }

            return ERR_0_OK;

        }
        // LET a$ = "XYZ" (num_indices_passed == 0)
        if (slot->exists) {
            zx_free_string_slot(slot);
            zx_init_string_slot(slot);
        }

        slot->data = malloc(value_length);
        if (slot->data == NULL) return ERR_4_OUT_OF_MEMORY;

        memcpy(slot->data, value_string, value_length);

        slot->len = value_length;
        slot->exists = true;
        slot->is_dimmed = false;
        return ERR_0_OK;
    }
    // =========================================================================
    // SCENARIO 3 & 4: STRING ARRAYS (is_dimmed == true)
    // =========================================================================
    if (!slot->exists) return ERR_2_VARIABLE_NOT_FOUND;

    // Lokale variabelen voor de interceptie van de Sinclair-kronkel
    const uint16_t *actual_indices = indices;
    size_t actual_num_passed = num_indices_passed;
    uint16_t temp_indices[10] = {0}; // Ruimte voor maximaal 10 dimensies

    // DE KRONKEL INTERCEPTIE:
    // Als er 0 indices worden meegegeven aan een array (bijv. LET a$ = "XYZ"),
    // dan targets de Spectrum altijd de allereerste rij (index 1 voor alle matrix-dimensies)
    if (num_indices_passed == 0) {
        for (uint8_t i = 0; i < slot->num_dimensions - 1; i++) {
            temp_indices[i] = 1;
        }
        actual_indices = temp_indices;
        actual_num_passed = slot->num_dimensions - 1; // Gedraagt zich nu als a$(1)
    }

    if (actual_indices == NULL) return ERR_UNKNOWN;
    if (actual_num_passed < slot->num_dimensions - 1 || actual_num_passed > slot->num_dimensions) {
        return ERR_3_SUBSCRIPT_WRONG;
    }

    // --- PAS 1: Index- en Grensbewaking ---
    size_t flat_idx = 0;
    for (uint8_t i = 0; i < actual_num_passed; i++) {
        // Basiscontrole: valt de index binnen de gedimensioneerde matrix-grenzen?
        if (actual_indices[i] < 1 || actual_indices[i] > slot->dimension_sizes[i]) {
            return ERR_B_INTEGER_OUT_OF_RANGE;
        }

        // Strenge controle voor de allerlaatste dimensie (de karakter-as / stringbreedte)
        if (i == slot->num_dimensions - 1) {
            if (desired_len >= 0) {
                // Expliciete TO (bijv. 4 TO 6). Alleen controleren als de lengte > 0 is.
                // Als desired_len == 0 (6 TO 4), dan hoeven we de rechtergrens niet te controleren!
                if (desired_len > 0 && (actual_indices[i] + desired_len - 1 > slot->dimension_sizes[i])) {
                    return ERR_B_INTEGER_OUT_OF_RANGE;
                }
            }
            // Bij desired_len == -1 of -2 is de basisscheck (<= dimension_sizes[i]) al voldoende!
        }

        // Multi-dimensionale adresberekening (row-major flat index)
        size_t idx = actual_indices[i] - 1;
        flat_idx = flat_idx * slot->dimension_sizes[i] + idx;
    }

    // --- PAS 2: Bepaal de doellengte (string_len) op basis van de vlaggen ---
    size_t string_len = 0;

    if (actual_num_passed == slot->num_dimensions - 1) {
        // Geval: a$(2) -> We targeten de GEHELE resterende rij/dimensie
        size_t last_dim_size = slot->dimension_sizes[slot->num_dimensions - 1];
        flat_idx *= last_dim_size; // Schuif de pointer door naar het begin van de rij
        string_len = last_dim_size; // De doellengte is de volledige breedte van de rij
    } else {
        // Geval: actual_num_passed == slot->num_dimensions -> We zitten op karakter-niveau
        size_t last_dim_idx = slot->num_dimensions - 1;
        size_t last_dim_size = slot->dimension_sizes[last_dim_idx];
        size_t char_pos_in_row = actual_indices[last_dim_idx] - 1;

        if (desired_len >= 0) {
            // Toestand 3: Expliciete TO (bijv. 4 TO 6, óf de lege string 0 bij 6 TO 4!)
            string_len = desired_len;
        } else if (desired_len == SLICE_OPEN_TO) {
            // Toestand 2: Open TO (bijv. a$(1, 4 TO )) -> Vanaf startkarakter tot het absolute einde van de rij
            string_len = last_dim_size - char_pos_in_row;
        } else {
            // Toestand 1: Geen TO gebruikt (bijv. a$(1, 4)) -> Betekent exact één karakter overschrijven
            string_len = 1;
        }
    }

    // --- PAS 3: Procrustean Toewijzing (Truncation & Padding) ---
    if (string_len <= value_length) {
        // Geval: Nieuwe tekst is langer of exact gelijk aan de doellengte -> Afkappen (Truncation)!
        // Veiligheid: Alleen kopiëren als string_len groter is dan 0 (voorkomt memcpy fratsen bij lege slices)
        if (string_len > 0) {
            memcpy(slot->data + flat_idx, value_string, string_len);
        }
    } else {
        // Geval: Nieuwe tekst is korter dan de doellengte -> Invullen en opvullen met spaties (Padding)!
        memcpy(slot->data + flat_idx, value_string, value_length);
        memset(slot->data + flat_idx + value_length, 32, string_len - value_length); // 32 = ASCII spatie
    }

    return ERR_0_OK;
}

//LoopControl
void loop_init(ZxLoopControl *loop_control) {
    loop_control->return_line = 0;
    loop_control->return_statement = 0;
    loop_control->end_value = 0;
    loop_control->step_value = 0;
}
void loop_set(ZxLoopControl *loop_control, const uint16_t return_line, const uint8_t return_statement, const double end_value, const double step_value) {
    loop_control->return_line = return_line;
    loop_control->return_statement = return_statement;
    loop_control->end_value = end_value;
    loop_control->step_value = step_value;
}
//GO SUB
void set_go_sub_stack(ZxGoSub *go_sub_stack, const uint16_t line, const uint8_t statement) {
    go_sub_stack->return_line = line;
    go_sub_stack->return_statement = statement;
}
