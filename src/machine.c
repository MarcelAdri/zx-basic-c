//
// Created by Marcel on 17-05-2026.
//
#include <stddef.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "machine.h"
#include "errors.h"

#define NOT_FOUND (-1)

typedef struct {
    char name[MAX_VAR_NAME_LEN];       // We reserveren max. 99 tekens voor de naam (+ '\0')
    double value;
} NumericVariable;

typedef struct Machine {
    double loop_counters[26];
    bool loop_counter_defined[26];

    char string_variables[26][256];
    bool string_variable_defined[26];

    NumericVariable *numeric_vars;
    int numeric_variable_count;
    int numeric_variable_capacity;

    ZxPrintCallback print_callback;
    uint8_t cursor_x;
    uint8_t cursor_y;
} Machine;

static void sanitize_variabele_name(char *dest, const char *src, const size_t max_len) {
    size_t dest_idx = 0;

    while (*src != '\0' && dest_idx < max_len - 1) {
        if (!isspace((unsigned char)*src)) {
            dest[dest_idx] = (char)tolower((unsigned char) *src);
            dest_idx++;
        }
        src++;
    }
    dest[dest_idx] = '\0';
}

static int get_numeric_variable_index(ZxMachine machine, const char *var_name, const int max_len) {
    char clean_search_name[max_len];
    sanitize_variabele_name(clean_search_name, var_name, max_len);

    for (int i = 0; i < machine->numeric_variable_count; i++) {
        char clean_existing_name[max_len];
        sanitize_variabele_name(clean_existing_name,
            machine->numeric_vars[i].name, max_len);
        if (strcmp(clean_existing_name, clean_search_name) == 0) {
            return i;
        }
    }
    return NOT_FOUND;
}

ZxMachine machine_create(void) {
    Machine* machine = malloc(sizeof(Machine));
    if (machine == NULL) {
        return NULL;
    }

    memset(machine, 0, sizeof(Machine));

    machine->numeric_variable_capacity = 4;
    machine->numeric_vars = malloc(machine->numeric_variable_capacity * sizeof(NumericVariable));
    if (machine->numeric_vars == NULL) {
        free(machine); // Opruimen als dit faalt!
        return NULL;
    }

    return machine;
}

ZxError machine_set_numeric(ZxMachine machine, const char *var_name, double value) {
    int i = get_numeric_variable_index(machine, var_name, MAX_VAR_NAME_LEN);
    if (i != NOT_FOUND) {
        machine->numeric_vars[i].value = value;
        return ERR_OK;
    }

    if (machine->numeric_variable_count >= machine->numeric_variable_capacity) {
        machine->numeric_variable_capacity *= 2;
        machine->numeric_vars = realloc(machine->numeric_vars, machine->numeric_variable_capacity * sizeof(NumericVariable));
        if (machine->numeric_vars == NULL) {
            return ERR_MEM_ALLOCATION;
        }
    }
    const int nieuw_index = machine->numeric_variable_count;

    strncpy(machine->numeric_vars[nieuw_index].name, var_name, MAX_VAR_NAME_LEN - 1);
    machine->numeric_vars[nieuw_index].name[MAX_VAR_NAME_LEN - 1] = '\0';
    machine->numeric_vars[nieuw_index].value = value;

    machine->numeric_variable_count++;

    return ERR_OK;
}

ZxError machine_get_numeric(ZxMachine machine, const char *var_name, double *value) {
    int i = get_numeric_variable_index(machine, var_name, MAX_VAR_NAME_LEN);
    if (i != NOT_FOUND) {
        *value = machine->numeric_vars[i].value;
        return ERR_OK;
    }
    return ERR_UNDEFINED_VARIABLE;
}

void machine_destroy(ZxMachine machine) {
    if (machine != NULL) {
        // Ruim eerst de dynamische array binnenin op
        if (machine->numeric_vars != NULL) {
            free(machine->numeric_vars);
        }
        // Ruim daarna de machine struct zelf op
        free(machine);
    }
}
uint8_t machine_get_cursor_x(ZxMachine machine) {
    if (machine != NULL) {
        return machine->cursor_x;
    }
    return 0;
}
void machine_set_print_callback(ZxMachine machine, ZxPrintCallback callback) {
    if (machine != NULL) {
        machine->print_callback = callback;
    }
}
void machine_print_output(ZxMachine machine, const char *text) {
    if (machine == NULL || machine->print_callback == NULL || text == NULL) return;

    size_t len = strlen(text);
    if (len == 0) return;

    size_t required_size = len + (len / 32) + 2;

    char *output_buffer = malloc(required_size);
    if (output_buffer == NULL) {

        return;
    }

    size_t out_idx = 0;

    for (size_t i = 0; i < len; i++) {
        if (machine->cursor_x >= 32) {
            output_buffer[out_idx] = '\n';
            out_idx++;
            machine->cursor_x = 0;
        }

        output_buffer[out_idx] = text[i];
        out_idx++;

        if (text[i] == '\n') {
            machine->cursor_x = 0;
        } else {
            machine->cursor_x++;
        }
    }

    output_buffer[out_idx] = '\0';

    machine->print_callback(output_buffer);

    free(output_buffer);
}
