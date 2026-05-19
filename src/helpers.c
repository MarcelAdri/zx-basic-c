//
// Created by Marcel on 19-05-2026.
//

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

bool is_valid_number(const char *text) {
    char *end = NULL;
    errno = 0;

    float value = strtof(text, &end);

    if (end == text) {
        return false; // geen geldig getal gevonden
    }

    while (*end == ' ') {
        end++;
    }

    if (*end != '\0') {
        return false; // resttekst na het getal
    }

    if (errno == ERANGE) {
        return false; // buiten bereik
    }

    return true;
}
