#include <stdio.h>

#include "ast.h"
#include "machine.h"
#include "version.h"
#include "execute.h"

// Dit zorgt ervoor dat Emscripten-functies beschikbaar zijn als we voor het web compileren
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE // Doet niets op je lokale pc, voorkomt compiler-fouten
#endif

void ons_systeem_print_kanaal(const char *text) {
    printf("%s\n", text);
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
// Deze functie maken we beschikbaar voor JavaScript!
EMSCRIPTEN_KEEPALIVE
void run_basic_line(const char *source_code, ZxMachine machine) {
    if (machine == NULL) {
        printf("Fout: Geen actieve machinecontext meegegeven vanuit de UI.\n");
        return;
    }

    const char *pointer = source_code;

    // FIX: Maak cmd aan op de Stack (veilig!), in plaats van een losse pointer 'Command *cmd'
    Command cmd = {0};

    // We geven het adres van cmd mee via de & operator
    const ZxError error = execute(&machine, &pointer);
    if (error != ERR_OK) {
        char message[256];
        error_message(error, message, sizeof message);
        printf("%s\n", message);
    }

}

int main(void) {
    printf("ZX Spectrum BASIC WASM Module Geladen. v%s\n", ZX_BASIC_VERSION);
    return 0;
}