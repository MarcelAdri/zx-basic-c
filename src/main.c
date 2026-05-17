#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "ast.h"

// Dit zorgt ervoor dat Emscripten-functies beschikbaar zijn als we voor het web compileren
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE // Doet niets op je lokale pc, voorkomt compiler-fouten
#endif


// Deze functie maken we beschikbaar voor JavaScript!
EMSCRIPTEN_KEEPALIVE
void run_basic_line(const char *source_code) {
    const char *pointer = source_code;
    Command cmd = from_string(&pointer);

    switch (cmd.type) {
        case CMD_ERROR:
            printf("Fout: %s\n", cmd.data.error_cmd.expression_string);
            return;
        case CMD_PRINT:
            printf("%s\n", cmd.data.print_cmd.expression_string);
            return;
        default:
            printf("Fout: Onbekend commando\n");
            return;
    }
}

int main(void) {
    printf("ZX Spectrum BASIC WASM Module Geladen. 0.1.4\n");
    return 0;
}