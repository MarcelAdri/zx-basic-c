#include <stdio.h>
#include <stdint.h>

#include "machine.h"
#include "version.h"
#include "execute.h"
#include "characters.h"

// Dit zorgt ervoor dat Emscripten-functies beschikbaar zijn als we voor het web compileren
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE // Doet niets op je lokale pc, voorkomt compiler-fouten
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
const char* UI_format_zx_line(const uint8_t *buffer, const size_t length) {
    // Statische buffer, zodat deze in het geheugen blijft bestaan nadat de functie klaar is
    static char formatted_output[2048];

    if (length == 0 || buffer == NULL) {
        return ""; // Lege invoer is een lege string
    }

    ZxError err = build_zx_sentence(buffer, length, formatted_output);
    if (err == ERR_OK) {
        return formatted_output; // Geef de prachtig geformatteerde tekst terug!
    }
    error_message(err, formatted_output, sizeof formatted_output);
    return formatted_output;
}
EMSCRIPTEN_KEEPALIVE
char UI_get_cursor_mode(const uint8_t *buffer, const size_t length) {
    return get_expected_cursor_mode(buffer, length);
}

// Deze functie maken we beschikbaar voor JavaScript!
EMSCRIPTEN_KEEPALIVE
void run_basic_line(const uint8_t *buffer, size_t length, ZxMachine machine) {
    if (machine == NULL) {
        printf("Fout: Geen actieve machinecontext meegegeven vanuit de UI.\n");
        return;
    }

    if (buffer == NULL || length == 0) {
        printf("Fout: Geen opdracht ontvangen.\n");
        return;
    }
    const ZxError err = execute(machine, buffer, length);
    if (err != ERR_OK) {
        char error_message_text[256];
        error_message(err, error_message_text, sizeof error_message_text);
        printf("%s\n", error_message_text);
    }
}

int main(void) {
    printf("ZX Spectrum BASIC WASM Module Geladen. v%s\n", ZX_BASIC_VERSION);
    return 0;
}