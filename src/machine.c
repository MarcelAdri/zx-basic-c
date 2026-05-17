//
// Created by Marcel on 17-05-2026.
//
#include <stddef.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "machine.h"
#include "errors.h"

#define NOT_FOUND (-1)

typedef struct {
    char name[MAX_VAR_NAME_LEN];       // We reserveren max. 99 tekens voor de naam (+ '\0')
    float value;
} NumericVariable;

typedef struct Machine {
    float loop_counters[26];
    bool loop_counter_defined[26];

    char string_variables[26][256];
    bool string_variable_defined[26];

    NumericVariable *numeric_vars;
    int numeric_variable_count;
    int numeric_variable_capacity;
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

ZxError machine_set_numeric(ZxMachine machine, const char *var_name, float value) {
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

ZxError machine_get_numeric(ZxMachine machine, const char *var_name, float *value) {
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
