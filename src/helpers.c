//
// Created by Marcel on 19-05-2026.
//

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
//#include <tgmath.h>

#include "helpers.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>


#include "characters.h"
#include "errors.h"
#include "machine.h"
#include "execute.h"

ZxError list_program(ZxMachine *machine, uint16_t start_line, bool is_automatic) {
    if (machine == NULL) return ERR_UNKNOWN;

    ZxLine* program = machine_get_program(*machine);
    if (program == NULL) return ERR_UNKNOWN;

    uint16_t start = start_line;
    const uint16_t edit_line = machine_get_current_edit_line(*machine);

    if (is_automatic) {
        // 1. More than 22 lines?
        size_t line_counter = 0;
        for (uint16_t i = 0; i < 10000; i++) {
            if (program[i].exists) line_counter++;
        }

        if (line_counter <= 22) {
            start = 0;
        } else {
            // 2. Drape around edit_line
            line_counter = 0;
            start = edit_line;
            uint16_t current = edit_line;

            while (current > 0 && line_counter < 10) {
                if (program[current].exists) {
                    line_counter++;
                    start = current;
                }
                current--;
            }
        }
    }

    // Execute CLS
    uint8_t cmd = ZX_STATEMENT_CLS;
    size_t size = 1;
    ZxError err = execute(*machine, &cmd, size);
    if (err != ERR_0_OK) return err;

    uint16_t current_line = start;

    while (current_line < 10000) {

        if (program[current_line].exists) {

            //Line-number
            char line_num_str[8];
            if (current_line == edit_line) {
                snprintf(line_num_str, sizeof(line_num_str), "%u>", current_line);
            } else {
                snprintf(line_num_str, sizeof(line_num_str), "%u ", current_line);
            }

            ZxValue zx_line_num;
            zx_init_value(&zx_line_num);
            zx_assign_string((uint8_t*)line_num_str, strlen(line_num_str), &zx_line_num);
            machine_print_value(*machine, zx_line_num);
            zx_free_string(&zx_line_num);

            //Actual line
            ZxValue zx_tokens;
            zx_init_value(&zx_tokens);
            zx_assign_string(program[current_line].tokens, program[current_line].length, &zx_tokens);
            machine_print_value(*machine, zx_tokens);
            zx_free_string(&zx_tokens);

            // Check of het scherm vol is
            if (machine_get_text_cursor_y(*machine) >= 21) {
                if (is_automatic) {
                    return ERR_0_OK;
                } else {
                    // TODO: Hier komt later het estafette-stokje voor de "scroll?" state!
                    // Voor nu stoppen we gewoon veilig om crashes te voorkomen.
                    return ERR_0_OK;
                }
            }


            machine_next_line(*machine);
        }
        current_line++;
    }

    return ERR_0_OK;
}
ZxError formatted_number(const double number, uint8_t *out_string, const size_t out_string_size, size_t *bytes_written) {
    if (out_string == NULL || out_string_size == 0 || bytes_written == NULL) {
        return ERR_UNKNOWN;
    }

    // Spectrum Rule: 0 is printed as a single digit 0
    if (number == 0.0f) {
        out_string[0] = get_token_from_key('0', KEYMAP_MODE_LITERAL);
        *bytes_written = 1;
        return ERR_0_OK;
    }

    // Pak de absolute waarde om de wetenschappelijke drempel veilig te testen
    double abs_num = fabs(number);

    // Spectrum Rule: Wetenschappelijke notatie buiten de grenzen
    if (abs_num <= 1e-5f || abs_num >= 1e13f) {
        char werk_string[out_string_size];
        int n = snprintf(werk_string, out_string_size, "%.7E", number);
        if (n < 0 || (size_t)n >= out_string_size) {
            return ERR_A_INVALID_ARGUMENT;
        }

        char *e = strchr(werk_string, 'E');
        if (!e) {
            return string_to_zx_characters(werk_string, strlen(werk_string), out_string, out_string_size, bytes_written);
        }
        char *p = e - 1;
        while (p > werk_string && *p == '0') {
            --p;
        }
        if (*p == '.') {
            --p;
        }
        memmove(p + 1, e, strlen(e) + 1);
        return string_to_zx_characters(werk_string, strlen(werk_string), out_string, out_string_size, bytes_written);
    }

    // Spectrum Rule: Gewone notatie met max 8 *significante* cijfers
    // We berekenen de wiskundige grootte van het getal om de juiste precisie te bepalen.
    int order = (int)floor(log10(abs_num));
    int precision = 8 - (order + 1);
    if (precision < 0) precision = 0; // Voor hele grote getallen geen komma nodig
    char werk_string[out_string_size];
    int n = snprintf(werk_string, out_string_size, "%.*f", precision, number);
    if (n < 0 || (size_t)n >= out_string_size) {
        return ERR_A_INVALID_ARGUMENT;
    }

    // Sloop overtollige nullen weg (ALLEEN als er een komma in het getal zit!)
    if (strchr(werk_string, '.')) {
        char *end = werk_string + strlen(werk_string);
        char *p = end - 1;
        while (p > werk_string && *p == '0') {
            --p;
        }
        if (*p == '.') {
            --p;
        }
        p[1] = '\0';
    }
    // Spectrum Authenticiteit: "A decimal point right at the beginning is always followed by a zero"
    // Oftewel: strip de voorloopnul bij getallen zoals 0.03, maar laat hem staan bij 0.3!
    if (werk_string[0] == '0' && werk_string[1] == '.' && werk_string[2] == '0') {
        // Maak van "0.03" -> ".03" door alles 1 positie naar links te schuiven
        memmove(werk_string, werk_string + 1, strlen(werk_string));
    } else if (werk_string[0] == '-' && werk_string[1] == '0' && werk_string[2] == '.' && werk_string[3] == '0') {
        // Hetzelfde voor negatieve getallen: "-0.03" -> "-.03"
        memmove(werk_string + 1, werk_string + 2, strlen(werk_string) - 1);
    }

    return string_to_zx_characters(werk_string, strlen(werk_string), out_string, out_string_size, bytes_written);
}

ZxError make_double(const char *text, double *out_double) {
    char *end = NULL;
    errno = 0;
    const double value = strtod(text, &end);

    if (end == text) {
        return ERR_C_NONSENSE_IN_BASIC; // geen geldig getal gevonden
    }

    while (*end == ' ') {
        end++;
    }

    if (*end != '\0') {
        return ERR_C_NONSENSE_IN_BASIC; // resttekst na het getal
    }

    if (errno == ERANGE) {
        return ERR_6_NUMBER_TOO_BIG; // buiten bereik
    }

    if (out_double != NULL) {
        *out_double = value;
    }
    return ERR_0_OK;
}

ZxError parse_number_to_double(const uint8_t *expression, size_t expression_size, ZxValue *out_number, size_t *bytes_read) {
    if (expression == NULL || out_number == NULL || bytes_read == NULL) return ERR_UNKNOWN;
    char number_string[200];
    size_t i = 0;
    while (i < expression_size && is_zx_space(expression[i])) {
        i++;
    }
    if (i < expression_size && is_zx_number_start_character(expression[i])) {
        size_t len = 0;
        while (i < expression_size && len < 200 - 1 && is_zx_number_character(expression[i])) {
            number_string[len] = *get_content_from_token(expression[i]);
            len++;
            i++;
            }
        if (len > 0) {
            number_string[len] = '\0';
            double value;
            ZxError err = make_double(number_string, &value);
            if (err != ERR_0_OK) return err;
            err = zx_assign_number(value, out_number);
            if (err != ERR_0_OK) return err;
            *bytes_read = i;
            return ERR_0_OK;

        }
        return ERR_C_NONSENSE_IN_BASIC;
    }
    return ERR_C_NONSENSE_IN_BASIC;

}

ZxError parse_string_literal(const uint8_t *expression, size_t expression_size, ZxValue *literal, size_t *bytes_read) {
    if (literal == NULL || expression == NULL || bytes_read == NULL) {
        return ERR_UNKNOWN;
    }
    size_t i = 0;
    while (i < expression_size && is_zx_space(expression[i])) {
        i++;
    }
    uint8_t lit[expression_size];
    if (i < expression_size && expression[i] == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
        size_t len = 0;
        i++;
        while (i < expression_size) {

            if  (expression[i] == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
                if (i + 1 < expression_size && expression[i+1] == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
                    lit[len] = get_token_from_key('"', KEYMAP_MODE_LITERAL);
                    len++;
                    i += 2;
                    continue;
                }
                *bytes_read = i + 1;
                return zx_assign_string(lit, len, literal);
            }
            if (!is_zx_printable_character(expression[i])) {
                if (is_zx_graphics_character(expression[i])) {  //TODO: graphic characters
                    lit[len] = 96;  //ukp
                } else {
                    lit[len] = get_token_from_key('?', KEYMAP_MODE_LITERAL);
                }
            } else {
                lit[len] = expression[i];
            }
            len++;
            i++;
        }
        return ERR_A_INVALID_ARGUMENT;
    }
    return ERR_A_INVALID_ARGUMENT;
}
ZxError parse_string_literal_with_quotes(const uint8_t *expression, size_t expression_size, ZxValue *literal) {
    ZxError err;
    if (expression == NULL || literal == NULL) {
        return ERR_UNKNOWN;
    }

    size_t bytes_read;

    // 1. Haal de kale string op en stop hem direct in de ZxValue
    err = parse_string_literal(expression, expression_size, literal, &bytes_read);
    if (err != ERR_0_OK) {
        return err;
    }

    // 2. Haal de pointer en de lengte op uit de ZxValue
    uint8_t *text_in = NULL;
    size_t inner_length = 0;

    // Let op de dubbele pointer via '&text_in' !
    err = zx_get_string(*literal, &text_in, &inner_length);
    if (err != ERR_0_OK) {
        return err;
    }

    // 3. Maak een tijdelijke, kersverse array voor de ingepakte string
    uint8_t lit[inner_length + 2];

    // 4. Plak de tekst erin, maar begin bij index 1 (lit + 1)
    memcpy(lit + 1, text_in, inner_length);

    // 5. Haal de quote op en plak deze op de voor- en achterkant
    uint8_t quote_token = (uint8_t)get_token_from_key('"', KEYMAP_MODE_LITERAL);
    lit[0] = quote_token;
    lit[inner_length + 1] = quote_token;

    // 6. Overschrijf de oude ZxValue.
    // zx_assign_string fixt intern de malloc én de automatische zx_free_string!
    return zx_assign_string(lit, inner_length + 2, literal);
}

ZxError parse_variable_name(const uint8_t *expression, size_t expression_size, char *variable_name, size_t *bytes_read) {
    if (expression == NULL || variable_name == NULL || bytes_read == NULL) return ERR_UNKNOWN;
    size_t i = 0;
    while (i < expression_size && is_zx_space(expression[i])) {
        i++;
    }
    if (i == expression_size) {
        return ERR_2_VARIABLE_NOT_FOUND;
    }

    if (i < expression_size - 1 && is_zx_alpha(expression[i])) {
        if (expression[i+1] == get_token_from_key('$', KEYMAP_MODE_LITERAL)) {
            variable_name[0] = *get_content_from_token(expression[i]);
            variable_name[1] = '$';
            variable_name[2] = '\0';
            *bytes_read = i + 2;
            return ERR_0_OK;
        }
    }
    size_t len = 0;
    while (i < expression_size && (is_zx_alnum(expression[i]) || is_zx_space(expression[i]))) {
        variable_name[len] = *get_content_from_token(expression[i]);
        len++;
        i++;
    }
    variable_name[len] = '\0';

    *bytes_read = i;

    return ERR_0_OK;

}
int name_to_index(const uint8_t name) {
    int lower_name = tolower(name);
    if (lower_name >= 'a' && lower_name <= 'z') {
        return lower_name - 'a';
    }
    return -1;
}
