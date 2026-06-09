//
// Created by Marcel on 17-05-2026.
//

#ifndef ZX_BASIC_C_MACHINE_H
#define ZX_BASIC_C_MACHINE_H
#include "errors.h"
#include "zx_types.h"
#include <stdbool.h>

#define MAX_VAR_NAME_LEN 100
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
    ZX_STATE_RUNNING = 1,       // Bezig met het uitvoeren van BASIC (of een commando)
    ZX_STATE_WAIT_SCROLL = 2,   // Scherm is vol, wacht op Y/N/SPACE
    ZX_STATE_WAIT_INPUT = 3,    // BASIC programma staat stil door een INPUT commando
    ZX_STATE_WAIT_PAUSE = 4     // BASIC programma staat stil door PAUSE commando
} ZxState;

typedef enum {
    ZX_SCROLL_REASON_NONE = 0,
    ZX_SCROLL_REASON_LIST,
    ZX_SCROLL_REASON_RUN,
} ZxScrollReason;

ZxMachine machine_create(void);
ZxError machine_set_numeric(ZxMachine machine, const char *var_name, ZxValue value);
ZxError machine_get_numeric(ZxMachine machine, const char *var_name, ZxValue *value);
ZxError machine_set_string(ZxMachine machine, uint8_t var_name, ZxValue *value);
ZxError machine_get_string(ZxMachine machine, uint8_t var_name, ZxValue *value);
uint8_t machine_get_text_cursor_x(ZxMachine machine);
uint8_t machine_get_text_cursor_y(ZxMachine machine);
void machine_set_text_cursor_x(ZxMachine machine, uint8_t x);
void machine_set_text_cursor_y(ZxMachine machine, uint8_t y);
void machine_clear_text_screen(ZxMachine machine);
const uint8_t* machine_get_text_screen(ZxMachine machine);
const uint8_t* machine_get_from_text_screen(ZxMachine machine, uint8_t y, uint8_t x);
const uint8_t* machine_get_from_system_screen(ZxMachine machine, uint8_t y, uint8_t x);
ZxState machine_get_state(ZxMachine machine);
ZxScrollReason machine_get_scroll_reason(ZxMachine machine);
void machine_set_scroll_reason(ZxMachine machine, const ZxScrollReason reason);
uint16_t machine_get_scroll_resume_line(ZxMachine machine);
void machine_set_scroll_resume_line(ZxMachine machine, const uint16_t line);
void machine_set_state(ZxMachine machine, const ZxState state);
void machine_set_location(ZxMachine machine, uint16_t line, uint8_t statement);
ZxError machine_insert_line(ZxMachine machine, uint16_t line_number, const uint8_t *tokens, size_t length);
uint16_t machine_get_current_line(ZxMachine machine);
uint8_t machine_get_current_statement(ZxMachine machine);
void machine_set_current_edit_line(ZxMachine machine, uint16_t line_number);
uint16_t machine_get_current_edit_line(ZxMachine machine);
void machine_set_top_line_in_list(ZxMachine machine, uint16_t line_number);
uint16_t machine_get_top_line_in_list(ZxMachine machine);
ZxLine* machine_get_program(ZxMachine machine);
void machine_set_rng_state(ZxMachine machine, uint32_t state);
uint32_t machine_get_rng_state(ZxMachine machine);
void machine_reset(ZxMachine machine);
void machine_destroy(ZxMachine machine);
void machine_next_line(ZxMachine machine);
void machine_set_print_callback(ZxMachine machine, ZxPrintCallback callback);
void machine_print_value(ZxMachine machine, ZxValue value);
const uint8_t* machine_get_system_screen(ZxMachine machine);
void machine_print_to_system(ZxMachine machine, const char *text);

#endif //ZX_BASIC_C_MACHINE_H
