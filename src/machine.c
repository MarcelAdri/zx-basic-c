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
#include "characters.h"

#define NOT_FOUND (-1)

#define ZX_MAX_BASIC_RAM 41500

typedef struct {
    uint8_t *tokens;
    size_t length;
    bool exists;
} ZxLine;

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

    ZxLine program_memory[10000];
    size_t used_basic_ram;

    uint16_t current_line;
    uint8_t current_statement;

    ZxValue string_variables[26];

    NumericVariable *numeric_vars;
    int numeric_variable_count;
    int numeric_variable_capacity;

    ZxPrintCallback print_callback;

    uint32_t rng_state;

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
    machine->rng_state = 12345;

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
ZxError machine_insert_line(ZxMachine machine, uint16_t line_number, const uint8_t *tokens, size_t length) {
    size_t new_line_cost = length + 5;

    size_t old_line_cost = 0;
    if (machine->program_memory[line_number].exists) {
        old_line_cost = machine->program_memory[line_number].length + 5;
    }

    size_t projected_ram = (machine->used_basic_ram - old_line_cost) + new_line_cost;

    if (projected_ram > ZX_MAX_BASIC_RAM) {
        return ERR_G_NO_ROOM_FOR_LINE;
    }

    if (machine->program_memory[line_number].exists) {
        free(machine->program_memory[line_number].tokens);
    }

    machine->program_memory[line_number].tokens = malloc(length);
    memcpy(machine->program_memory[line_number].tokens, tokens, length);
    machine->program_memory[line_number].length = length;
    machine->program_memory[line_number].exists = true;

    machine->used_basic_ram = projected_ram;

    return ERR_0_OK;
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
ZxError machine_set_string(ZxMachine machine, const uint8_t var_name, ZxValue *value) {
    if (machine == NULL || value == NULL) {
        return ERR_UNKNOWN;
    }

    int i = name_to_index(var_name);
    if (i == NOT_FOUND) {
        return ERR_F_INVALID_FILENAME;
    }
    if (value->type != ZX_TYPE_STRING) {
        return ERR_C_NONSENSE_IN_BASIC;
    }

    ZxValue *slot = &machine->string_variables[i];
    zx_free_string(slot);

    uint8_t *parser_text = NULL;
    size_t text_len = 0;
    zx_get_string(*value, &parser_text, &text_len);

    uint8_t *machine_text = malloc(text_len);
    if (machine_text == NULL && text_len > 0) {
        return ERR_4_OUT_OF_MEMORY;
    }
    if (text_len > 0) {
        memcpy(machine_text, parser_text, text_len);
    }
    return zx_assign_string(machine_text, text_len, slot);
}
ZxError machine_get_string(ZxMachine machine, const uint8_t var_name, ZxValue *value) {
    if (machine == NULL || value == NULL) {
        return ERR_UNKNOWN;
    }

    int i = name_to_index(var_name);

    if (i != NOT_FOUND) {
        zx_assign_string(machine->string_variables[i].data.string.text,
                 machine->string_variables[i].data.string.length,
                 value);
        if (value->type != ZX_TYPE_STRING) {
            return ERR_2_VARIABLE_NOT_FOUND;
        }
        return ERR_0_OK;
    }
    return ERR_2_VARIABLE_NOT_FOUND;
}
void machine_set_rng_state(ZxMachine machine, uint32_t state) {
    if (machine != NULL) {
        machine->rng_state = state;
    }
}
uint32_t machine_get_rng_state(ZxMachine machine) {
    if (machine != NULL) {
        return machine->rng_state;
    }
    return 0;
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
void machine_set_text_cursor_y(ZxMachine machine, const uint8_t y) {
    if (machine != NULL) {
        machine->text_cursor_y = y;
    }
}
void machine_set_print_callback(ZxMachine machine, ZxPrintCallback callback) {
    if (machine != NULL) {
        machine->print_callback = callback;
    }
}
void machine_clear_text_screen(ZxMachine machine) {
    if (machine != NULL) {
        memset(&machine->text_screen[0][0], ' ', 22 * 32);
        machine->text_cursor_x = 0;
        machine->text_cursor_y = 0;
    }
}
const uint8_t* machine_get_text_screen(ZxMachine machine) {
    if (machine != NULL) {
        return &machine->text_screen[0][0]; // Pointer naar de allereerste pixel
    }
    return NULL;
}
const uint8_t* machine_get_from_text_screen(ZxMachine machine, const uint8_t y, const uint8_t x) {
    if (machine != NULL) {
        return &machine->text_screen[y][x];
    }
    return NULL;
}
const uint8_t* machine_get_from_system_screen(ZxMachine machine, const uint8_t y, const uint8_t x) {
    if (machine != NULL) {
        return &machine->system_screen[y][x];
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
static void machine_print_to_text(ZxMachine machine, const uint8_t *tokens, size_t len) {
    assert(tokens != NULL && "tokens mag nooit NULL zijn in deze interne functie");
    if (machine == NULL || len == 0) return;

    for (size_t i = 0; i < len; i++) {
        uint8_t token = tokens[i];

        if (token == '\n' || token == 13) {
            machine_next_line(machine);
            continue;
        }

        if (token >= 165) {
            const char *keyword_text = get_content_from_token(token);
            size_t kw_len = strlen(keyword_text);

            for (size_t j = 0; j < kw_len; j++) {
                machine->text_screen[machine->text_cursor_y][machine->text_cursor_x] = (uint8_t)keyword_text[j];
                if (machine->text_cursor_x >= 31) {
                    machine_next_line(machine);
                } else {
                    machine->text_cursor_x++;
                }
            }
            continue;
        }

        machine->text_screen[machine->text_cursor_y][machine->text_cursor_x] = token;

        if (machine->text_cursor_x >= 31) {
            machine_next_line(machine);
        } else {
            machine->text_cursor_x++;
        }
    }

}
void machine_print_value(ZxMachine machine, const ZxValue value) {
    if (machine == NULL) return;

    if (value.type == ZX_TYPE_STRING) {
        uint8_t *tokens = NULL;
        size_t tokens_len = 0;

        zx_get_string(value, &tokens, &tokens_len);

        machine_print_to_text(machine, tokens, tokens_len);
    }
    else if (value.type == ZX_TYPE_NUMBER) {
        double number;
        zx_get_number(value, &number);

        uint8_t num_buffer[32];
        size_t tokens_len = 0;

        if (formatted_number(number, num_buffer, sizeof(num_buffer), &tokens_len) == ERR_0_OK) {
            machine_print_to_text(machine, num_buffer, tokens_len);
        }
    }
    else {
        uint8_t fallback_token = 32;

        machine_print_to_text(machine, &fallback_token, 1);
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
