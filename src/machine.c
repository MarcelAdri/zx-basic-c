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
    char name[MAX_VAR_NAME_LEN];       // We reserveren max. 99 tekens voor de naam (+ '\0')
    ZxValue value;
} NumericVariable;



typedef struct Machine {
    ZxState state;
    ZxWaitReason wait_reason;
    uint16_t wait_reason_resume_line;

    uint8_t text_screen[22][32];
    uint8_t system_screen[2][32];

    uint8_t text_cursor_x;
    uint8_t text_cursor_y;

    ZxLine program_memory[10000];
    uint16_t current_edit_line;
    uint16_t top_line_in_list;
    size_t used_basic_ram;

    uint16_t current_line;
    uint8_t current_statement;

    uint16_t old_line;
    uint8_t old_statement;

    uint8_t direct_buffer[2048];
    size_t direct_length;

    ZxValue string_variables[26];

    NumericVariable *numeric_vars;
    int numeric_variable_count;
    size_t numeric_variable_capacity;

    ZxPrintCallback print_callback;

    uint32_t rng_state;
    uint32_t frame_counter;

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

    machine_reset(machine);

    return machine;
}
ZxState machine_get_state(ZxMachine machine) {
    if (machine != NULL) {
        return (int)machine->state;
    }
    return ZX_STATE_IDLE;
}
void machine_set_state(ZxMachine machine, const ZxState state) {
    if (machine != NULL) {
        machine->state = state;
    }
}
ZxWaitReason machine_get_wait_reason(ZxMachine machine) {
    if (machine != NULL) {
        return machine->wait_reason;
    }
    return ZX_WAIT_NONE;
}
void machine_set_wait_reason(ZxMachine machine, const ZxWaitReason reason) {
    if (machine != NULL) {
        machine->wait_reason = reason;
    }
}
uint16_t machine_get_wait_resume_line(ZxMachine machine) {
    if (machine != NULL) {
        return machine->wait_reason_resume_line;
    }
    return 0;
}
void machine_set_wait_resume_line(ZxMachine machine, const uint16_t line) {
    if (machine != NULL) {
        machine->wait_reason_resume_line = line;
    }
}
void machine_set_location(ZxMachine machine, const uint16_t line, const uint8_t statement) {
    if (machine) {
        machine->current_line = line;
        machine->current_statement = statement;
    }
}
ZxError machine_delete_line(ZxMachine machine, const uint16_t line_number) {
    if (machine == NULL) {
        return ERR_UNKNOWN;
    }
    if (machine->program_memory[line_number].exists) {
        free(machine->program_memory[line_number].tokens);
        machine->program_memory[line_number].exists = false;
        machine->program_memory[line_number].length = 0;
    }
    machine_set_current_edit_line(machine, line_number);
    return ERR_0_OK;
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

    machine->program_memory[line_number].tokens = malloc(length + 1);
    memcpy(machine->program_memory[line_number].tokens, tokens, length);
    machine->program_memory[line_number].tokens[length] = '\0';
    machine->program_memory[line_number].length = length;
    machine->program_memory[line_number].exists = true;

    machine->used_basic_ram = projected_ram;

    return ERR_0_OK;
}
uint16_t machine_get_current_line(ZxMachine machine) {
    return machine ? machine->current_line : 0;
}
void machine_set_current_line(ZxMachine machine, uint16_t line_number) {
    if (machine == NULL) return;
    machine->current_line = line_number;
}
uint8_t machine_get_current_statement(ZxMachine machine) {
    return machine ? machine->current_statement : 1;
}
void machine_set_current_statement(ZxMachine machine, uint8_t statement) {
    if (machine == NULL) return;
    machine->current_statement = statement;
}
void machine_set_old_line(ZxMachine machine) {
    machine->old_line = machine->current_line;
    machine->old_statement = machine->current_statement;
}
uint16_t machine_get_old_line(ZxMachine machine) {
    return machine ? machine->old_line : 0;
}
uint8_t machine_get_old_statement(ZxMachine machine) {
    return machine ? machine->old_statement : 1;
}
void machine_set_direct_buffer(ZxMachine machine, const uint8_t *buffer, size_t buffer_length) {
    if (machine == NULL || buffer == NULL) return;

    size_t max_size = sizeof(machine->direct_buffer);
    if (buffer_length > max_size) {
        buffer_length = max_size;
    }
    memset(machine->direct_buffer, 0, max_size);
    memcpy(machine->direct_buffer, buffer, buffer_length);
    machine->direct_length = buffer_length;

    machine_set_current_line(machine, 0);
    machine_set_current_statement(machine, 1);
}
const uint8_t* machine_get_direct_buffer(ZxMachine machine, size_t *buffer_length) {
    if (machine == NULL) {
        if (buffer_length != NULL) *buffer_length = 0;
        return NULL;
    }

    if (buffer_length != NULL) {
        *buffer_length = machine->direct_length;
    }

    return machine->direct_buffer;
}
void machine_set_current_edit_line(ZxMachine machine, uint16_t line_number) {
    if (machine != NULL) {
        machine->current_edit_line = line_number;
    }
}
uint16_t machine_get_current_edit_line(ZxMachine machine) {
    return machine != NULL ? machine->current_edit_line : 0;
}
void machine_retrieve_current_edit_line(ZxMachine machine, uint16_t *line_number, uint8_t *line, size_t *line_length) {
    if (machine == NULL || line == NULL || line_length == NULL || line_number == NULL) {
        return;
    }

    uint16_t edit_line = machine_get_current_edit_line(machine);

    // 1. Vang het interne adres op in een TIJDELIJKE pointer
    const uint8_t *internal_tokens = machine_retrieve_program_line(machine, &edit_line, line_length);

    // 2. Kopieer van het interne geheugen naar de veilige buffer ('line')
    if (*line_length > 0 && internal_tokens != NULL) {
        memcpy(line, internal_tokens, *line_length);
    }

    *line_number = edit_line;

    // 3. Nu de data veilig is gekopieerd naar de 'line' buffer,
    // mogen we de originele interne regel veilig wissen!
    machine_delete_line(machine, edit_line);

    // 4. Update de lijst
    list_program(&machine, 0, true);
}
const uint8_t* machine_retrieve_program_line(ZxMachine machine, uint16_t *line_number, size_t *line_size) {
    if (machine == NULL || line_number == NULL || line_size == NULL) {
        return NULL;
    }

    uint16_t line_num = *line_number;

    // 1. Zoek vooruit
    while (line_num < 10000 && !machine->program_memory[line_num].exists) {
        line_num++;
    }

    // 2. Fallback achteruit (Let op: start bij 9999, wegens array bounds!)
    if (line_num >= 10000) {
        line_num = 9999;
        while (line_num > 0 && !machine->program_memory[line_num].exists) {
            line_num--;
        }
    }

    // Als er écht niets is
    if (line_num == 0 && !machine->program_memory[0].exists) {
        return NULL;
    }

    // 3. CRUCIAL: Eerst de data veiligstellen in de pointers...
    *line_number = line_num;
    *line_size = machine->program_memory[line_num].length;

    // ...en de VOLLEDIGE array kopiëren met memcpy!
    if (*line_size > 0 && machine->program_memory[line_num].tokens != NULL) {
        return machine->program_memory[line_num].tokens;
    }
    return NULL;
}
uint16_t machine_get_next_line(ZxMachine machine, const uint16_t line_num) {
    uint16_t next_line = line_num;
    next_line++;
    while (next_line < 10000 && !machine->program_memory[next_line].exists) {
        next_line++;
    }
    if (next_line <= 9999 && machine->program_memory[next_line].exists) {
        return next_line;
    }
    return 0;
}
void machine_set_top_line_in_list(ZxMachine machine, const uint16_t line_number) {
    if (machine != NULL) {
        machine->top_line_in_list = line_number;
    }
}
uint16_t machine_get_top_line_in_list(ZxMachine machine) {
    return machine != NULL ? machine->top_line_in_list : 0;
}
ZxLine* machine_get_program(ZxMachine machine) {
    if (machine == NULL) {
        return NULL;
    }
    return machine->program_memory;
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

        size_t new_capacity = (machine->numeric_variable_capacity == 0) ? 4 : (machine->numeric_variable_capacity * 2);

        NumericVariable* new_array = realloc(machine->numeric_vars, new_capacity * sizeof(NumericVariable));

        if (new_array == NULL) {
            return ERR_4_OUT_OF_MEMORY;
        }

        machine->numeric_vars = new_array;
        machine->numeric_variable_capacity = new_capacity;
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
void machine_clear_variables(ZxMachine machine) {
    machine->numeric_variable_count = 0;
    if (machine->numeric_vars != NULL) {
        free(machine->numeric_vars);
    }
    machine->numeric_vars = NULL;
    machine->numeric_variable_capacity = 0;
    machine->used_basic_ram = 0;

    //TODO: Counter, arrays
}
void machine_reset(ZxMachine machine) {
    if (machine == NULL) return;

    ZxLine* program = machine_get_program(machine);
    if (program != NULL) {
        for (uint16_t i = 0; i < 10000; i++) {
            if (program[i].exists) {
                if (program[i].tokens != NULL) {
                    free(program[i].tokens);
                    program[i].tokens = NULL;
                }
                program[i].exists = false;
                program[i].length = 0;
            }
        }
    }

    machine_clear_text_screen(machine);
    machine_print_to_system(machine, ""); // Wis eventuele scroll? of error meldingen

    // --- 3. Reset de machine status ---
    machine_set_state(machine, ZX_STATE_IDLE);
    machine->wait_reason = ZX_WAIT_NONE;

    machine_clear_variables(machine);

    machine->current_line = 0;
    machine->current_statement = 1;
    machine->current_edit_line = 0;
    machine_set_rng_state(machine, 12345);
    machine->frame_counter = 0;

    //TODO: GOSUB stack
}
void machine_destroy(ZxMachine machine) {
    if (machine != NULL) {
        machine_reset(machine);

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
        machine->wait_reason = ZX_WAIT_SCROLL;
        memmove(&machine->text_screen[0][0], &machine->text_screen[1][0], 21 * 32); //TODO: move screen update to resume
        memset(&machine->text_screen[21][0], ' ', 32);
        machine->text_cursor_y = 21;
    }

}
static void machine_print_to_text(ZxMachine machine, const uint8_t *tokens, size_t len) {
    assert(tokens != NULL && "tokens mag nooit NULL zijn in deze interne functie");
    if (machine == NULL || len == 0) return;

    char formatted_text[2048] = {0};
    ZxError err = build_zx_sentence(tokens, len, formatted_text);
    if (err != ERR_0_OK) return;

    size_t formatted_len = strlen(formatted_text);

    for (size_t i = 0; i < formatted_len; i++) {
        char c = formatted_text[i];

        if (c == '\n' || c == '\r') {
            machine_next_line(machine);
            continue;
        }

        machine->text_screen[machine->text_cursor_y][machine->text_cursor_x] = (uint8_t)c;

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
uint32_t machine_get_frames(ZxMachine machine) {
    if (machine == NULL) return 0;
    return machine->frame_counter;
}
void machine_tick_frame(ZxMachine machine) {
    if (machine == NULL) return;
    machine->frame_counter++;
}
