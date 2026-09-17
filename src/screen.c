//
// Created by Marcel on 06-08-2026.
//

#include "screen.h"
#include "characters.h"

#include <string.h>

#define SCREEN_COLS 32
#define SYSTEM_SCREEN_ROWS 2
#define TOTAL_SCREEN_ROWS (MAIN_SCREEN_ROWS + SYSTEM_SCREEN_ROWS)
#define SCAN_LINES 8
#define SYS_DEFAULT_ATTR 0x38

#define VRAM_PIXELS_START  16384  // (Straks voor pixels)
#define VRAM_ATTRS_START   22528  // 768 attribuut-bytes
#define VRAM_ATTRS_END     23295  // 768 attribuut-bytes
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
#define SYSVAR_COORDS_X    23677 //coordinaten laatste plot
#define SYSVAR_COORDS_Y    23678 //coordinaten laatste plot

#define ATTR_FLASH_MASK 0x80 // 1000 0000
#define ATTR_BRIGHT_MASK 0X40 // 0100 0000
#define ATTR_PAPER_MASK 0X38 // 0011 1000
#define ATTR_INK_MASK 0x07 // 0000 0111

#define ATTR_P_INV_MASK 0x40 // 0100 0000
#define ATTR_P_OVER_MASK 0x10 // 0001 0000
#define ATTR_T_INV_MASK 0x04 // 0000 0100
#define ATTR_T_OVER_MASK 0x01 // 0000 0001

#define SCREEN_WIDTH  (SCREEN_COLS * 8)
#define SCREEN_HEIGHT (TOTAL_SCREEN_ROWS * 8)
#define MAIN_SCREEN_HEIGHT (MAIN_SCREEN_ROWS * 8)

static const uint32_t ZX_PALETTE[16] = {
    // Normaal (BRIGHT 0) - formaat 0xAABBGGRR voor Little-Endian Canvas
    0xFF000000, // 0: Black
    0xFFD70000, // 1: Blue
    0xFF0000D7, // 2: Red
    0xFFD700D7, // 3: Magenta
    0xFF00D700, // 4: Green
    0xFFD7D700, // 5: Cyan
    0xFF00D7D7, // 6: Yellow
    0xFFD7D7D7, // 7: White
    // Helder (BRIGHT 1)
    0xFF000000, // 8: Bright Black
    0xFFFF0000, // 9: Bright Blue
    0xFF0000FF, // 10: Bright Red
    0xFFFF00FF, // 11: Bright Magenta
    0xFF00FF00, // 12: Bright Green
    0xFFFFFF00, // 13: Bright Cyan
    0xFF00FFFF, // 14: Bright Yellow
    0xFFFFFFFF  // 15: Bright White
};
static uint32_t rgba_framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
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
static uint16_t screen_get_pixel_address(uint8_t col, uint8_t row, uint8_t scanline) {
    if (col >= SCREEN_COLS || row >= TOTAL_SCREEN_ROWS || scanline >= SCAN_LINES) return 0;

    uint16_t t = row / 8;     // 0, 1 of 2 (boven, midden, onder)
    uint16_t r = row % 8;     // 0 t/m 7 (rij binnen het derde deel)
    uint16_t s = scanline;    // 0 t/m 7 (scanline)
    uint16_t c = col;         // 0 t/m 31 (kolom)

    return VRAM_PIXELS_START | (t << 11) | (s << 8) | (r << 5) | c;
}
static uint16_t screen_get_pixel_address_plot(const uint8_t x, const uint8_t y) {
    if (y > MAIN_SCREEN_HEIGHT) return 0;
    uint8_t col = x / SCAN_LINES;
    uint8_t screen_y = (MAIN_SCREEN_HEIGHT - 1) - y;
    uint8_t row = screen_y / SCAN_LINES;
    uint8_t scanline = screen_y % SCAN_LINES;
    return screen_get_pixel_address(col, row, scanline);
}
static int screen_get_offset_pixel(const uint8_t x, const uint8_t y) {
    if (y > MAIN_SCREEN_HEIGHT) return ERR_B_INTEGER_OUT_OF_RANGE;

    uint8_t col = x / SCAN_LINES;
    uint8_t screen_y = (MAIN_SCREEN_HEIGHT - 1) - y;
    uint8_t row = screen_y / SCAN_LINES;

    return (row * SCREEN_COLS) + col;
}
static ZxError screen_get_attributes_pixel(ZxMachine machine,
    const uint8_t x,
    const uint8_t y,
    uint8_t *attr_byte) {

    const int offset = screen_get_offset_pixel(x, y);

    return machine_peek(machine, VRAM_ATTRS_START + (uint16_t)offset, attr_byte);
}
static ZxError screen_sys_scroll_up(ZxMachine machine) {
    ZxError err;
    const uint8_t src_row = MAIN_SCREEN_ROWS + 1; // Regel 23
    const uint8_t dst_row = MAIN_SCREEN_ROWS;     // Regel 22

    // 1. Pixels van regel 23 naar regel 22 verplaatsen
    for (uint8_t line = 0; line < SCAN_LINES; line++) {
        for (uint8_t col = 0; col < SCREEN_COLS; col++) {
            uint16_t src_addr = screen_get_pixel_address(col, src_row, line);
            uint16_t dst_addr = screen_get_pixel_address(col, dst_row, line);
            uint8_t byte = 0;

            err = machine_peek(machine, src_addr, &byte);
            if (err != ERR_0_OK) return err;

            err = machine_poke(machine, dst_addr, byte);
            if (err != ERR_0_OK) return err;

            // Wis pixel op regel 23
            err = machine_poke(machine, src_addr, 0x00);
            if (err != ERR_0_OK) return err;
        }
    }

    // 2. Attributen van regel 23 naar regel 22 verplaatsen
    for (uint8_t col = 0; col < SCREEN_COLS; col++) {
        uint16_t src_attr_addr = VRAM_ATTRS_START + (src_row * SCREEN_COLS) + col;
        uint16_t dst_attr_addr = VRAM_ATTRS_START + (dst_row * SCREEN_COLS) + col;
        uint8_t attr = SYS_DEFAULT_ATTR;

        err = machine_peek(machine, src_attr_addr, &attr);
        if (err != ERR_0_OK) return err;

        err = machine_poke(machine, dst_attr_addr, attr);
        if (err != ERR_0_OK) return err;

        // Reset attribuut op regel 23
        err = machine_poke(machine, src_attr_addr, SYS_DEFAULT_ATTR);
        if (err != ERR_0_OK) return err;
    }

    return ERR_0_OK;
}

// Life Cycle
ZxError screen_init(ZxMachine machine) {
    if (machine == NULL) return ERR_UNKNOWN;

    ZxError err;
    // Zet de Sinclair fabrieksinstellingen in het geheugen
    err = machine_poke(machine, SYSVAR_ATTR_P, SYS_DEFAULT_ATTR);
    if (err != ERR_0_OK) return err;
    err = machine_poke(machine, SYSVAR_ATTR_T, SYS_DEFAULT_ATTR);
    if (err != ERR_0_OK) return err;
    err = machine_poke(machine, SYSVAR_MASK_P, 0x00);
    if (err != ERR_0_OK) return err;
    err = machine_poke(machine, SYSVAR_MASK_T, 0x00);
    if (err != ERR_0_OK) return err;
    err = machine_poke(machine, SYSVAR_P_FLAG, 0x00);
    if (err != ERR_0_OK) return err;
    err = machine_poke(machine, SYSVAR_BORDCR, SYS_DEFAULT_ATTR);
    if (err != ERR_0_OK) return err;

    // Wis het scherm met deze standaarden
    err = screen_clear(machine);
    if (err != ERR_0_OK) return err;
    return screen_clear_sys(machine);
}
ZxError screen_clear(ZxMachine machine) {
    if (machine == NULL) return ERR_UNKNOWN;

    ZxError err;
    err = screen_reset_temp_attrs(machine);
    if (err != ERR_0_OK) return err;

    uint8_t perm_attr;
    err = machine_peek(machine, SYSVAR_ATTR_P, &perm_attr);
    if (err != ERR_0_OK) return err;

    for (int i = VRAM_PIXELS_START; i < VRAM_ATTRS_START; i++) {
        err = machine_poke(machine, i, 0x00);
        if (err != ERR_0_OK) return err;
    }

    for (int i = VRAM_ATTRS_START; i <= VRAM_ATTRS_END; i++) {
        err = machine_poke(machine, i, perm_attr);
        if (err != ERR_0_OK) return err;
    }

    // Zet cursor terug op (0, 0)
    return screen_set_txt_cursor(machine, 0, 0);
}

//Attribuut beheer
ZxError screen_set_flash(ZxMachine machine, const uint8_t flash, const bool is_permanent) {
    if (machine == NULL) return ERR_UNKNOWN;
    if (flash != 0 && flash != 1 && flash != 8) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }
    ZxError err;
    uint8_t attr_byte;
    uint8_t mask_byte;

    err = machine_peek(machine, SYSVAR_ATTR_T, &attr_byte);
    if (err != ERR_0_OK) return err;
    err = machine_peek(machine, SYSVAR_MASK_T, &mask_byte);
    if (err != ERR_0_OK) return err;

    apply_flash_bits(&attr_byte, &mask_byte, flash);
    err = machine_poke(machine, SYSVAR_ATTR_T, attr_byte);
    if (err != ERR_0_OK) return err;
    err = machine_poke(machine, SYSVAR_MASK_T, mask_byte);
    if (err != ERR_0_OK) return err;



    // Indien permanent: werk ook de permanente systeemvariabelen bij
    if (is_permanent) {
        err = machine_peek(machine, SYSVAR_ATTR_P, &attr_byte);
        if (err != ERR_0_OK) return err;
        err = machine_peek(machine, SYSVAR_MASK_P, &mask_byte);
        if (err != ERR_0_OK) return err;

        apply_flash_bits(&attr_byte, &mask_byte, flash);
        err = machine_poke(machine, SYSVAR_ATTR_P, attr_byte);
        if (err != ERR_0_OK) return err;
        err = machine_poke(machine, SYSVAR_MASK_P, mask_byte);
        if (err != ERR_0_OK) return err;
    }

    return ERR_0_OK;
}
ZxError screen_set_bright(ZxMachine machine, const uint8_t bright, const bool is_permanent) {
    if (machine == NULL) return ERR_UNKNOWN;
    if (bright != 0 && bright != 1 && bright != 8) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }
    ZxError err;
    uint8_t attr_byte;
    uint8_t mask_byte;

    err = machine_peek(machine, SYSVAR_ATTR_T, &attr_byte);
    if (err != ERR_0_OK) return err;
    err = machine_peek(machine, SYSVAR_MASK_T, &mask_byte);
    if (err != ERR_0_OK) return err;

    apply_bright_bits(&attr_byte, &mask_byte, bright);
    err = machine_poke(machine, SYSVAR_ATTR_T, attr_byte);
    if (err != ERR_0_OK) return err;
    err = machine_poke(machine, SYSVAR_MASK_T, mask_byte);
    if (err != ERR_0_OK) return err;

    // Indien permanent: werk ook de permanente systeemvariabelen bij
    if (is_permanent) {
        err = machine_peek(machine, SYSVAR_ATTR_P, &attr_byte);
        if (err != ERR_0_OK) return err;
        err = machine_peek(machine, SYSVAR_MASK_P, &mask_byte);
        if (err != ERR_0_OK) return err;

        apply_bright_bits(&attr_byte, &mask_byte, bright);
        err = machine_poke(machine, SYSVAR_ATTR_P, attr_byte);
        if (err != ERR_0_OK) return err;
        err = machine_poke(machine, SYSVAR_MASK_P, mask_byte);
        if (err != ERR_0_OK) return err;
    }

    return ERR_0_OK;
}
ZxError screen_set_ink(ZxMachine machine, const uint8_t ink, const bool is_permanent) {
    if (machine == NULL) return ERR_UNKNOWN;
    if (ink > 8) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }
    ZxError err;
    uint8_t attr_byte;
    uint8_t mask_byte;

    err = machine_peek(machine, SYSVAR_ATTR_T, &attr_byte);
    if (err != ERR_0_OK) return err;
    err = machine_peek(machine, SYSVAR_MASK_T, &mask_byte);
    if (err != ERR_0_OK) return err;

    apply_ink_bits(&attr_byte, &mask_byte, ink);
    err = machine_poke(machine, SYSVAR_ATTR_T, attr_byte);
    if (err != ERR_0_OK) return err;
    err = machine_poke(machine, SYSVAR_MASK_T, mask_byte);
    if (err != ERR_0_OK) return err;

    // Indien permanent: werk ook de permanente systeemvariabelen bij
    if (is_permanent) {
        err = machine_peek(machine, SYSVAR_ATTR_P, &attr_byte);
        if (err != ERR_0_OK) return err;
        err = machine_peek(machine, SYSVAR_MASK_P, &mask_byte);
        if (err != ERR_0_OK) return err;

        apply_ink_bits(&attr_byte, &mask_byte, ink);
        err = machine_poke(machine, SYSVAR_ATTR_P, attr_byte);
        if (err != ERR_0_OK) return err;
        err = machine_poke(machine, SYSVAR_MASK_P, mask_byte);
        if (err != ERR_0_OK) return err;
    }

    return ERR_0_OK;
}
ZxError screen_set_paper(ZxMachine machine, const uint8_t paper, const bool is_permanent) {
    if (machine == NULL) return ERR_UNKNOWN;
    if (paper > 8) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }
    ZxError err;
    uint8_t attr_byte;
    uint8_t mask_byte;

    err = machine_peek(machine, SYSVAR_ATTR_T, &attr_byte);
    if (err != ERR_0_OK) return err;
    err = machine_peek(machine, SYSVAR_MASK_T, &mask_byte);
    if (err != ERR_0_OK) return err;

    apply_paper_bits(&attr_byte, &mask_byte, paper);
    err = machine_poke(machine, SYSVAR_ATTR_T, attr_byte);
    if (err != ERR_0_OK) return err;
    err = machine_poke(machine, SYSVAR_MASK_T, mask_byte);
    if (err != ERR_0_OK) return err;

    // Indien permanent: werk ook de permanente systeemvariabelen bij
    if (is_permanent) {
        err = machine_peek(machine, SYSVAR_ATTR_P, &attr_byte);
        if (err != ERR_0_OK) return err;
        err = machine_peek(machine, SYSVAR_MASK_P, &mask_byte);
        if (err != ERR_0_OK) return err;

        apply_paper_bits(&attr_byte, &mask_byte, paper);
        err = machine_poke(machine, SYSVAR_ATTR_P, attr_byte);
        if (err != ERR_0_OK) return err;
        err = machine_poke(machine, SYSVAR_MASK_P, mask_byte);
        if (err != ERR_0_OK) return err;
    }

    return ERR_0_OK;
}
ZxError screen_set_inverse(ZxMachine machine, const uint8_t inverse, const bool is_permanent) {
    if (machine == NULL) return ERR_UNKNOWN;
    if (inverse != 0 && inverse != 1) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }

    ZxError err;
    uint8_t p_flag;
    err = machine_peek(machine, SYSVAR_P_FLAG, &p_flag);
    if (err != ERR_0_OK) return err;

    // 1. Pas altijd toe op de tijdelijke vlag (bit 2)
    if (inverse == 1) {
        p_flag |= ATTR_T_INV_MASK;
    } else {
        p_flag &= ~ATTR_T_INV_MASK;
    }

    // 2. Indien permanent: pas ook toe op de permanente vlag (bit 6)
    if (is_permanent) {
        if (inverse == 1) {
            p_flag |= ATTR_P_INV_MASK;
        } else {
            p_flag &= ~ATTR_P_INV_MASK;
        }
    }
    return machine_poke(machine, SYSVAR_P_FLAG, p_flag);
}
ZxError screen_set_over(ZxMachine machine, const uint8_t over, const bool is_permanent) {
    if (machine == NULL) return ERR_UNKNOWN;
    if (over != 0 && over != 1) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }

    ZxError err;
    uint8_t p_flag;
    err = machine_peek(machine, SYSVAR_P_FLAG, &p_flag);
    if (err != ERR_0_OK) return err;

    // 1. Pas altijd toe op de tijdelijke vlag (bit 2)
    if (over == 1) {
        p_flag |= ATTR_T_OVER_MASK;
    } else {
        p_flag &= ~ATTR_T_OVER_MASK;
    }

    // 2. Indien permanent: pas ook toe op de permanente vlag (bit 6)
    if (is_permanent) {
        if (over == 1) {
            p_flag |= ATTR_P_OVER_MASK;
        } else {
            p_flag &= ~ATTR_P_OVER_MASK;
        }
    }
    return machine_poke(machine, SYSVAR_P_FLAG, p_flag);
}

//Attribuut beheer temp
ZxError screen_reset_temp_attrs(ZxMachine machine) {
    if (machine == NULL) return ERR_UNKNOWN;

    ZxError err;

    uint8_t attrs;
    err = machine_peek(machine, SYSVAR_ATTR_P, &attrs);
    if (err != ERR_0_OK) return err;
    uint8_t mask;
    err = machine_peek(machine, SYSVAR_MASK_P, &mask);
    if (err != ERR_0_OK) return err;

    err = machine_poke(machine, SYSVAR_ATTR_T, attrs);
    if (err != ERR_0_OK) return err;
    err = machine_poke(machine, SYSVAR_MASK_T, mask);
    if (err != ERR_0_OK) return err;

    // Kopieer permanente INVERSE & OVER (bits 6 en 4) terug naar tijdelijk (bits 2 en 0)
    uint8_t p_flags;
    err = machine_peek(machine, SYSVAR_P_FLAG, &p_flags);
    if (err != ERR_0_OK) return err;
    return machine_poke(machine, SYSVAR_P_FLAG, (p_flags & 0x50) | ((p_flags >> 4) & 0x05));
}

//Write and read txt
ZxError screen_put_txt_char(ZxMachine machine, const uint8_t character, bool *scroll) {
    if (machine == NULL) return ERR_UNKNOWN;

    ZxError err;

    uint8_t x;
    err = screen_get_txt_cursor_x(machine, &x);
    if (err != ERR_0_OK) return err;

    uint8_t y;
    err = screen_get_txt_cursor_y(machine, &y);
    if (err != ERR_0_OK) return err;

    // 1. Vraag de 8 scanline-bytes op aan de Font Engine
    uint8_t bitmap[8];
    err = zx_get_character_bitmap(machine, character, bitmap);
    if (err != ERR_0_OK) return err;

    // 2. Pas tijdelijke INVERSE toe (wissel de laagste 3 bits met bits 3..5)
    uint8_t p_flag;
    err = machine_peek(machine, SYSVAR_P_FLAG, &p_flag);
    if (err != ERR_0_OK) return err;
    if (p_flag & ATTR_T_INV_MASK) {
        for (int i = 0; i < SCAN_LINES; i++) {
            bitmap[i] = ~bitmap[i]; // Bitwise NOT: alle 0-pixels worden 1, en 1 wordt 0
        }
    }

    for (uint8_t scanline = 0; scanline < SCAN_LINES; scanline++) {
        uint16_t pixel_addr = screen_get_pixel_address(x, y, scanline);
        uint8_t byte_to_write = bitmap[scanline];

        if (p_flag & ATTR_T_OVER_MASK) {
            uint8_t cur_pixels = 0;
            err = machine_peek(machine, pixel_addr, &cur_pixels);
            if (err != ERR_0_OK) return err;
            byte_to_write ^= cur_pixels;
        }

        err = machine_poke(machine, pixel_addr, byte_to_write);
        if (err != ERR_0_OK) return err;
    }

    const int offset = (y * SCREEN_COLS) + x;

    // 3. Schrijf de kleurattribuut-byte alleen weg als OVER 0 is
    if (!(p_flag & ATTR_T_OVER_MASK)) {
        uint16_t attr_addr = VRAM_ATTRS_START + (uint16_t)offset;
        uint8_t cur_attr = 0;
        err = machine_peek(machine, attr_addr, &cur_attr);
        if (err != ERR_0_OK) return err;

        uint8_t mask = 0;
        err = machine_peek(machine, SYSVAR_MASK_T, &mask);
        if (err != ERR_0_OK) return err;

        uint8_t attr = 0;
        err = machine_peek(machine, SYSVAR_ATTR_T, &attr);
        if (err != ERR_0_OK) return err;

        uint8_t final_attr = (cur_attr & mask) | (attr & ~mask);
        err = machine_poke(machine, attr_addr, final_attr);
        if (err != ERR_0_OK) return err;
    }

    return screen_txt_advance_x(machine, scroll);
}
ZxError screen_txt_new_line(ZxMachine machine, bool *scroll) {
    if (machine == NULL) return ERR_UNKNOWN;

    ZxError err;
    *scroll = false;

    uint8_t cursor_y;
    err = screen_get_txt_cursor_y(machine, &cursor_y);
    if (err != ERR_0_OK) return err;

    if (++cursor_y >= MAIN_SCREEN_ROWS) {
        *scroll = true;
        // 1. Pixels 8 scanlines omhoog schuiven (interleaved)
        for (uint8_t y = 0; y < MAIN_SCREEN_ROWS - 1; y++) {
            for (uint8_t line = 0; line < SCAN_LINES; line++) {
                for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                    uint16_t src = screen_get_pixel_address(x, y + 1, line);
                    uint16_t dst = screen_get_pixel_address(x, y, line);
                    uint8_t byte;

                    err = machine_peek(machine, src, &byte);
                    if (err != ERR_0_OK) return err;

                    err = machine_poke(machine, dst, byte);
                    if (err != ERR_0_OK) return err;
                }
            }
        }

        // 2. Attributen 1 regel omhoog schuiven (volledig lineair!)
        uint16_t attr_bytes_to_move = (MAIN_SCREEN_ROWS - 1) * SCREEN_COLS;
        for (uint16_t i = 0; i < attr_bytes_to_move; i++) {
            uint8_t attr;
            err = machine_peek(machine, VRAM_ATTRS_START + SCREEN_COLS + i, &attr);
            if (err != ERR_0_OK) return err;

            err = machine_poke(machine, VRAM_ATTRS_START + i, attr);
            if (err != ERR_0_OK) return err;
        }

        // 3. Onderste regel wissen: pixels op 0x00
        uint8_t bottom_row = MAIN_SCREEN_ROWS - 1;
        for (uint8_t line = 0; line < SCAN_LINES; line++) {
            for (uint8_t x = 0; x < SCREEN_COLS; x++) {
                uint16_t addr = screen_get_pixel_address(x, bottom_row, line);
                err = machine_poke(machine, addr, 0x00);
                if (err != ERR_0_OK) return err;
            }
        }
        // 4. Onderste regel wissen: attributen vullen met ATTR_P
        uint8_t default_attr;
        err = machine_peek(machine, SYSVAR_ATTR_P, &default_attr);
        if (err != ERR_0_OK) return err;

        uint16_t bottom_attr_start = VRAM_ATTRS_START + (bottom_row * SCREEN_COLS);
        for (uint8_t x = 0; x < SCREEN_COLS; x++) {
            err = machine_poke(machine, bottom_attr_start + x, default_attr);
            if (err != ERR_0_OK) return err;
        }

        cursor_y = MAIN_SCREEN_ROWS - 1;
    }
    return screen_set_txt_cursor(machine, cursor_y, 0);

}
ZxError screen_txt_advance_x(ZxMachine machine, bool *scroll) {
    if (machine == NULL) return ERR_UNKNOWN;

    *scroll = false;

    uint8_t x;
    ZxError err = screen_get_txt_cursor_x(machine, &x);
    if (err != ERR_0_OK) return err;

    uint8_t y;
    err = screen_get_txt_cursor_y(machine, &y);
    if (err != ERR_0_OK) return err;

    if (++x >= SCREEN_COLS) {
        return screen_txt_new_line(machine, scroll);
    }

    return screen_set_txt_cursor(machine, y, x);
}
ZxError screen_set_txt_cursor(ZxMachine machine, const uint8_t y, const uint8_t x) {
    if (!machine) return ERR_UNKNOWN;

    ZxError err;

    err = machine_poke(machine, SYSVAR_S_POSN_ROW, 24 - y);
    if (err != ERR_0_OK) return err;

    return machine_poke(machine, SYSVAR_S_POSN_COL, 33 - x);
}
ZxError screen_get_txt_cursor_x(ZxMachine machine, uint8_t *x) {
    if (machine == NULL || x == NULL) return ERR_UNKNOWN;

    ZxError err = machine_peek(machine, SYSVAR_S_POSN_COL, x);
    if (err != ERR_0_OK) return err;

    *x = 33 - *x;
    return ERR_0_OK;
}
ZxError screen_get_txt_cursor_y(ZxMachine machine, uint8_t *y) {
    if (machine == NULL || y == NULL) return ERR_UNKNOWN;

    ZxError err = machine_peek(machine, SYSVAR_S_POSN_ROW, y);
    if (err != ERR_0_OK) return err;

    *y = 24 - *y;
    return ERR_0_OK;
}

//Write and read sys
ZxError screen_clear_sys(ZxMachine machine) {
    if (machine == NULL) return ERR_UNKNOWN;

    ZxError err;
    for (uint8_t y = MAIN_SCREEN_ROWS; y < MAIN_SCREEN_ROWS + 2; y++) {
        for (uint8_t x = 0; x < SCREEN_COLS; x++) {
            for (uint8_t l = 0; l < SCAN_LINES; l++) {
                uint16_t address = screen_get_pixel_address(x, y, l);
                err = machine_poke(machine, address, 0x00);
                if (err != ERR_0_OK) return err;
            }

            int offset = y * SCREEN_COLS + x;
            err = machine_poke(machine, VRAM_ATTRS_START + offset, SYS_DEFAULT_ATTR);
            if (err != ERR_0_OK) return err;
        }
    }

    return screen_set_sys_cursor(machine, 0, 0);
}
ZxError screen_put_sys_char_attr(ZxMachine machine, const uint8_t character, uint8_t attr) {
    if (machine == NULL) return ERR_UNKNOWN;

    ZxError err;
    uint8_t x, y;
    err = screen_get_sys_cursor_x(machine, &x);
    if (err != ERR_0_OK) return err;
    err = screen_get_sys_cursor_y(machine, &y);
    if (err != ERR_0_OK) return err;

    uint8_t bitmap[8];
    err = zx_get_character_bitmap(machine, character, bitmap);
    if (err != ERR_0_OK) return err;

    uint8_t physical_y = MAIN_SCREEN_ROWS + (y > 1 ? 1 : y); // Regel 22 of 23

    // 1. Schrijf pixels naar VRAM
    for (uint8_t scanline = 0; scanline < SCAN_LINES; scanline++) {
        uint16_t pixel_addr = screen_get_pixel_address(x, physical_y, scanline);
        err = machine_poke(machine, pixel_addr, bitmap[scanline]);
        if (err != ERR_0_OK) return err;
    }

    // 2. Schrijf attribuut met physical_y (voorkomt overschrijven van regel 0/1!)
    const int offset = (physical_y * SCREEN_COLS) + x;
    uint16_t attr_addr = VRAM_ATTRS_START + (uint16_t)offset;
    err = machine_poke(machine, attr_addr, attr);
    if (err != ERR_0_OK) return err;

    // 3. Cursor opschuiven
    x++;
    if (x >= SCREEN_COLS) {
        x = 0;
        if (y == 0) {
            y = 1; // Ga van regel 22 door naar regel 23
        } else {
            // We staan al op regel 23: schuif regel 23 omhoog naar 22!
            err = screen_sys_scroll_up(machine);
            if (err != ERR_0_OK) return err;
            y = 1;
        }
    }
    return screen_set_sys_cursor(machine, y, x);
}
ZxError screen_put_sys_char(ZxMachine machine, const uint8_t character) {
    return screen_put_sys_char_attr(machine, character, SYS_DEFAULT_ATTR);
}
ZxError screen_set_sys_cursor(ZxMachine machine, const uint8_t rel_y, const uint8_t x) {
    if (!machine) return ERR_UNKNOWN;

    ZxError err;

    uint8_t physical_y = 22 + (rel_y > 1 ? 1 : rel_y);

    err = machine_poke(machine, SYSVAR_S_POSNL_ROW, 24 - physical_y);
    if (err != ERR_0_OK) return err;

    return machine_poke(machine, SYSVAR_S_POSNL_COL, 33 - x);
}
ZxError screen_get_sys_cursor_x(ZxMachine machine, uint8_t *x) {
    if (machine == NULL || x == NULL) return ERR_UNKNOWN;

    ZxError err = machine_peek(machine, SYSVAR_S_POSNL_COL, x);
    if (err != ERR_0_OK) return err;

    *x = 33 - *x;
    return ERR_0_OK;
}
ZxError screen_get_sys_cursor_y(ZxMachine machine, uint8_t *y) {
    if (machine == NULL || y == NULL) return ERR_UNKNOWN;

    uint8_t y_raw;
    ZxError err = machine_peek(machine, SYSVAR_S_POSNL_ROW, &y_raw);
    if (err != ERR_0_OK) return err;

    uint8_t physical_y = 24 - y_raw;
    *y = (physical_y >= 22) ? (physical_y - 22) : 0;
    return ERR_0_OK;
}

//character getters
ZxError screen_get_char(ZxMachine machine, const int y, const int x, uint8_t *character) {
    if (machine == NULL || character == NULL) return ERR_UNKNOWN;

    if (y < 0 || y >= TOTAL_SCREEN_ROWS || x < 0 || x >= SCREEN_COLS) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }

    //retrieve bitmap
    uint8_t bitmap[8];

    for (int i = 0; i < SCAN_LINES; i++) {
        const uint16_t address = screen_get_pixel_address(x, y, i);
        const ZxError err = machine_peek(machine, address, &bitmap[i]);
        if (err != ERR_0_OK) return err;
    }

    return zx_recognize_character(machine, bitmap, character);
}

ZxError screen_get_attr(ZxMachine machine, const int y, const int x, uint8_t *attributes) {
    if (machine == NULL || attributes == NULL) return ERR_UNKNOWN;

    if (y < 0 || y >= MAIN_SCREEN_ROWS || x < 0 || x >= SCREEN_COLS) {
        return ERR_B_INTEGER_OUT_OF_RANGE;
    }

    const uint16_t offset = (y * SCREEN_COLS) + x;
    const uint16_t address = VRAM_ATTRS_START + offset;
    return machine_peek(machine, address, attributes);
}
uint32_t* screen_get_framebuffer(void) {
    return rgba_framebuffer;
}
void screen_render_frame(ZxMachine machine, const bool flash_state) {
    for (uint8_t row = 0; row < TOTAL_SCREEN_ROWS; row++) {
        for (uint8_t col = 0; col < SCREEN_COLS; col++) {
            // 1. Attribuut ophalen
            const uint16_t attr_addr = VRAM_ATTRS_START + (row * SCREEN_COLS) + col;
            uint8_t attr = 0;
            machine_peek(machine, attr_addr, &attr);

            uint8_t ink   = attr & 0x07;
            uint8_t paper = (attr >> 3) & 0x07;
            const uint8_t bright = (attr & 0x40) ? 8 : 0;
            const bool flash    = (attr & 0x80) != 0;

            if (flash && flash_state) {
                // Wissel ink en paper om tijdens de actieve flash-fase
                const uint8_t tmp = ink;
                ink = paper;
                paper = tmp;
            }

            const uint32_t color_ink   = ZX_PALETTE[ink + bright];
            const uint32_t color_paper = ZX_PALETTE[paper + bright];

            // 2. Scanlines 0..7 van dit karakter renderen
            for (uint8_t line = 0; line < SCAN_LINES; line++) {
                uint16_t pixel_addr = screen_get_pixel_address(col, row, line);
                uint8_t pixels = 0;
                machine_peek(machine, pixel_addr, &pixels);

                // Lineaire index in de framebuffer:
                const uint32_t fb_y = (row * 8) + line;
                uint32_t fb_idx = (fb_y * SCREEN_WIDTH) + (col * 8);

                // 8 pixels wegschrijven (MSB links, LSB rechts)
                for (int b = 7; b >= 0; b--) {
                    rgba_framebuffer[fb_idx++] = (pixels & (1 << b)) ? color_ink : color_paper;
                }
            }
        }
    }
}
ZxError screen_plot(ZxMachine machine, const uint8_t x, const uint8_t y) {
    if (machine == NULL) return ERR_UNKNOWN;

    uint16_t address = screen_get_pixel_address_plot(x, y);
    if (address == 0) return ERR_B_INTEGER_OUT_OF_RANGE;

    uint8_t existing_scan_line;
    ZxError err = machine_peek(machine, address, &existing_scan_line);
    if (err != ERR_0_OK) return err;

    uint8_t bit_mask = 0x80 >> (x & 7);

    uint8_t p_flag;
    err = machine_peek(machine, SYSVAR_P_FLAG, &p_flag);
    if (err != ERR_0_OK) return err;

    bool over = (p_flag & ATTR_T_OVER_MASK) != 0;
    bool inverse = (p_flag & ATTR_T_INV_MASK) != 0;

    uint8_t target_scan_line;
    if (!over && !inverse) {
        target_scan_line = existing_scan_line | bit_mask;
    } else if (over && !inverse) {
        target_scan_line = existing_scan_line ^ bit_mask;
    } else if (!over && inverse){
        target_scan_line = existing_scan_line & ~bit_mask;
    }
    else {
        target_scan_line = existing_scan_line;
    }

    if (!over) {
        int offset = screen_get_offset_pixel(x, y);
        uint16_t attr_addr = VRAM_ATTRS_START + (uint16_t)offset;
        uint8_t cur_attr = 0;
        err = machine_peek(machine, attr_addr, &cur_attr);
        if (err != ERR_0_OK) return err;

        uint8_t mask = 0;
        err = machine_peek(machine, SYSVAR_MASK_T, &mask);
        if (err != ERR_0_OK) return err;

        uint8_t attr = 0;
        err = machine_peek(machine, SYSVAR_ATTR_T, &attr);
        if (err != ERR_0_OK) return err;

        uint8_t final_attr = (cur_attr & mask) | (attr & ~mask);
        err = machine_poke(machine, attr_addr, final_attr);
        if (err != ERR_0_OK) return err;
    }

    err = machine_poke(machine, SYSVAR_COORDS_X, x);
    if (err != ERR_0_OK) return err;

    err =  machine_poke(machine, SYSVAR_COORDS_Y, y);
    if (err != ERR_0_OK) return err;

    return machine_poke(machine, address, target_scan_line);
}