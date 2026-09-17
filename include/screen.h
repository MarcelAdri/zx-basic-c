//
// Created by Marcel on 06-08-2026.
//

#ifndef ZX_BASIC_C_SCREEN_H
#define ZX_BASIC_C_SCREEN_H
#include <stdbool.h>
#include <stdint.h>

#include "errors.h"
#include "machine.h"

#define MAIN_SCREEN_ROWS 22

ZxError screen_init(ZxMachine machine);
ZxError screen_clear(ZxMachine machine);

ZxError screen_set_flash(ZxMachine machine, uint8_t flash, bool is_permanent);
ZxError screen_set_bright(ZxMachine machine, uint8_t bright, bool is_permanent);
ZxError screen_set_ink(ZxMachine machine, uint8_t ink, bool is_permanent);
ZxError screen_set_paper(ZxMachine machine, uint8_t paper, bool is_permanent);
ZxError screen_set_inverse(ZxMachine machine, uint8_t inverse, bool is_permanent);
ZxError screen_set_over(ZxMachine machine, uint8_t over, bool is_permanent);

ZxError screen_reset_temp_attrs(ZxMachine machine);

ZxError screen_put_txt_char(ZxMachine machine, uint8_t character, bool *scroll);
ZxError screen_txt_new_line(ZxMachine machine, bool *scroll);
ZxError screen_txt_advance_x(ZxMachine machine, bool *scroll);
ZxError screen_set_txt_cursor(ZxMachine machine, uint8_t y, uint8_t x);
ZxError screen_get_txt_cursor_x(ZxMachine machine, uint8_t *x);
ZxError screen_get_txt_cursor_y(ZxMachine machine, uint8_t *y);

ZxError screen_clear_sys(ZxMachine machine);
ZxError screen_put_sys_char_attr(ZxMachine machine, uint8_t character, uint8_t attr);
ZxError screen_put_sys_char(ZxMachine machine, uint8_t character);
ZxError screen_set_sys_cursor(ZxMachine machine, uint8_t rel_y, uint8_t x);
ZxError screen_get_sys_cursor_x(ZxMachine machine, uint8_t *x);
ZxError screen_get_sys_cursor_y(ZxMachine machine, uint8_t *y);

ZxError screen_get_char(ZxMachine machine, int y, int x, uint8_t *character);
ZxError screen_get_attr(ZxMachine machine, int y, int x, uint8_t *attributes);
uint32_t* screen_get_framebuffer(void);
void screen_render_frame(ZxMachine machine, bool flash_state);
ZxError screen_plot(ZxMachine machine, uint8_t x, uint8_t y);

#endif //ZX_BASIC_C_SCREEN_H
