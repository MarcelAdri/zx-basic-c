//
// Created by Marcel on 06-08-2026.
//

#ifndef ZX_BASIC_C_SCREEN_H
#define ZX_BASIC_C_SCREEN_H
#include <stdbool.h>
#include <stdint.h>

#include "errors.h"

#define MAIN_SCREEN_ROWS 22

typedef struct {
    uint8_t character; // Het ZX Spectrum/ASCII teken
    uint8_t attribute; // Bit 7: FLASH, Bit 6: BRIGHT, Bit 3-5: PAPER, Bit 0-2: INK
} ZxCell;

// Attribuut-status van de machine (Permanent vs Tijdelijk voor PRINT)
typedef struct {
    uint8_t ink;       // 0-7, of 8 (transparent)
    uint8_t paper;     // 0-7, of 8 (transparent)
    uint8_t flash;     // 0 (off), 1 (on), of 8 (transparent)
    uint8_t bright;    // 0 (off), 1 (on), of 8 (transparent)
    uint8_t inverse;   // 0 (off), 1 (on)
    uint8_t over;      // 0 (off), 1 (on)
} ZxPrintAttributes;

typedef struct ZxScreen *ZxScreen;

ZxScreen screen_create(void);
void screen_destroy(ZxScreen screen);
void screen_clear(ZxScreen screen);

void screen_set_perm_attrs(ZxScreen screen, const ZxPrintAttributes *attrs);
ZxPrintAttributes screen_get_perm_attrs(ZxScreen screen);
ZxError screen_set_perm_flash(ZxScreen screen, uint8_t flash);
ZxError screen_set_perm_bright(ZxScreen screen, uint8_t bright);
ZxError screen_set_perm_ink(ZxScreen screen, uint8_t ink);
ZxError screen_set_perm_paper(ZxScreen screen, uint8_t paper);
ZxError screen_set_perm_inverse(ZxScreen screen, uint8_t inverse);
ZxError screen_set_perm_over(ZxScreen screen, uint8_t over);

void screen_set_temp_attrs(ZxScreen screen, const ZxPrintAttributes *attrs);
ZxPrintAttributes screen_get_temp_attrs(ZxScreen screen);
ZxError screen_set_temp_flash(ZxScreen screen, uint8_t flash);
ZxError screen_set_temp_bright(ZxScreen screen, uint8_t bright);
ZxError screen_set_temp_ink(ZxScreen screen, uint8_t ink);
ZxError screen_set_temp_paper(ZxScreen screen, uint8_t paper);
ZxError screen_set_temp_inverse(ZxScreen screen, uint8_t inverse);
ZxError screen_set_temp_over(ZxScreen screen, uint8_t over);
void screen_reset_temp_attrs(ZxScreen screen);

bool screen_put_txt_char(ZxScreen screen, uint8_t character);
bool screen_txt_new_line(ZxScreen screen);
bool screen_txt_advance_x(ZxScreen screen);
void screen_set_txt_cursor(ZxScreen screen, uint8_t y, uint8_t x);
uint8_t screen_get_txt_cursor_x(ZxScreen screen);
uint8_t screen_get_txt_cursor_y(ZxScreen screen);

void screen_clear_sys(ZxScreen screen);
void screen_put_sys_char(ZxScreen screen, uint8_t character);
void screen_set_sys_cursor(ZxScreen screen, uint8_t y, uint8_t x);
uint8_t screen_get_sys_cursor_x(ZxScreen screen);
uint8_t screen_get_sys_cursor_y(ZxScreen screen);

const ZxCell* screen_get_cell(ZxScreen screen, int y, int x);
const ZxCell* screen_get_buffer(ZxScreen screen);

#endif //ZX_BASIC_C_SCREEN_H
