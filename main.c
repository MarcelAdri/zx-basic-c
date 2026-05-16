#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Dit zorgt ervoor dat Emscripten-functies beschikbaar zijn als we voor het web compileren
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE // Doet niets op je lokale pc, voorkomt compiler-fouten
#endif

typedef enum {
    TOKEN_PRINT,
    TOKEN_STRING,
    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
} Token;

// (De scan_next_token functie blijft exact hetzelfde als voorheen)
Token scan_next_token(const char **input) {
    Token token;
    memset(&token, 0, sizeof(Token));

    while (**input == ' ') {
        (*input)++;
    }

    if (**input == '\0' || **input == '\n') {
        token.type = TOKEN_EOF;
        return token;
    }

    if (strncmp(*input, "PRINT", 5) == 0) {
        token.type = TOKEN_PRINT;
        strcpy(token.value, "PRINT");
        *input += 5;
        return token;
    }

    if (**input == '"') {
        (*input)++;
        int length = 0;
        while (**input != '"' && **input != '\0' && length < 255) {
            token.value[length] = **input;
            length++;
            (*input)++;
        }

        if (**input == '"') {
            (*input)++;
            token.type = TOKEN_STRING;
        } else {
            token.type = TOKEN_ERROR;
            strcpy(token.value, "Fout: String niet afgesloten!");
        }
        return token;
    }

    token.type = TOKEN_ERROR;
    strcpy(token.value, "Onbekend commando");
    return token;
}

// Deze functie maken we beschikbaar voor JavaScript!
EMSCRIPTEN_KEEPALIVE
void run_basic_line(const char *source_code) {
    const char *pointer = source_code;

    Token token1 = scan_next_token(&pointer);

    if (token1.type == TOKEN_PRINT) {
        Token token2 = scan_next_token(&pointer);

        if (token2.type == TOKEN_STRING) {
            // Emscripten stuurt printf direct door naar de JavaScript console/html!
            printf("%s\n", token2.value);
        } else if (token2.type == TOKEN_ERROR) {
            printf("%s\n", token2.value);
        } else {
            printf("Fout: Verwachtte een string na PRINT\n");
        }
    } else if (token1.type == TOKEN_ERROR) {
        printf("%s\n", token1.value);
    } else {
        printf("Fout: Alleen PRINT wordt ondersteund.\n");
    }
}

int main(void) {
    printf("ZX Spectrum BASIC WASM Module Geladen.\n");
    return 0;
}