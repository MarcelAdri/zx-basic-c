#include <stdio.h>
#include <stdint.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "machine.h"
#include "version.h"
#include "execute.h"
#include "characters.h"
#include "helpers.h"
#include "main.h"

#include "screen.h"

// Dit zorgt ervoor dat Emscripten-functies beschikbaar zijn als we voor het web compileren
#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#else
    #define EMSCRIPTEN_KEEPALIVE

    // Fopspeen voor Clangd: vervang macro's lokaal door lege declaraties
    #define EM_ASM(...)
    #define EM_ASM_(...)

    // De magische fopspeen voor EM_JS:
    // Vertaalt EM_JS(void, naam, (), { JS }) naar -> void naam() {}
    #define EM_JS(ret, name, params, ...) ret name params { }
#endif

// 1. C roept deze functie aan als hij het LOAD commando leest
EM_JS(void, UI_trigger_load, (ZxMachine machine), {
    // Dit is pure JavaScript!
    triggerLoadTape(machine);
});

// 2. C roept deze functie aan als hij SAVE "naam" leest
EM_JS(void, UI_trigger_save, (ZxMachine machine, const char* filename_ptr), {
    // We moeten de C-pointer naar de string eerst omzetten naar een JavaScript string
    const jsFilename = UTF8ToString(filename_ptr);
    triggerSaveTape(machine, jsFilename);
});
EM_JS(void, UI_trigger_edit, (uint16_t line_number, const uint8_t* tokens, size_t length), {
    loadTokensIntoEditor(line_number, tokens, length);
});

static void machine_print_error(ZxMachine machine, ZxError err) {
    const char *msg = get_zx_error_message(err);
    uint16_t cur_line = machine_get_current_line(machine);
    uint8_t stmt = machine_get_current_statement(machine);

    char sys_msg[64];
    snprintf(sys_msg, sizeof(sys_msg), "%s, %u:%d", msg, cur_line, stmt);

    machine_print_to_system(machine, sys_msg);
}
void ons_systeem_print_kanaal(const char *text) {
    printf("%s", text);
}
EMSCRIPTEN_KEEPALIVE
ZxMachine UI_machine_create(void) {
    ZxMachine machine = machine_create();
    if (machine != NULL) {
        // Koppel ons print-kanaal aan de machine!
        machine_set_print_callback(machine, ons_systeem_print_kanaal);
    }
    return machine;
}
EMSCRIPTEN_KEEPALIVE
void UI_render_input_line(ZxMachine machine, const uint8_t *tokens, size_t length, size_t cursor_pos, char cursor_mode) {
    if (machine == NULL) return;

    // 1. Wis de onderste twee regels van VRAM
    screen_clear_sys(machine);

    char left_text[512] = {0};
    char right_text[512] = {0};

    // 2. Converteer tokens vóór en na de cursor naar tekst
    if (cursor_pos > 0 && tokens != NULL) {
        build_zx_sentence(tokens, cursor_pos, left_text);
    }
    if (tokens != NULL && length > cursor_pos) {
        build_zx_sentence(tokens + cursor_pos, length - cursor_pos, right_text);
    }

    size_t total_chars = strlen(left_text) + 1 + strlen(right_text); // +1 voor het cursorblok

    // 3. Bepaal startregel:
    // Past het op 1 regel (<= 32)? Begin onderaan op regel 23 (rel_y = 1).
    // Langer dan 32 tekens? Begin bovenaan op regel 22 (rel_y = 0),
    // zodat regel 1 boven staat en regel 2 er netjes onder op regel 23 verschijnt.
    uint8_t start_y = (total_chars <= 32) ? 1 : 0;
    screen_set_sys_cursor(machine, start_y, 0);

    // 4. Tekst vóór de cursor printen
    for (size_t i = 0; left_text[i] != '\0'; i++) {
        screen_put_sys_char(machine, (uint8_t)left_text[i]);
    }

    // 5. Knipperend Sinclair cursorblok met actieve modus (K, L, E, F, G)
    screen_put_sys_char_attr(machine, (uint8_t)cursor_mode, 0x87);

    // 6. Tekst na de cursor printen
    for (size_t i = 0; right_text[i] != '\0'; i++) {
        screen_put_sys_char(machine, (uint8_t)right_text[i]);
    }
}
EMSCRIPTEN_KEEPALIVE
uint32_t* UI_get_framebuffer_ptr(void) {
    return screen_get_framebuffer();
}
EMSCRIPTEN_KEEPALIVE
void UI_update_frame(ZxMachine machine, bool flash_state) {
    screen_render_frame(machine, flash_state);
}
EMSCRIPTEN_KEEPALIVE
void UI_machine_destroy(ZxMachine machine) {
    machine_destroy(machine);
}
EMSCRIPTEN_KEEPALIVE
int UI_translate_keypress(char key, char mode) {
    return get_token_from_key(key, mode);
}
EMSCRIPTEN_KEEPALIVE
bool UI_get_flash_invert(ZxMachine machine) {
    if (machine == NULL) return false;
    // Knippert elke 16 frames (authentieke Sinclair timing)
    return (machine_get_frames(machine) / 16) % 2 != 0;
}
EMSCRIPTEN_KEEPALIVE
int UI_get_machine_state(ZxMachine machine) {
    return machine_get_state(machine);
}
EMSCRIPTEN_KEEPALIVE
int UI_get_wait_reason(ZxMachine machine) {
    return machine_get_wait_reason(machine);
}
EMSCRIPTEN_KEEPALIVE
void UI_execute_batch(ZxMachine machine, int max_statements) {
    if (machine == NULL) return;

    for (int i = 0; i < max_statements; i++) {

        // 1. Voer één statement uit (Slicer + Switch)
        ZxError err = execute_single_step(machine);

        // 2. Trekt de machine aan de noodrem voor UI-interactie?
        if (machine_get_wait_reason(machine) != ZX_WAIT_NONE) {

            if (machine_get_wait_reason(machine) == ZX_WAIT_SCROLL) {
                machine_print_to_system(machine, "scroll?");
            }
            // (Later kun je hier INPUT of PAUSE afhandelen)

            break; // Stop de batch! JavaScript (via UI_resume) lost het verder op.
        }

        // 3. Is er een runtime error gecrasht? (bijv. Division by Zero)
        if (err != ERR_0_OK) {
            machine_set_state(machine, ZX_STATE_IDLE); // Forceer stop
            machine_print_error(machine, err);         // Print de foutmelding
            break;
        }

        // 4. Zijn we natuurlijk klaargekomen met de code?
        if (machine_get_state(machine) == ZX_STATE_IDLE) {
            machine_print_error(machine, ERR_0_OK); // Print "0 OK"
            break;
        }
    }
}
EMSCRIPTEN_KEEPALIVE
void UI_resume(ZxMachine machine, uint8_t token) {
    if (machine == NULL) return;

    // Waarom stonden we stil?
    switch (machine_get_wait_reason(machine)) {
        case ZX_WAIT_NONE:    // We stonden niet stil, we horen hier niet te zijn
            return;

        case ZX_WAIT_SCROLL:
            if (is_no(token)) {
                // 1. De gebruiker drukt op 'N' of 'STOP': we breken af!
                machine_set_old_line(machine);             // Sla huidige positie op voor CONTINUE
                machine_set_wait_reason(machine, ZX_WAIT_NONE); // Hef de blokkade op
                machine_set_state(machine, ZX_STATE_IDLE); // Terug naar de '>' prompt
                machine_print_error(machine, ERR_D_BREAK); // Toon "D BREAK - CONT repeats"
                return;
            }

            // 2. De gebruiker drukt op Spatie (of een andere toets): we gaan door!
            machine_print_to_system(machine, "");          // Wis de "scroll?" tekst onderin
            machine_set_wait_reason(machine, ZX_WAIT_NONE); // Hef de blokkade op
            break;

        case ZX_WAIT_INPUT:  // TODO
        case ZX_WAIT_PAUSE:
            machine_set_wait_reason(machine, ZX_WAIT_NONE);
            break;
    }
}
EMSCRIPTEN_KEEPALIVE
void UI_request_edit_current_line(ZxMachine machine) {
    if (machine == NULL) return;

    uint16_t line_number = 0;
    uint8_t tokens[2048] = {0};
    size_t tokens_len = 0;
    machine_retrieve_current_edit_line(machine, &line_number, tokens, &tokens_len);

    if (tokens_len > 0) {
         UI_trigger_edit(line_number, tokens, tokens_len);
    }
}
EMSCRIPTEN_KEEPALIVE
const char* UI_format_zx_line(const uint8_t *buffer, const size_t length) {
    // Statische buffer, zodat deze in het geheugen blijft bestaan nadat de functie klaar is
    static char formatted_output[2048];

    if (length == 0 || buffer == NULL) {
        return ""; // Lege invoer is een lege string
    }

    ZxError err = build_zx_sentence(buffer, length, formatted_output);
    if (err == ERR_0_OK) {
        return formatted_output; // Geef de prachtig geformatteerde tekst terug!
    }

    // DE VEILIGE EN MODERNE FIX:
    // We gebruiken nu get_zx_error_message en verpakken de fout netjes
    // zodat de UI-gebruiker ziet dat het om een format-probleem gaat.
    snprintf(formatted_output, sizeof(formatted_output), "[Format Error: %s]", get_zx_error_message(err));

    return formatted_output;
}
EMSCRIPTEN_KEEPALIVE
char UI_get_cursor_mode(const uint8_t *buffer, const size_t length) {
    return get_expected_cursor_mode(buffer, length);
}


EMSCRIPTEN_KEEPALIVE
void run_basic_line(const uint8_t *buffer, size_t size, ZxMachine machine) {
    if (machine == NULL || buffer == NULL || size == 0) return;

    size_t i = 0;
    while (i < size && is_zx_space(buffer[i])) {
        i++;
    }
    if (i >= size) return;
    uint16_t line = 0;
    if (is_zx_digit_character(buffer[i])) {
        char line_number[5];
        size_t j = 0;
        while (i < size && is_zx_digit_character(buffer[i])) {
            if (j < sizeof(line_number)) {
                line_number[j++] = (char)buffer[i++];
            } else {
                i++;
                j++;
            }
        }
        if (j > sizeof(line_number) - 1) {
            ZxError err = ERR_B_INTEGER_OUT_OF_RANGE;
            machine_print_error(machine, err);
            return;
        } else {
            line_number[j] = '\0';
            line = atoi(line_number);
        }

    }
    while (i < size && is_zx_space(buffer[i])) {
        i++;
    }

    if (line == 0) {
        machine_print_to_system(machine, "");
        // 1. Het is een direct commando. Zet het in de speciale buffer.
        machine_set_direct_buffer(machine, buffer + i, size - i);

        // 2. Zet het stoplicht op groen voor JavaScript!
        machine_set_state(machine, ZX_STATE_DIRECT);

    } else {
        if (i >= size) {
            ZxError err = machine_delete_line(machine, line);
            if (err != ERR_0_OK) {
                machine_print_error(machine, err);
            } else {
                list_program(&machine, 0, true);
                machine_print_error(machine, ERR_0_OK);
            }
        } else {
            
            ZxError err = machine_insert_line(machine, line, buffer + i, size - i);
            if (err != ERR_0_OK) {
                machine_print_error(machine, err);
            } else {
                machine_set_current_edit_line(machine, line);
                list_program(&machine, 0, true);

                machine_print_error(machine, ERR_0_OK);
            }
        }
    }

}
EMSCRIPTEN_KEEPALIVE
const char* UI_get_version(void) {
    return ZX_BASIC_VERSION;
}
EMSCRIPTEN_KEEPALIVE
uint8_t* UI_serialize_program(ZxMachine machine, size_t* out_size) {
    // Roep de originele functie uit helper.c aan
    return machine_serialize_program(machine, out_size);
}

EMSCRIPTEN_KEEPALIVE
ZxError UI_deserialize_program(ZxMachine machine, const uint8_t* buffer, size_t size) {
    // Roep de originele functie uit helper.c aan
    return machine_deserialize_program(machine, buffer, size);
}
EMSCRIPTEN_KEEPALIVE
void UI_move_cursor_up(ZxMachine machine) {
    if (machine == NULL) return;
    uint16_t line = machine_get_current_edit_line(machine);
    ZxLine* program = machine_get_program(machine);

    // 1. Zoek achteruit naar de vorige bestaande regel
    uint16_t prev_line = line;
    while (prev_line > 0) {
        prev_line--;
        if (program[prev_line].exists) {
            machine_set_current_edit_line(machine, prev_line);
            list_program(&machine, 0, true);
            return;
        }
    }

    // 2. SPOOKREGEL FIX: We vonden niets naar boven.
    // Bestaat de regel waar we nu op staan eigenlijk wel?
    if (!program[line].exists) {
        // Nee! Zoek als reddingsboei de dichtstbijzijnde regel naar beneden.
        uint16_t fallback = 0;
        while (fallback < 9999) {
            fallback++;
            if (program[fallback].exists) {
                machine_set_current_edit_line(machine, fallback);
                list_program(&machine, 0, true);
                return;
            }
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void UI_move_cursor_down(ZxMachine machine) {
    if (machine == NULL) return;
    uint16_t line = machine_get_current_edit_line(machine);
    ZxLine* program = machine_get_program(machine);

    // 1. Zoek vooruit naar de volgende bestaande regel
    uint16_t next_line = line;
    while (next_line < 9999) {
        next_line++;
        if (program[next_line].exists) {
            machine_set_current_edit_line(machine, next_line);
            list_program(&machine, 0, true);
            return;
        }
    }

    // 2. SPOOKREGEL FIX: We vonden niets naar beneden.
    // Bestaat de regel waar we nu op staan eigenlijk wel?
    if (!program[line].exists) {
        // Nee! Zoek als reddingsboei de dichtstbijzijnde regel naar boven.
        uint16_t fallback = 9999;
        while (fallback > 0) {
            fallback--;
            if (program[fallback].exists) {
                machine_set_current_edit_line(machine, fallback);
                list_program(&machine, 0, true);
                return;
            }
        }
    }
}
EMSCRIPTEN_KEEPALIVE
void UI_tick_frame(ZxMachine machine) {
    machine_tick_frame(machine);
}
EMSCRIPTEN_KEEPALIVE
void UI_set_pressed_key(ZxMachine machine, uint8_t token) {
    machine_set_pressed_key(machine, token);
}
EMSCRIPTEN_KEEPALIVE
void UI_clear_pressed_key(ZxMachine machine, uint8_t token) {
    if (machine_get_pressed_key(machine) == token) {
        machine_set_pressed_key(machine, 0);
    }
}

int main(void) {
    setlocale(LC_NUMERIC, "C");

    printf("ZX Spectrum BASIC WASM Module Geladen. v%s\n", ZX_BASIC_VERSION);
    return 0;
}