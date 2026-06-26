//
// Created by Marcel on 17-05-2026.
//

#ifndef ZX_BASIC_C_MACHINE_H
#define ZX_BASIC_C_MACHINE_H
#include "errors.h"
#include "zx_types.h"
#include <stdbool.h>

#define MAX_TEXT_SENTENCE_LEN 256
#define MAX_TOKEN_SENTENCE_LEN 200

typedef struct Machine* ZxMachine;
typedef void (*ZxPrintCallback)(const char *text);

typedef struct {
    uint8_t *tokens;
    size_t length;
    bool exists;
} ZxLine;

typedef enum {
    ZX_STATE_IDLE = 0,      // Wacht op een commando onderin beeld
    ZX_STATE_DIRECT = 1,
    ZX_STATE_RUNNING = 2,       // Bezig met het uitvoeren van BASIC (of een commando)
} ZxState;

typedef enum {
    ZX_WAIT_NONE = 0,
    ZX_WAIT_SCROLL = 1,
    ZX_WAIT_INPUT = 2,
    ZX_WAIT_PAUSE = 3
} ZxWaitReason;

ZxMachine machine_create(void);
uint8_t machine_get_text_cursor_x(ZxMachine machine);
uint8_t machine_get_text_cursor_y(ZxMachine machine);
void machine_set_text_cursor_x(ZxMachine machine, uint8_t x);
void machine_set_text_cursor_y(ZxMachine machine, uint8_t y);
void machine_clear_text_screen(ZxMachine machine);
const uint8_t* machine_get_text_screen(ZxMachine machine);
const uint8_t* machine_get_from_text_screen(ZxMachine machine, uint8_t y, uint8_t x);
const uint8_t* machine_get_from_system_screen(ZxMachine machine, uint8_t y, uint8_t x);
ZxState machine_get_state(ZxMachine machine);
ZxWaitReason machine_get_wait_reason(ZxMachine machine);
void machine_set_wait_reason(ZxMachine machine, ZxWaitReason reason);
uint16_t machine_get_wait_resume_line(ZxMachine machine);
void machine_set_wait_resume_line(ZxMachine machine, uint16_t line);
void machine_set_state(ZxMachine machine, ZxState state);
void machine_set_location(ZxMachine machine, uint16_t line, uint8_t statement);
ZxError machine_delete_line(ZxMachine machine, uint16_t line_number);
ZxError machine_insert_line(ZxMachine machine, uint16_t line_number, const uint8_t *tokens, size_t length);
uint16_t machine_get_current_line(ZxMachine machine);
void machine_set_current_line(ZxMachine machine, uint16_t line_number);
void machine_set_current_statement(ZxMachine machine, uint8_t statement);
void machine_set_old_line(ZxMachine machine);
uint16_t machine_get_old_line(ZxMachine machine);
uint8_t machine_get_old_statement(ZxMachine machine);
void machine_set_direct_buffer(ZxMachine machine, const uint8_t *buffer, size_t buffer_length);
const uint8_t* machine_get_direct_buffer(ZxMachine machine, size_t *buffer_length);
void machine_retrieve_current_edit_line(ZxMachine machine, uint16_t *line_number, uint8_t *line, size_t *line_length);
const uint8_t* machine_retrieve_program_line(ZxMachine machine, uint16_t *line_number, size_t *line_size);
uint16_t machine_get_next_line(ZxMachine machine, uint16_t line_num);
uint8_t machine_get_current_statement(ZxMachine machine);
void machine_set_current_edit_line(ZxMachine machine, uint16_t line_number);
uint16_t machine_get_current_edit_line(ZxMachine machine);
void machine_set_top_line_in_list(ZxMachine machine, uint16_t line_number);
uint16_t machine_get_top_line_in_list(ZxMachine machine);
ZxLine* machine_get_program(ZxMachine machine);
void machine_set_rng_state(ZxMachine machine, uint32_t state);
uint32_t machine_get_rng_state(ZxMachine machine);
void machine_clear_variables(ZxMachine machine);
void machine_reset(ZxMachine machine);
void machine_destroy(ZxMachine machine);
void machine_next_line(ZxMachine machine);
void machine_set_print_callback(ZxMachine machine, ZxPrintCallback callback);
void machine_print_value(ZxMachine machine, ZxValue value);
const uint8_t* machine_get_system_screen(ZxMachine machine);
void machine_print_to_system(ZxMachine machine, const char *text);
uint32_t machine_get_frames(ZxMachine machine);
void machine_tick_frame(ZxMachine machine);
uint32_t machine_get_pause_start_frame(ZxMachine machine);
void machine_set_pause_start_frame(ZxMachine machine, uint32_t frame);
int machine_get_pause_length(ZxMachine machine);
void machine_set_pause_length(ZxMachine machine, int length);
void machine_set_pressed_key(ZxMachine machine, uint8_t token);
uint8_t machine_get_pressed_key(ZxMachine machine);
ZxError machine_get_variable(ZxMachine machine, const char *var_name, const uint16_t *indices, uint8_t num_indices_passed, int32_t desired_len, ZxValue *value);
ZxError machine_set_variable(ZxMachine machine, const char *var_name, const uint16_t *indices, uint8_t num_indices_passed, int32_t desired_len, ZxValue value);

#endif //ZX_BASIC_C_MACHINE_H
