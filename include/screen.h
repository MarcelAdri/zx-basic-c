//
// Created by Marcel on 06-08-2026.
//

#ifndef ZX_BASIC_C_SCREEN_H
#define ZX_BASIC_C_SCREEN_H
#include <stdbool.h>
#include <stdint.h>

#include "errors.h"

#define MAIN_SCREEN_ROWS 22

typedef uint8_t* ZxScreen;

void screen_init(ZxScreen screen);
void screen_clear(ZxScreen screen);

ZxError screen_set_flash(ZxScreen screen, uint8_t flash, bool is_permanent);
ZxError screen_set_bright(ZxScreen screen, uint8_t bright, bool is_permanent);
ZxError screen_set_ink(ZxScreen screen, uint8_t ink, bool is_permanent);
ZxError screen_set_paper(ZxScreen screen, uint8_t paper, bool is_permanent);
ZxError screen_set_inverse(ZxScreen screen, uint8_t inverse, bool is_permanent) ;
ZxError screen_set_over(ZxScreen screen, uint8_t over, bool is_permanent);

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

uint8_t screen_get_char(ZxScreen screen, int y, int x);
uint8_t screen_get_attr(ZxScreen screen, int y, int x);
const uint8_t* screen_get_chars_buffer(ZxScreen screen);
const uint8_t* screen_get_attrs_buffer(ZxScreen screen);

#endif //ZX_BASIC_C_SCREEN_H
