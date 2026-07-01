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






typedef struct Machine {
    ZxState state;
    ZxWaitReason wait_reason;
    uint16_t wait_reason_resume_line;

    uint32_t pause_start_frame;
    int pause_length;

    uint8_t text_screen[22][32];
    uint8_t system_screen[2][32];

    uint8_t text_cursor_x;
    uint8_t text_cursor_y;

    uint8_t current_pressed_key;

    ZxLine program_memory[10000];
    uint16_t current_edit_line;
    uint16_t top_line_in_list;
    size_t used_basic_ram;

    uint16_t current_line;
    uint8_t current_statement;

    uint16_t old_line;
    uint8_t old_statement;

    ZxGoSub go_sub_stack[MAX_GO_SUB_STACK_SIZE];
    uint8_t go_sub_stack_index;

    uint8_t direct_buffer[2048];
    size_t direct_length;

    ZxStringSlot string_variables[26];
    ZxNumericArray numeric_arrays[26];
    ZxLoopControl loops[26];

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
static NumericVariable* find_or_create_numeric_variable(ZxMachine machine, const char *name) {
    // 1. Bestaat hij al?
    int i = get_numeric_variable_index(machine, name, MAX_VAR_NAME_LEN);
    if (i != NOT_FOUND) {
        return &machine->numeric_vars[i];
    }

    // 2. Nee? Check capaciteit en groei indien nodig
    if (machine->numeric_variable_count >= machine->numeric_variable_capacity) {
        size_t new_capacity = (machine->numeric_variable_capacity == 0) ? 4 : (machine->numeric_variable_capacity * 2);
        NumericVariable* new_array = realloc(machine->numeric_vars, new_capacity * sizeof(NumericVariable));
        if (new_array == NULL) return NULL;

        machine->numeric_vars = new_array;
        machine->numeric_variable_capacity = new_capacity;
    }

    // 3. Initialiseer de nieuwe variabele
    int nieuw_index = machine->numeric_variable_count;
    strncpy(machine->numeric_vars[nieuw_index].name, name, MAX_VAR_NAME_LEN - 1);
    machine->numeric_vars[nieuw_index].name[MAX_VAR_NAME_LEN - 1] = '\0';
    machine->numeric_vars[nieuw_index].value = 0.0; // Veilige default

    machine->numeric_variable_count++;
    return &machine->numeric_vars[nieuw_index];
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
        size_t old_line_cost = machine->program_memory[line_number].length + 5;
        if (machine->used_basic_ram >= old_line_cost) {
            machine->used_basic_ram -= old_line_cost;
        } else {
            machine->used_basic_ram = 0; // Veiligheidsvangnet tegen onder de 0 gaan
        }
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
    machine->current_pressed_key = 0;

    for (int i = 0; i < 26; i++) {
        zx_free_numeric_array(&machine->numeric_arrays[i]);
        zx_free_string_slot(&machine->string_variables[i]);
        loop_init(&machine->loops[i]);
    }

    machine->go_sub_stack_index = 0;

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

    machine->current_pressed_key = 0;

    machine->used_basic_ram = 0;
    machine->current_line = 0;
    machine->current_statement = 1;
    machine->current_edit_line = 0;
    machine_set_rng_state(machine, 12345);
    machine->frame_counter = 0;
    machine->pause_start_frame = 0;
    machine->pause_length = -1;

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

    if (machine_get_wait_reason(machine) == ZX_WAIT_PAUSE) {
        uint32_t start_frame = machine_get_pause_start_frame(machine);
        uint32_t current_frame = machine_get_frames(machine);
        int pause_length = machine_get_pause_length(machine);
        if (pause_length == -1 || pause_length == 0) {
            return;
        }
        if ((current_frame - start_frame) >= pause_length) {
            machine_set_wait_reason(machine, ZX_WAIT_NONE);
            machine_set_pause_length(machine, -1);
            machine_set_pause_start_frame(machine, 0);
        }
        return;
    }
}
uint32_t machine_get_pause_start_frame(ZxMachine machine) {
    if (machine == NULL) return 0;
    return machine->pause_start_frame;
}
void machine_set_pause_start_frame(ZxMachine machine, uint32_t frame) {
    if (machine == NULL) return;
    machine->pause_start_frame = frame;
}
int machine_get_pause_length(ZxMachine machine) {
    if (machine == NULL) return 0;
    return machine->pause_length;
}
void machine_set_pause_length(ZxMachine machine, int length) {
    if (machine == NULL) return;
    machine->pause_length = length;
}
void machine_set_pressed_key(ZxMachine machine, const uint8_t token) {
    if (machine) machine->current_pressed_key = token;
}
uint8_t machine_get_pressed_key(ZxMachine machine) {
    return machine ? machine->current_pressed_key : 0;
}
//Variable getter en setter
ZxError machine_get_variable(ZxMachine machine, const char *var_name, const uint16_t *indices, const uint8_t num_indices_passed, const int32_t desired_len, ZxValue *value) {
    if (machine == NULL || var_name == NULL || value == NULL) {
        return ERR_UNKNOWN;
    }
    //1. string variabelen
    if (strlen(var_name) == 2 && var_name[1] == ZX_CHAR_DOLLAR) {
        int i = name_to_index((uint8_t)var_name[0]);
        if (i == NOT_FOUND) return ERR_2_VARIABLE_NOT_FOUND;
        if (!machine->string_variables[i].exists) return ERR_2_VARIABLE_NOT_FOUND;
        return zx_get_string_element(&machine->string_variables[i], indices, num_indices_passed, desired_len, value);
    }

    //2. numerieke variabelen
    //2a. enkele variabele
    if (num_indices_passed == 0) {
        int i = get_numeric_variable_index(machine, var_name, MAX_VAR_NAME_LEN);
        if (i != NOT_FOUND) {
            return zx_get_numeric_value(&machine->numeric_vars[i], value);
        }

        return ERR_2_VARIABLE_NOT_FOUND;
    }
    //2b. array-element
    if (strlen(var_name) != 1) return ERR_C_NONSENSE_IN_BASIC;
    int i = name_to_index((uint8_t)var_name[0]);
    if (i == NOT_FOUND) {
        return ERR_C_NONSENSE_IN_BASIC;
    }

    ZxNumericArray *array = &machine->numeric_arrays[i];

    if (!array->exists) return ERR_2_VARIABLE_NOT_FOUND;

    return zx_get_numeric_array_element(array, indices, num_indices_passed, value);

}
ZxError machine_set_variable(ZxMachine machine, const char *var_name, const uint16_t *indices, const uint8_t num_indices_passed, const int32_t desired_len, const ZxValue value) {
    if (machine == NULL || var_name == NULL) return ERR_UNKNOWN;

    // =========================================================================
    // 1. DOEL IS EEN STRING VARIABELE OF STRING ARRAY (bijv. "a$")
    // =========================================================================
    if (strlen(var_name) == 2 && var_name[1] == ZX_CHAR_DOLLAR) {

        if (value.type != ZX_TYPE_STRING) {
            return ERR_C_NONSENSE_IN_BASIC; // Sinclair type mismatch
        }

        int i = name_to_index((uint8_t)var_name[0]);
        if (i == NOT_FOUND) return ERR_C_NONSENSE_IN_BASIC;

        return zx_set_string_element(&machine->string_variables[i], indices, num_indices_passed, desired_len, value);
    }

    // =========================================================================
    // NUMERIEKE LOGICA (Vanaf hier weten we zeker dat het doel een getal/getallen-array is)
    // =========================================================================
    // TYPE CHECK: Als je naar een numeriek doel schrijft, MOET de waarde een getal zijn!
    if (value.type != ZX_TYPE_NUMBER) {
        return ERR_C_NONSENSE_IN_BASIC; // Sinclair type mismatch
    }

    // =========================================================================
    // 2. DOEL IS EEN GEWONE NUMERIEKE SCALAR (bijv. "LET snelheid = 10")
    // =========================================================================
    if (num_indices_passed == 0) {
        // We delegeren het zoeken of aanmaken naar een interne machine-helper!
        NumericVariable *var = find_or_create_numeric_variable(machine, var_name);
        if (var == NULL) return ERR_4_OUT_OF_MEMORY;

        return zx_set_numeric_value(var, value);
    }

    // =========================================================================
    // 3. DOEL IS EEN NUMERIEKE ARRAY (bijv. "LET a(1,2) = 10")
    // =========================================================================
    if (strlen(var_name) != 1) return ERR_C_NONSENSE_IN_BASIC;

    int i = name_to_index((uint8_t)var_name[0]);
    if (i == NOT_FOUND) return ERR_C_NONSENSE_IN_BASIC;

    ZxNumericArray *array = &machine->numeric_arrays[i];
    if (!array->exists) return ERR_2_VARIABLE_NOT_FOUND;

    return zx_set_numeric_array_element(array, indices, num_indices_passed, value);
}
ZxError machine_reserve_variable(ZxMachine machine, const char *var_name, const uint16_t *dimension_sizes, const uint8_t num_dimensions) {
    if (machine == NULL || var_name == NULL || dimension_sizes == NULL) return ERR_UNKNOWN;

    if (strlen(var_name) == 2 && var_name[1] == ZX_CHAR_DOLLAR) {

        int i = name_to_index((uint8_t)var_name[0]);
        if (i == NOT_FOUND) return ERR_C_NONSENSE_IN_BASIC;

        return zx_dim_string_slot(num_dimensions, dimension_sizes, &machine->string_variables[i]);
    }
    if (strlen(var_name) != 1) return ERR_C_NONSENSE_IN_BASIC;

    int i = name_to_index((uint8_t)var_name[0]);
    if (i == NOT_FOUND) return ERR_C_NONSENSE_IN_BASIC;

    return zx_dim_numeric_array(num_dimensions, dimension_sizes, &machine->numeric_arrays[i]);
}
//LoopControl
ZxError machine_loop_set(ZxMachine machine, const char *var_name, const uint16_t return_line, const uint8_t return_statement, const double end_value, const double step_value) {
    if (machine == NULL || var_name == NULL) return ERR_UNKNOWN;
    if (strlen(var_name) != 1) return ERR_C_NONSENSE_IN_BASIC;

    int i = name_to_index((uint8_t)var_name[0]);
    if (i == NOT_FOUND) return ERR_C_NONSENSE_IN_BASIC;

    loop_set(&machine->loops[i], return_line, return_statement, end_value, step_value);
    return ERR_0_OK;
}
ZxError machine_loop_get(ZxMachine machine, const char *var_name, ZxLoopControl **out_loop_control) {
    if (machine == NULL || var_name == NULL || out_loop_control == NULL) return ERR_UNKNOWN;
    if (strlen(var_name) != 1) return ERR_C_NONSENSE_IN_BASIC;

    int i = name_to_index((uint8_t)var_name[0]);
    if (i == NOT_FOUND) return ERR_C_NONSENSE_IN_BASIC;

    *out_loop_control = &machine->loops[i];
    return ERR_0_OK;
}
//GO SUB
ZxError machine_push_go_sub_stack(ZxMachine machine, const uint16_t line_number, const uint8_t statement) {
    if (machine == NULL) return ERR_UNKNOWN;
    if (machine->go_sub_stack_index >= MAX_GO_SUB_STACK_SIZE) return ERR_4_OUT_OF_MEMORY;
    machine->go_sub_stack[machine->go_sub_stack_index].return_line = line_number;
    machine->go_sub_stack[machine->go_sub_stack_index].return_statement = statement;
    machine->go_sub_stack_index++;
    return ERR_0_OK;
}
ZxError machine_pop_go_sub_stack(ZxMachine machine, uint16_t *out_line, uint8_t *out_statement) {
    if (machine == NULL || out_line == NULL || out_statement == NULL) return ERR_UNKNOWN;
    if (machine->go_sub_stack_index == 0) return ERR_7_NO_GOSUB;
    machine->go_sub_stack_index--;
    *out_line = machine->go_sub_stack[machine->go_sub_stack_index].return_line;
    *out_statement = machine->go_sub_stack[machine->go_sub_stack_index].return_statement;
    return ERR_0_OK;
}