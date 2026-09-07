//
// Created by Marcel on 06-08-2026.
//

#include "screen.h"
#include "characters.h"

#include <string.h>

#define SCREEN_COLS 32
#define SYSTEM_SCREEN_ROWS 2
#define TOTAL_SCREEN_ROWS (MAIN_SCREEN_ROWS + SYSTEM_SCREEN_ROWS)
#define SYS_DEFAULT_ATTR 0x38

#define VRAM_CHARS_START   16384  // (Straks voor pixels/karakters)
#define VRAM_ATTRS_START   22528  // 768 attribuut-bytes
#define SYSVAR_ATTR_P      23693  // Permanente attributen
#define SYSVAR_ATTR_T      23695  // Tijdelijke attributen
#define SYSVAR_MASK_P      23694 // Transparantie
#define SYSVAR_MASK_T      23696 // Transparantie
#define SYSVAR_P_FLAG      23697 //Inverse en Over
#define SYSVAR_BORDCR      23624 //Border attributes
#define SYSVAR_S_POSN_COL  23688  // 33 - x (text)
#define SYSVAR_S_POSN_ROW  23689  // 24 - y (text)
#define SYSVAR_S_POSNL_COL  23682  // 33 - (x + 1) (sys)
#define SYSVAR_S_POSNL_ROW  23683  // 24 - y (sys)
#define SYSVAR_DF_SZ       23659 //hoogte sys-vak

#define ATTR_FLASH_MASK 0x80 // 1000 0000
#define ATTR_BRIGHT_MASK 0X40 // 0100 0000
#define ATTR_PAPER_MASK 0X38 // 0011 1000
#define ATTR_INK_MASK 0x07 // 0000 0111

#define ATTR_P_INV_MASK 0x40 // 0100 0000
#define ATTR_P_OVER_MASK 0x10 // 0001 0000
#define ATTR_T_INV_MASK 0x04 // 0000 0100
#define ATTR_T_OVER_MASK 0x01 // 0000 0001

static void apply_flash_bits(uint8_t *attr_byte, uint8_t *mask_byte, uint8_t flash) {
    if (flash == 8) {
        *mask_byte |= ATTR_FLASH_MASK;  // Transparant: masker AAN
        *attr_byte &= ~ATTR_FLASH_MASK; // Data UIT
    } else {
        *mask_byte &= ~ATTR_FLASH_MASK; // Masker UIT
        if (flash == 1) {
            *attr_byte |= ATTR_FLASH_MASK;
        } else {
            *attr_byte &= ~ATTR_FLASH_MASK;
        }
    }
}
static void apply_bright_bits(uint8_t *attr_byte, uint8_t *mask_byte, uint8_t bright) {
    if (bright == 8) {
        *mask_byte |= ATTR_BRIGHT_MASK;  // Transparant: masker AAN
        *attr_byte &= ~ATTR_BRIGHT_MASK; // Data UIT
    } else {
        *mask_byte &= ~ATTR_BRIGHT_MASK; // Masker UIT
        if (bright == 1) {
            *attr_byte |= ATTR_BRIGHT_MASK;
        } else {
            *attr_byte &= ~ATTR_BRIGHT_MASK;
        }
    }
}
static void apply_paper_bits(uint8_t *attr_byte, uint8_t *mask_byte, uint8_t paper) {
    if (paper == 8) {
        *mask_byte |= ATTR_PAPER_MASK;  // Maskeer alle 3 de paper-bits (bits 3..5)
        *attr_byte &= ~ATTR_PAPER_MASK; // Wis de paper-bits in de data
    } else {
        *mask_byte &= ~ATTR_PAPER_MASK; // Masker UIT voor paper
        // Wis de oude 3 paper-bits en schrijf de nieuwe waarde (0..7) 3 posities naar links
        *attr_byte = (*attr_byte & ~ATTR_PAPER_MASK) | ((paper & 0x07) << 3);
    }
}

static void apply_ink_bits(uint8_t *attr_byte, uint8_t *mask_byte, uint8_t ink) {
    if (ink == 8) {
        *mask_byte |= ATTR_INK_MASK;  // Maskeer alle 3 de ink-bits (bits 0..2)
        *attr_byte &= ~ATTR_INK_MASK; // Wis de ink-bits in de data
    } else {
        *mask_byte &= ~ATTR_INK_MASK; // Masker UIT voor ink
        // Wis de oude 3 ink-bits en schrijf de nieuwe waarde (0..7) op bits 0..2
        *attr_byte = (*attr_byte & ~ATTR_INK_MASK) | (ink & 0x07);
    }
}

// Life Cycle
void screen_init(ZxScreen screen) {
    if (screen == NULL) return;

    // Zet de Sinclair fabrieksinstellingen in het geheugen
    screen[SYSVAR_ATTR_P] = SYS_DEFAULT_ATTR;
    screen[SYSVAR_ATTR_T] = SYS_DEFAULT_ATTR;
    screen[SYSVAR_MASK_P] = 0x00;
    screen[SYSVAR_MASK_T] = 0x00;
    screen[SYSVAR_P_FLAG] = 0x00;
    screen[SYSVAR_BORDCR] = SYS_DEFAULT_ATTR;

    // Wis het scherm met deze standaarden
    screen_clear(screen);
    screen_clear_sys(screen);
}
void screen_clear(ZxScreen screen) {
    if (screen == NULL) return;

    screen_reset_temp_attrs(screen);

    uint8_t perm_attr = screen[SYSVAR_ATTR_P];

    for (int y = 0; y < MAIN_SCREEN_ROWS; y++) {
        for (int x = 0; x < SCREEN_COLS; x++) {
            int offset = (y * 32) + x;
            screen[VRAM_CHARS_START + offset] = ZX_CHAR_SPACE;
            screen[VRAM_ATTRS_START + offset] = perm_attr;
        }
    }

    // Zet cursor terug op (0, 0)
    screen_set_txt_cursor(screen, 0, 0);
}

//Attribuut beheer permanent
ZxError screen_set_flash(ZxScreen screen, const uint8_t flash, const bool is_permanent) {
    if (screen == NULL) return ERR_UNKNOWN;
    if (flash != 0 && flash != 1 && flash != 8) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }

    // Pas altijd toe op de actieve tijdelijke print-status
    apply_flash_bits(&screen[SYSVAR_ATTR_T], &screen[SYSVAR_MASK_T], flash);

    // Indien permanent: werk ook de permanente systeemvariabelen bij
    if (is_permanent) {
        apply_flash_bits(&screen[SYSVAR_ATTR_P], &screen[SYSVAR_MASK_P], flash);
    }

    return ERR_0_OK;
}
ZxError screen_set_bright(ZxScreen screen, const uint8_t bright, const bool is_permanent) {
    if (screen == NULL) return ERR_UNKNOWN;
    if (bright != 0 && bright != 1 && bright != 8) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }

    // Pas altijd toe op de actieve tijdelijke print-status
    apply_bright_bits(&screen[SYSVAR_ATTR_T], &screen[SYSVAR_MASK_T], bright);

    // Indien permanent: werk ook de permanente systeemvariabelen bij
    if (is_permanent) {
        apply_bright_bits(&screen[SYSVAR_ATTR_P], &screen[SYSVAR_MASK_P], bright);
    }

    return ERR_0_OK;
}
ZxError screen_set_ink(ZxScreen screen, const uint8_t ink, const bool is_permanent) {
    if (screen == NULL) return ERR_UNKNOWN;

    if (ink > 8) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }

    // Pas altijd toe op de actieve tijdelijke print-status
    apply_ink_bits(&screen[SYSVAR_ATTR_T], &screen[SYSVAR_MASK_T], ink);

    // Indien permanent: werk ook de permanente systeemvariabelen bij
    if (is_permanent) {
        apply_ink_bits(&screen[SYSVAR_ATTR_P], &screen[SYSVAR_MASK_P], ink);
    }

    return ERR_0_OK;
}
ZxError screen_set_paper(ZxScreen screen, const uint8_t paper, const bool is_permanent) {
    if (screen == NULL) return ERR_UNKNOWN;

    if (paper > 8) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }

    // Pas altijd toe op de actieve tijdelijke print-status
    apply_paper_bits(&screen[SYSVAR_ATTR_T], &screen[SYSVAR_MASK_T], paper);

    // Indien permanent: werk ook de permanente systeemvariabelen bij
    if (is_permanent) {
        apply_paper_bits(&screen[SYSVAR_ATTR_P], &screen[SYSVAR_MASK_P], paper);
    }

    return ERR_0_OK;
}
ZxError screen_set_inverse(ZxScreen screen, const uint8_t inverse, const bool is_permanent) {
    if (screen == NULL) return ERR_UNKNOWN;
    if (inverse != 0 && inverse != 1) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }

    // 1. Pas altijd toe op de tijdelijke vlag (bit 2)
    if (inverse == 1) {
        screen[SYSVAR_P_FLAG] |= ATTR_T_INV_MASK;
    } else {
        screen[SYSVAR_P_FLAG] &= ~ATTR_T_INV_MASK;
    }

    // 2. Indien permanent: pas ook toe op de permanente vlag (bit 6)
    if (is_permanent) {
        if (inverse == 1) {
            screen[SYSVAR_P_FLAG] |= ATTR_P_INV_MASK;
        } else {
            screen[SYSVAR_P_FLAG] &= ~ATTR_P_INV_MASK;
        }
    }

    return ERR_0_OK;
}
ZxError screen_set_over(ZxScreen screen, const uint8_t over, const bool is_permanent) {
    if (screen == NULL) return ERR_UNKNOWN;
    if (over != 0 && over != 1) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }

    // 1. Pas altijd toe op de tijdelijke vlag (bit 0)
    if (over == 1) {
        screen[SYSVAR_P_FLAG] |= ATTR_T_OVER_MASK;
    } else {
        screen[SYSVAR_P_FLAG] &= ~ATTR_T_OVER_MASK;
    }

    // 2. Indien permanent: pas ook toe op de permanente vlag (bit 4)
    if (is_permanent) {
        if (over == 1) {
            screen[SYSVAR_P_FLAG] |= ATTR_P_OVER_MASK;
        } else {
            screen[SYSVAR_P_FLAG] &= ~ATTR_P_OVER_MASK;
        }
    }

    return ERR_0_OK;
}

//Attribuut beheer temp
void screen_reset_temp_attrs(ZxScreen screen) {
    if (screen == NULL) return;

    screen[SYSVAR_ATTR_T] = screen[SYSVAR_ATTR_P];
    screen[SYSVAR_MASK_T] = screen[SYSVAR_MASK_P];

    // Kopieer permanente INVERSE & OVER (bits 6 en 4) terug naar tijdelijk (bits 2 en 0)
    uint8_t p_flags = screen[SYSVAR_P_FLAG];
    screen[SYSVAR_P_FLAG] = (p_flags & 0x50) | ((p_flags >> 4) & 0x05);
}

//Write and read txt
bool screen_put_txt_char(ZxScreen screen, const uint8_t character) {
    if (screen == NULL) return false;

    const uint8_t x = screen_get_txt_cursor_x(screen);
    const uint8_t y = screen_get_txt_cursor_y(screen);

    const int offset = (y * SCREEN_COLS) + x;

    // Schrijf karakter en tijdelijk attribuut direct in de 64K bak
    screen[VRAM_CHARS_START + offset] = character;

    // 1. Combineer met VRAM op basis van het transparantie-masker
    uint8_t cur_attr = screen[VRAM_ATTRS_START + offset];
    uint8_t mask = screen[SYSVAR_MASK_T];
    uint8_t attr = screen[SYSVAR_ATTR_T];
    uint8_t final_attr = (cur_attr & mask) | (attr & ~mask);

    // 2. Pas tijdelijke INVERSE toe (wissel de laagste 3 bits met bits 3..5)
    if (screen[SYSVAR_P_FLAG] & ATTR_T_INV_MASK) {
        uint8_t ink = final_attr & 0x07;
        uint8_t paper = (final_attr >> 3) & 0x07;
        final_attr = (final_attr & 0xC0) | (ink << 3) | paper;
    }

    screen[VRAM_ATTRS_START + offset] = final_attr;

    return screen_txt_advance_x(screen);
}
bool screen_txt_new_line(ZxScreen screen) {
    if (screen == NULL) return false;

    uint8_t y = screen_get_txt_cursor_y(screen);

    if (++y >= MAIN_SCREEN_ROWS) {
        y = MAIN_SCREEN_ROWS - 1;

        memmove(&screen[VRAM_CHARS_START], &screen[VRAM_CHARS_START + 32], (MAIN_SCREEN_ROWS - 1) * SCREEN_COLS);
        memmove(&screen[VRAM_ATTRS_START], &screen[VRAM_ATTRS_START + 32], (MAIN_SCREEN_ROWS - 1) * SCREEN_COLS);

        for (int x = 0; x < SCREEN_COLS; x++) {
            const int offset = (y * SCREEN_COLS) + x;
            screen[VRAM_CHARS_START + offset] = ZX_CHAR_SPACE;
            screen[VRAM_ATTRS_START + offset] = screen[SYSVAR_ATTR_P];
        }

        screen_set_txt_cursor(screen, y, 0);

        return true;
    }

    screen_set_txt_cursor(screen, y, 0);

    return false;
}
bool screen_txt_advance_x(ZxScreen screen) {
    if (screen == NULL) return false;

    uint8_t x = screen_get_txt_cursor_x(screen);
    uint8_t y = screen_get_txt_cursor_y(screen);

    if (++x >= SCREEN_COLS) {
        return screen_txt_new_line(screen);
    }

    screen_set_txt_cursor(screen, y, x);

    return false;
}
void screen_set_txt_cursor(ZxScreen screen, const uint8_t y, const uint8_t x) {
    if (!screen) return;
    screen[SYSVAR_S_POSN_ROW] = 24 - y;
    screen[SYSVAR_S_POSN_COL] = 33 - x;
}
uint8_t screen_get_txt_cursor_x(ZxScreen screen) {
    return 33 - screen[SYSVAR_S_POSN_COL];
}
uint8_t screen_get_txt_cursor_y(ZxScreen screen) {
    return 24 - screen[SYSVAR_S_POSN_ROW];
}

//Write and read sys
void screen_clear_sys(ZxScreen screen) {
    if (screen == NULL) return;

    for (uint8_t y = MAIN_SCREEN_ROWS; y < MAIN_SCREEN_ROWS + 2; y++) {
        for (uint8_t x = 0; x < SCREEN_COLS; x++) {
            int offset = y * SCREEN_COLS + x;
            screen[VRAM_CHARS_START + offset] = ZX_CHAR_SPACE;
            screen[VRAM_ATTRS_START + offset] = SYS_DEFAULT_ATTR;
        }
    }

    screen_set_sys_cursor(screen, 0, 0);
}
void screen_put_sys_char(ZxScreen screen, const uint8_t character) {
    if (screen == NULL) return;

    uint8_t x = screen_get_sys_cursor_x(screen);
    uint8_t y = screen_get_sys_cursor_y(screen);

    uint8_t physical_y = MAIN_SCREEN_ROWS + y; // Altijd netjes regel 22 of 23

    int offset = physical_y * SCREEN_COLS + x;

    screen[VRAM_CHARS_START + offset] = character;
    screen[VRAM_ATTRS_START + offset] = SYS_DEFAULT_ATTR;

    // Optioneel: sys cursor opschuiven
    x++;
    if (x >= SCREEN_COLS) {
        x = 0;
        y = (y == 0) ? 1 : 0;
    }
    screen_set_sys_cursor(screen, y, x);
}
void screen_set_sys_cursor(ZxScreen screen, const uint8_t rel_y, const uint8_t x) {
    if (!screen) return;
    uint8_t physical_y = 22 + (rel_y > 1 ? 1 : rel_y);
    screen[SYSVAR_S_POSNL_ROW] = 24 - physical_y;
    screen[SYSVAR_S_POSNL_COL] = 33 - x;
}
uint8_t screen_get_sys_cursor_x(ZxScreen screen) {
    return 33 - screen[SYSVAR_S_POSNL_COL];
}
uint8_t screen_get_sys_cursor_y(ZxScreen screen) {
    uint8_t physical_y = 24 - screen[SYSVAR_S_POSNL_ROW];
    return (physical_y >= 22) ? (physical_y - 22) : 0;
}

//character getters
uint8_t screen_get_char(ZxScreen screen, const int y, const int x) {
    if (screen == NULL || y < 0 || y >= TOTAL_SCREEN_ROWS || x < 0 || x >= SCREEN_COLS) {
        return ZX_CHAR_SPACE;
    }
    return screen[VRAM_CHARS_START + (y * SCREEN_COLS) + x];
}

uint8_t screen_get_attr(ZxScreen screen, const int y, const int x) {
    if (screen == NULL || y < 0 || y >= TOTAL_SCREEN_ROWS || x < 0 || x >= SCREEN_COLS) {
        return SYS_DEFAULT_ATTR;
    }
    return screen[VRAM_ATTRS_START + (y * SCREEN_COLS) + x];
}
const uint8_t* screen_get_chars_buffer(ZxScreen screen) {
    if (screen == NULL) return NULL;
    return &screen[VRAM_CHARS_START];
}

const uint8_t* screen_get_attrs_buffer(ZxScreen screen) {
    if (screen == NULL) return NULL;
    return &screen[VRAM_ATTRS_START];
}