//
// Created by Marcel on 06-08-2026.
//

#include "screen.h"
#include "characters.h"

#include <stdlib.h>
#include <string.h>



#define SCREEN_COLS 32
#define SYSTEM_SCREEN_ROWS 2
#define TOTAL_SCREEN_ROWS (MAIN_SCREEN_ROWS + SYSTEM_SCREEN_ROWS)
#define SYS_DEFAULT_ATTR 0x38

struct ZxScreen {
    ZxCell vram[TOTAL_SCREEN_ROWS][SCREEN_COLS]; // 1 aaneengesloten blok van 768 cellen!

    // Gescheiden cursors voor de twee zones!
    uint8_t main_cursor_x;
    uint8_t main_cursor_y; // 0 t/m 21

    uint8_t sys_cursor_x;
    uint8_t sys_cursor_y;  // 0 t/m 1 (relatief voor het systeemvak)

    //Attributen
    ZxPrintAttributes perm_attrs;
    ZxPrintAttributes temp_attrs;
};

static uint8_t combine_attributes(uint8_t current_vram_attr, const ZxPrintAttributes *attrs) {
    // 1. Bepaal INK (bij 8/transparent behouden we de oude INK van het scherm)
    uint8_t ink = (attrs->ink == 8) ? (current_vram_attr & 0x07) : attrs->ink;

    // 2. Bepaal PAPER (bij 8/transparent behouden we de oude PAPER)
    uint8_t paper = (attrs->paper == 8) ? ((current_vram_attr >> 3) & 0x07) : attrs->paper;

    uint8_t flash = (attrs->flash == 8) ? (current_vram_attr & 0x80) : (attrs->flash ? 0x80 : 0x00);
    uint8_t bright = (attrs->bright == 8) ? (current_vram_attr & 0x40) : (attrs->bright ? 0x40 : 0x00);

    // 3. Pas INVERSE toe (wissel INK en PAPER om)
    if (attrs->inverse) {
        uint8_t tmp = ink;
        ink = paper;
        paper = tmp;
    }

    // Pak alles samen in 1 authentieke byte
    return flash | bright | (paper << 3) | ink;
}
static ZxPrintAttributes separate_attributes(const uint8_t vram_attr) {
    ZxPrintAttributes attrs;
    attrs.flash = (vram_attr & 0x80) ? 1 : 0;
    attrs.bright = (vram_attr & 0x40) ? 1 : 0;
    attrs.inverse = 0;
    attrs.over = 0;
    attrs.ink = vram_attr & 0x07;
    attrs.paper = (vram_attr >> 3) & 0x07;
    return attrs;
}

// Life Cycle
ZxScreen screen_create(void) {
    // 1. Alloceer geheugen op de heap voor de schermstructuur
    ZxScreen screen = malloc(sizeof(struct ZxScreen));
    if (screen == NULL) return NULL;

    // 2. Zet alle velden (zoals de cursors) gegarandeerd op 0
    memset(screen, 0, sizeof(struct ZxScreen));

    // 3. Standaard ZX Spectrum opstart-attributen: PAPER 7 (wit), INK 0 (zwart)
    ZxPrintAttributes attrs;
    attrs.ink = 0;
    attrs.paper = 7;
    attrs.flash = 0;
    attrs.bright = 0;
    attrs.inverse = 0;
    attrs.over = 0;

    uint8_t vram_attrs = 0;

    screen_set_perm_attrs(screen, &attrs);
    screen_reset_temp_attrs(screen);

    // 4. Vul het hele VRAM met spaties en de standaard opstart-kleuren
    screen_clear(screen);

    return screen;
}

void screen_destroy(ZxScreen screen) {
    if (screen != NULL) {
        free(screen);
    }
}

void screen_clear(ZxScreen screen) {
    if (screen == NULL) return;

    screen_reset_temp_attrs(screen);

    uint8_t vram_attrs = 0;

    for (int y = 0; y < TOTAL_SCREEN_ROWS; y++) {
        for (int x = 0; x < SCREEN_COLS; x++) {
            screen->vram[y][x].character = ZX_CHAR_SPACE;
            if (y < MAIN_SCREEN_ROWS) {
                screen->vram[y][x].attribute = combine_attributes(vram_attrs, &screen->temp_attrs);
            } else {
                screen->vram[y][x].attribute = SYS_DEFAULT_ATTR;
            }
        }
    }

    screen->main_cursor_x = 0;
    screen->main_cursor_y = 0;
}

//Attribuut beheer permanent
void screen_set_perm_attrs(ZxScreen screen, const ZxPrintAttributes *attrs) {
    if (screen == NULL || attrs == NULL) return;

    screen->perm_attrs = *attrs;
}
ZxPrintAttributes screen_get_perm_attrs(ZxScreen screen) {
    if (screen == NULL) return (ZxPrintAttributes){0};
    return screen->perm_attrs;
}
ZxError screen_set_perm_flash(ZxScreen screen, const uint8_t flash) {
    if (screen == NULL) return ERR_UNKNOWN;

    if (flash != 0 && flash != 1 && flash != 8) {
        return ERR_UNKNOWN;
    }

    screen->perm_attrs.flash = flash;
    screen->temp_attrs.flash = flash;

    return ERR_0_OK;
}
ZxError screen_set_perm_bright(ZxScreen screen, const uint8_t bright) {
    if (screen == NULL) return ERR_UNKNOWN;

    if (bright != 0 && bright != 1 && bright != 8) {
        return ERR_UNKNOWN;
    }

    screen->perm_attrs.bright = bright;
    screen->temp_attrs.bright = bright;

    return ERR_0_OK;
}
ZxError screen_set_perm_ink(ZxScreen screen, const uint8_t ink) {
    if (screen == NULL) return ERR_UNKNOWN;

    if (ink > 8) {
        return ERR_UNKNOWN;
    }

    screen->perm_attrs.ink = ink;
    screen->temp_attrs.ink = ink;

    return ERR_0_OK;
}
ZxError screen_set_perm_paper(ZxScreen screen, const uint8_t paper) {
    if (screen == NULL) return ERR_UNKNOWN;

    if (paper > 8) {
        return ERR_UNKNOWN;
    }

    screen->perm_attrs.paper = paper;
    screen->temp_attrs.paper = paper;

    return ERR_0_OK;
}
ZxError screen_set_perm_inverse(ZxScreen screen, const uint8_t inverse) {
    if (screen == NULL) return ERR_UNKNOWN;
    if (inverse != 0 && inverse != 1) return ERR_UNKNOWN;

    screen->perm_attrs.inverse = inverse;
    screen->temp_attrs.inverse = inverse;
    return ERR_0_OK;
}
ZxError screen_set_perm_over(ZxScreen screen, const uint8_t over) {
    if (screen == NULL) return ERR_UNKNOWN;
    if (over != 0 && over != 1) return ERR_UNKNOWN;

    screen->perm_attrs.over = over;
    screen->temp_attrs.over = over;
    return ERR_0_OK;
}

//Attribuut beheer temp
void screen_set_temp_attrs(ZxScreen screen, const ZxPrintAttributes *attrs) {
    if (screen == NULL || attrs == NULL) return;

    screen->temp_attrs = *attrs;
}
ZxPrintAttributes screen_get_temp_attrs(ZxScreen screen) {
    if (screen == NULL) return (ZxPrintAttributes){0};
    return screen->temp_attrs;
}
ZxError screen_set_temp_flash(ZxScreen screen, const uint8_t flash) {
    if (screen == NULL) return ERR_UNKNOWN;

    if (flash != 0 && flash != 1 && flash != 8) {
        return ERR_UNKNOWN;
    }

    screen->temp_attrs.flash = flash;

    return ERR_0_OK;
}
ZxError screen_set_temp_bright(ZxScreen screen, const uint8_t bright) {
    if (screen == NULL) return ERR_UNKNOWN;

    if (bright != 0 && bright != 1 && bright != 8) {
        return ERR_UNKNOWN;
    }

    screen->temp_attrs.bright = bright;

    return ERR_0_OK;
}
ZxError screen_set_temp_ink(ZxScreen screen, const uint8_t ink) {
    if (screen == NULL) return ERR_UNKNOWN;

    if (ink > 8) {
        return ERR_UNKNOWN;
    }

    screen->temp_attrs.ink = ink;

    return ERR_0_OK;
}
ZxError screen_set_temp_paper(ZxScreen screen, const uint8_t paper) {
    if (screen == NULL) return ERR_UNKNOWN;

    if (paper > 8) {
        return ERR_UNKNOWN;
    }

    screen->temp_attrs.paper = paper;

    return ERR_0_OK;
}
ZxError screen_set_temp_inverse(ZxScreen screen, const uint8_t inverse) {
    if (screen == NULL) return ERR_UNKNOWN;
    if (inverse != 0 && inverse != 1) return ERR_UNKNOWN;

    screen->temp_attrs.inverse = inverse;
    return ERR_0_OK;
}
ZxError screen_set_temp_over(ZxScreen screen, const uint8_t over) {
    if (screen == NULL) return ERR_UNKNOWN;
    if (over != 0 && over != 1) return ERR_UNKNOWN;

    screen->temp_attrs.over = over;
    return ERR_0_OK;
}
void screen_reset_temp_attrs(ZxScreen screen) {
    if (screen == NULL) return;
    screen->temp_attrs = screen->perm_attrs;
}

//Write and read txt
bool screen_put_txt_char(ZxScreen screen, const uint8_t character) {
    if (screen == NULL) return false;

    uint8_t current_attr = screen->vram[screen->main_cursor_y][screen->main_cursor_x].attribute;
    uint8_t final_attr = combine_attributes(current_attr, &screen->temp_attrs);

    ZxCell cell = {character, final_attr};
    screen->vram[screen->main_cursor_y][screen->main_cursor_x] = cell;

    return screen_txt_advance_x(screen);
}
bool screen_txt_new_line(ZxScreen screen) {
    if (screen == NULL) return false;

    screen->main_cursor_x = 0;
    screen->main_cursor_y++;

    if (screen->main_cursor_y >= MAIN_SCREEN_ROWS) {
        screen->main_cursor_y = MAIN_SCREEN_ROWS - 1; // Blijf op regel 21

        memmove(&screen->vram[0][0], &screen->vram[1][0], (MAIN_SCREEN_ROWS - 1) * SCREEN_COLS * sizeof(ZxCell));

        uint8_t blank_attr = combine_attributes(0, &screen->perm_attrs);
        for (int x = 0; x < SCREEN_COLS; x++) {
            screen->vram[MAIN_SCREEN_ROWS - 1][x].character = ZX_CHAR_SPACE;
            screen->vram[MAIN_SCREEN_ROWS - 1][x].attribute = blank_attr;
        }

        return true;
    }

    return false;
}
bool screen_txt_advance_x(ZxScreen screen) {
    if (screen == NULL) return false;

    screen->main_cursor_x++;
    if (screen->main_cursor_x >= SCREEN_COLS) {
        return screen_txt_new_line(screen);
    }

    return false;
}
void screen_set_txt_cursor(ZxScreen screen, const uint8_t y, const uint8_t x) {
    if (screen == NULL) return;
    screen->main_cursor_y = y;
    screen->main_cursor_x = x;
}
uint8_t screen_get_txt_cursor_x(ZxScreen screen) {
    return screen->main_cursor_x;
}
uint8_t screen_get_txt_cursor_y(ZxScreen screen) {
    return screen->main_cursor_y;
}

//Write and read sys
void screen_clear_sys(ZxScreen screen) {
    if (screen == NULL) return;

    for (uint8_t y = MAIN_SCREEN_ROWS; y < MAIN_SCREEN_ROWS + 2; y++) {
        for (uint8_t x = 0; x < SCREEN_COLS; x++) {
            screen->vram[y][x].character = ' ';
            screen->vram[y][x].attribute = SYS_DEFAULT_ATTR;
        }
    }
    screen->sys_cursor_x = 0;
    screen->sys_cursor_y = 0;
}
void screen_put_sys_char(ZxScreen screen, const uint8_t character) {
    if (screen == NULL) return;

    uint8_t physical_y = MAIN_SCREEN_ROWS + screen->sys_cursor_y; // Altijd netjes regel 22 of 23
    ZxCell cell = {character, SYS_DEFAULT_ATTR};
    screen->vram[physical_y][screen->sys_cursor_x] = cell;

    // Optioneel: sys cursor opschuiven
    screen->sys_cursor_x++;
    if (screen->sys_cursor_x >= SCREEN_COLS) {
        screen->sys_cursor_x = 0;
        screen->sys_cursor_y = (screen->sys_cursor_y == 0) ? 1 : 0;
    }
}
void screen_set_sys_cursor(ZxScreen screen, const uint8_t y, const uint8_t x) {
    if (screen == NULL) return;
    screen->sys_cursor_y = (y > 1) ? 1 : y;
    screen->sys_cursor_x = (x >= SCREEN_COLS) ? SCREEN_COLS - 1 : x;
}
uint8_t screen_get_sys_cursor_x(ZxScreen screen) {
    return screen->sys_cursor_x;
}
uint8_t screen_get_sys_cursor_y(ZxScreen screen) {
    return screen->sys_cursor_y;
}

//ZxCell getters
const ZxCell* screen_get_cell(ZxScreen screen, const uint8_t y, const uint8_t x) {
    if (screen == NULL) return NULL;

    if (y >= TOTAL_SCREEN_ROWS || x >= SCREEN_COLS) {
        return NULL;
    }

    return &screen->vram[y][x];
}
const ZxCell* screen_get_buffer(ZxScreen screen) {
    if (screen == NULL) return NULL;

    return *screen->vram;
}