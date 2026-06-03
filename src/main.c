#include <stdio.h>
#include <stdint.h>
#include <locale.h>

#include "machine.h"
#include "version.h"
#include "execute.h"
#include "characters.h"

// Dit zorgt ervoor dat Emscripten-functies beschikbaar zijn als we voor het web compileren
#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#else
    #define EMSCRIPTEN_KEEPALIVE

    // Fopspeen voor Clangd: vervang EM_ASM lokaal door absoluut niets
    #define EM_ASM(...)
    #define EM_ASM_(...)
#endif

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
EMSCRIPTEN_KEEPALIVE
const char* UI_get_keyword_for_token(int token) {
    return get_content_from_token(token);
}
EMSCRIPTEN_KEEPALIVE
const uint8_t* UI_get_text_screen(ZxMachine machine) {
    // We geven simpelweg de pointer naar de 22x32 array terug!
    return machine_get_text_screen(machine);
}
EMSCRIPTEN_KEEPALIVE
int UI_get_machine_state(ZxMachine machine) {
    return machine_get_state(machine);
}
EMSCRIPTEN_KEEPALIVE
const uint8_t* UI_get_system_screen(ZxMachine machine) {
    return machine_get_system_screen(machine);
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

    // 1. Voer de BASIC code uit!
    ZxError err = execute(machine, buffer, size);

    // 2. Haal de foutmelding op (bijv "0 OK" of "C Nonsense in BASIC")
    const char *msg = get_zx_error_message(err);
    uint16_t line = machine_get_current_line(machine);
    uint8_t stmt = machine_get_current_statement(machine);

    // 3. Formatteer hem zoals een echte Spectrum (met statement pointer)
    char sys_msg[64];
    snprintf(sys_msg, sizeof(sys_msg), "%s, %u:%d", msg, line, stmt);

    // 4. Stuur hem naar het systeem-scherm
    machine_print_to_system(machine, sys_msg);
}
EMSCRIPTEN_KEEPALIVE
const char* UI_get_version(void) {
    return ZX_BASIC_VERSION;
}

int main(void) {
    setlocale(LC_NUMERIC, "C");

    printf("ZX Spectrum BASIC WASM Module Geladen. v%s\n", ZX_BASIC_VERSION);
    return 0;
}