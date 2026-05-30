//
// Created by Marcel on 17-05-2026.
//
#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "machine.h"
#include "errors.h"
#include "zx_types.h"
#include "helpers.h"

#define NOT_FOUND (-1)

typedef struct {
    char name[MAX_VAR_NAME_LEN];       // We reserveren max. 99 tekens voor de naam (+ '\0')
    ZxValue value;
} NumericVariable;

typedef enum {
    ZX_STATE_IDLE,          // Wacht op een commando onderin beeld
    ZX_STATE_RUNNING,       // Bezig met het uitvoeren van BASIC (of een commando)
    ZX_STATE_WAIT_SCROLL,   // Scherm is vol, wacht op Y/N/SPACE
    ZX_STATE_WAIT_INPUT,    // BASIC programma staat stil door een INPUT commando
    ZX_STATE_WAIT_PAUSE     // BASIC programma staat stil door PAUSE commando
} ZxState;

typedef struct Machine {
    ZxState state;

    uint8_t text_screen[22][32];
    uint8_t system_screen[2][32];

    uint8_t text_cursor_x;
    uint8_t text_cursor_y;

    uint16_t current_line;
    uint8_t current_statement;

    ZxValue string_variables[26];

    NumericVariable *numeric_vars;
    int numeric_variable_count;
    int numeric_variable_capacity;

    ZxPrintCallback print_callback;

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
    memset(machine, 0, sizeof(Machine));

    if (machine == NULL) {
        return NULL;
    }

    for (int i = 0; i < 26; i++) {
        zx_init_value(&machine->string_variables[i]);
    }

    machine->text_cursor_x = 0;
    machine->text_cursor_y = 0;

    memset(&machine->text_screen[0][0], ' ', 22 * 32);
    memset(&machine->system_screen[0][0], ' ', 2 * 32);

    machine->numeric_variable_capacity = 4;
    machine->numeric_vars = malloc(machine->numeric_variable_capacity * sizeof(NumericVariable));
    if (machine->numeric_vars == NULL) {
        free(machine); // Opruimen als dit faalt!
        return NULL;
    }

    return machine;
}
int machine_get_state(ZxMachine machine) {
    if (machine != NULL) {
        return (int)machine->state;
    }
    return 0; // ZX_STATE_IDLE
}
void machine_set_location(ZxMachine machine, const uint16_t line, const uint8_t statement) {
    if (machine) {
        machine->current_line = line;
        machine->current_statement = statement;
    }
}

uint16_t machine_get_current_line(ZxMachine machine) {
    return machine ? machine->current_line : 0;
}

uint8_t machine_get_current_statement(ZxMachine machine) {
    return machine ? machine->current_statement : 1;
}
ZxError machine_set_numeric(ZxMachine machine, const char *var_name, ZxValue value) {
    if (machine == NULL || var_name == NULL) {
        return ERR_UNKNOWN;
    }
    if (value.type != ZX_TYPE_NUMBER) {
        return ERR_C_NONSENSE_IN_BASIC;
    }
    int i = get_numeric_variable_index(machine, var_name, MAX_VAR_NAME_LEN);
    if (i != NOT_FOUND) {
        return zx_assign_number(value.data.number, &machine->numeric_vars[i].value);
    }

    if (machine->numeric_variable_count >= machine->numeric_variable_capacity) {
        machine->numeric_variable_capacity *= 2;
        machine->numeric_vars = realloc(machine->numeric_vars, machine->numeric_variable_capacity * sizeof(NumericVariable));
        if (machine->numeric_vars == NULL) {
            return ERR_4_OUT_OF_MEMORY;
        }
    }
    const int nieuw_index = machine->numeric_variable_count;

    strncpy(machine->numeric_vars[nieuw_index].name, var_name, MAX_VAR_NAME_LEN - 1);
    machine->numeric_vars[nieuw_index].name[MAX_VAR_NAME_LEN - 1] = '\0';
    zx_init_value(&machine->numeric_vars[nieuw_index].value);

    ZxError err = zx_assign_number(value.data.number, &machine->numeric_vars[nieuw_index].value);

    if (err == ERR_0_OK) {
        machine->numeric_variable_count++;
    }

    return err;
}

ZxError machine_get_numeric(ZxMachine machine, const char *var_name, ZxValue *value) {
    if (machine == NULL || var_name == NULL || value == NULL) {
        return ERR_UNKNOWN;
    }

    int i = get_numeric_variable_index(machine, var_name, MAX_VAR_NAME_LEN);
    if (i != NOT_FOUND) {

        if (machine->numeric_vars[i].value.type != ZX_TYPE_NUMBER) {
            return ERR_C_NONSENSE_IN_BASIC;
        }

        return zx_assign_number(machine->numeric_vars[i].value.data.number, value);
    }
    return ERR_2_VARIABLE_NOT_FOUND;
}

void machine_destroy(ZxMachine machine) {
    if (machine != NULL) {
        // Ruim eerst de dynamische array binnenin op
        for (int i = 0; i < 26; i++) {
            zx_free_string(&machine->string_variables[i]);
        }
        if (machine->numeric_vars != NULL) {
            free(machine->numeric_vars);
        }
        // Ruim daarna de machine struct zelf op
        free(machine);
    }
}
uint8_t machine_get_text_cursor_x(ZxMachine machine) {
    if (machine != NULL) {
        return machine->text_cursor_x;
    }
    return 0;
}
uint8_t machine_get_text_cursor_y(ZxMachine machine) {
    if (machine != NULL) {
        return machine->text_cursor_y;
    }
    return 0;
}
void machine_set_text_cursor_x(ZxMachine machine, const uint8_t x) {
    if (machine != NULL) {
        machine->text_cursor_x = x;
    }
}
void machine_set_print_callback(ZxMachine machine, ZxPrintCallback callback) {
    if (machine != NULL) {
        machine->print_callback = callback;
    }
}
const uint8_t* machine_get_text_screen(ZxMachine machine) {
    if (machine != NULL) {
        return &machine->text_screen[0][0]; // Pointer naar de allereerste pixel
    }
    return NULL;
}
void machine_next_line(ZxMachine machine) {
    machine->text_cursor_x = 0;
    machine->text_cursor_y++;
    if (machine->text_cursor_y > 21) {
        machine->state = ZX_STATE_WAIT_SCROLL;
        memmove(&machine->text_screen[0][0], &machine->text_screen[1][0], 21 * 32);
        memset(&machine->text_screen[21][0], ' ', 32);
        machine->text_cursor_y = 21;
    }

}
static void machine_print_to_text(ZxMachine machine, const char *text) {
    assert(text != NULL && "text mag nooit NULL zijn in deze interne functie");
    if (machine == NULL || machine->print_callback == NULL) return;

    size_t len = strlen(text);
    if (len == 0) return;

    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\n') {
            machine_next_line(machine);
            continue;
        }
        machine->text_screen[machine->text_cursor_y][machine->text_cursor_x] = text[i];
        if (machine->text_cursor_x >= 31) {
            machine_next_line(machine);
        } else {
            machine->text_cursor_x++;
        }
    }
}
void machine_print_value(ZxMachine machine, const ZxValue value) {
    char c_string[2048] = {0};

    if (value.type == ZX_TYPE_STRING) {
        // Directe conversie: ZX-tokens van de stringvariabele -> C-string
        if (zx_to_string(value.data.string.text, value.data.string.length, c_string, sizeof(c_string)) == ERR_0_OK) {
            machine_print_to_text(machine, c_string);
        }
    }
    else if (value.type == ZX_TYPE_NUMBER) {
        uint8_t zx_tokens[32];
        size_t bytes_written = 0;

        // Stap 1: Van double naar ZX-tokens
        if (formatted_number(value.data.number, zx_tokens, sizeof(zx_tokens), &bytes_written) == ERR_0_OK) {
            // Stap 2: Van ZX-tokens naar C-string
            if (zx_to_string(zx_tokens, bytes_written, c_string, sizeof(c_string)) == ERR_0_OK) {
                machine_print_to_text(machine, c_string);
            }
        }
    }
}
const uint8_t* machine_get_system_screen(ZxMachine machine) {
    if (machine != NULL) {
        return &machine->system_screen[0][0];
    }
    return NULL;
}

// Een simpele functie om een melding (zoals "0 OK, 0:1") onderin te zetten
void machine_print_to_system(ZxMachine machine, const char *text) {
    if (machine == NULL || text == NULL) return;

    // Maak het systeemvak eerst even netjes schoon met spaties
    memset(&machine->system_screen[0][0], ' ', 2 * 32);

    size_t len = strlen(text);
    for (size_t i = 0; i < len && i < 64; i++) {
        // Bereken simpelweg de X en Y op basis van de index (max 64 tekens)
        int y = i / 32;
        int x = i % 32;
        machine->system_screen[y][x] = text[i];
    }
}
