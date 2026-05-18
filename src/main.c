#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "ast.h"
#include "machine.h"
#include "version.h"

// Dit zorgt ervoor dat Emscripten-functies beschikbaar zijn als we voor het web compileren
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE // Doet niets op je lokale pc, voorkomt compiler-fouten
#endif

EMSCRIPTEN_KEEPALIVE
ZxMachine UI_machine_create(void) {
    return machine_create();
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
    const ZxError error = command_from_string(&pointer, &cmd);
    switch (error) {
        case ERR_OK:
            switch (cmd.type) {
            case CMD_PRINT:
                    printf("%s\n", cmd.data.print_cmd.expression_string);
                    return;
            }
            break;
        default: {
            const size_t message_size = 256;
            char message[message_size];
            error_message(error, message, message_size);
            printf("%s\n", message);
            return;
        }
    }
}

int main(void) {
    printf("ZX Spectrum BASIC WASM Module Geladen. v%s\n", ZX_BASIC_VERSION);
    return 0;
}