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
void UI_machine_destroy(ZxMachine machine) {
    machine_destroy(machine);
}
EMSCRIPTEN_KEEPALIVE
int UI_translate_keypress(char key, char mode) {
    return get_token_from_key(key, mode);
}
// EMSCRIPTEN_KEEPALIVE
// const char* UI_get_keyword_for_token(int token) {
//     return get_content_from_token(token);
// }
// EMSCRIPTEN_KEEPALIVE
// const uint8_t* UI_get_text_screen(ZxMachine machine) {
//     return machine_get_text_screen(machine);
// }
EMSCRIPTEN_KEEPALIVE
const char* UI_get_text_screen_utf8(ZxMachine machine) {
    static char screen_utf8_buffer[4096];

    screen_utf8_buffer[0] = '\0';
    char *ptr = screen_utf8_buffer;
    size_t remaining = sizeof(screen_utf8_buffer);

    for (int y = 0; y < 22; y++) {
        for (int x = 0; x < 32; x++) {
            uint8_t token = *machine_get_from_text_screen(machine, y, x);

            const char *utf8_char = get_printable_content_from_token(token);
            size_t len = strlen(utf8_char);

            if (remaining > len + 1) {
                strcpy(ptr, utf8_char);
                ptr += len;
                remaining -= len;
            }
        }
        if (remaining > 2) {
            *ptr++ = '\n';
            remaining--;
        }
    }

    *ptr = '\0';

    return screen_utf8_buffer;
}
EMSCRIPTEN_KEEPALIVE
int UI_get_machine_state(ZxMachine machine) {
    return machine_get_state(machine);
}
EMSCRIPTEN_KEEPALIVE
void UI_resume_scroll(ZxMachine machine, uint8_t token) {
    if (machine == NULL) return;

    // Wis de "scroll?" prompt
    machine_print_to_system(machine, "");

    // Heeft de gebruiker afgebroken?
    if (is_no(token)) {
        machine_set_state(machine, ZX_STATE_IDLE);
        machine_set_scroll_reason(machine, ZX_SCROLL_REASON_NONE);
        machine_print_error(machine, ERR_D_BREAK);
        return;
    }

    // De gebruiker drukte niet op een nee-token, we gaan door!
    // Kijk naar de reden van de pauze en routeer door
    switch (machine_get_scroll_reason(machine)) {
        case ZX_SCROLL_REASON_LIST:
            // We waren aan het listen
            machine_set_state(machine, ZX_STATE_IDLE);
            machine_set_scroll_reason(machine, ZX_SCROLL_REASON_NONE);
            list_program(&machine, machine_get_scroll_resume_line(machine), false);
            break;

        case ZX_SCROLL_REASON_RUN:
            // We waren een programma aan het uitvoeren!
            machine_set_state(machine, ZX_STATE_RUNNING); // State weer actief
            machine_set_scroll_reason(machine, ZX_SCROLL_REASON_NONE);
            // TODO: run_program(&machine); <-- Die ga je in de toekomst bouwen!
            break;

        default:
            machine_set_state(machine, ZX_STATE_IDLE);
            break;
    }
}
EMSCRIPTEN_KEEPALIVE
const char* UI_get_system_screen_utf8(ZxMachine machine) {
    static char screen_utf8_buffer[4096];

    screen_utf8_buffer[0] = '\0';
    char *ptr = screen_utf8_buffer;
    size_t remaining = sizeof(screen_utf8_buffer);

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 32; x++) {
            uint8_t token = *machine_get_from_system_screen(machine, y, x);

            const char *utf8_char = get_printable_content_from_token(token);
            size_t len = strlen(utf8_char);

            if (remaining > len + 1) {
                strcpy(ptr, utf8_char);
                ptr += len;
                remaining -= len;
            }
        }
        if (remaining > 2) {
            *ptr++ = '\n';
            remaining--;
        }
    }

    *ptr = '\0';

    return screen_utf8_buffer;
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

// Deze functie maken we beschikbaar voor JavaScript!
#include <stdio.h>

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
    if (line == 0) {
        ZxError err = execute(machine, buffer + i, size - i);
        if (machine_get_state(machine) != ZX_STATE_WAIT_SCROLL) {
            machine_print_error(machine, err);
        } else {
            machine_print_to_system(machine, "scroll?");
        }


    } else {
        if (i >= size) {
            //todo: wis regel
            machine_print_to_system(machine, "0 OK, 0:1");
        } else {
            while (i < size && is_zx_space(buffer[i])) {
                i++;
            }
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

int main(void) {
    setlocale(LC_NUMERIC, "C");

    printf("ZX Spectrum BASIC WASM Module Geladen. v%s\n", ZX_BASIC_VERSION);
    return 0;
}