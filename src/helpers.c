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

ZxError formatted_number(const double number, char *out_string, const size_t out_string_size) {
    if (out_string == NULL || out_string_size == 0) {
        return ERR_INVALID_ARGUMENT;
    }

    // Spectrum Rule: 0 is printed as a single digit 0
    if (number == 0.0f) {
        snprintf(out_string, out_string_size, "0");
        return ERR_OK;
    }

    // Pak de absolute waarde om de wetenschappelijke drempel veilig te testen
    double abs_num = fabs(number);

    // Spectrum Rule: Wetenschappelijke notatie buiten de grenzen
    if (abs_num <= 1e-5f || abs_num >= 1e13f) {
        int n = snprintf(out_string, out_string_size, "%.7E", number);
        if (n < 0 || (size_t)n >= out_string_size) {
            return ERR_INVALID_ARGUMENT;
        }

        char *e = strchr(out_string, 'E');
        if (!e) return ERR_OK;

        char *p = e - 1;
        while (p > out_string && *p == '0') {
            --p;
        }
        if (*p == '.') {
            --p;
        }
        memmove(p + 1, e, strlen(e) + 1);
        return ERR_OK;
    }

    // Spectrum Rule: Gewone notatie met max 8 *significante* cijfers
    // We berekenen de wiskundige grootte van het getal om de juiste precisie te bepalen.
    int order = (int)floor(log10(abs_num));
    int precision = 8 - (order + 1);
    if (precision < 0) precision = 0; // Voor hele grote getallen geen komma nodig

    int n = snprintf(out_string, out_string_size, "%.*f", precision, number);
    if (n < 0 || (size_t)n >= out_string_size) {
        return ERR_INVALID_ARGUMENT;
    }

    // Sloop overtollige nullen weg (ALLEEN als er een komma in het getal zit!)
    if (strchr(out_string, '.')) {
        char *end = out_string + strlen(out_string);
        char *p = end - 1;
        while (p > out_string && *p == '0') {
            --p;
        }
        if (*p == '.') {
            --p;
        }
        p[1] = '\0';
    }
    // Spectrum Authenticiteit: "A decimal point right at the beginning is always followed by a zero"
    // Oftewel: strip de voorloopnul bij getallen zoals 0.03, maar laat hem staan bij 0.3!
    if (out_string[0] == '0' && out_string[1] == '.' && out_string[2] == '0') {
        // Maak van "0.03" -> ".03" door alles 1 positie naar links te schuiven
        memmove(out_string, out_string + 1, strlen(out_string));
    } else if (out_string[0] == '-' && out_string[1] == '0' && out_string[2] == '.' && out_string[3] == '0') {
        // Hetzelfde voor negatieve getallen: "-0.03" -> "-.03"
        memmove(out_string + 1, out_string + 2, strlen(out_string) - 1);
    }

    return ERR_OK;
}
ZxError make_double(const char *text, double *out_double) {
    char *end = NULL;
    errno = 0;

    const double value = strtod(text, &end);

    if (end == text) {
        return ERR_INVALID_NUMBER; // geen geldig getal gevonden
    }

    while (*end == ' ') {
        end++;
    }

    if (*end != '\0') {
        return ERR_INVALID_EXPRESSION; // resttekst na het getal
    }

    if (errno == ERANGE) {
        return ERR_OUT_OF_RANGE; // buiten bereik
    }

    if (out_double != NULL) {
        *out_double = value;
    }
    return ERR_OK;
}
ZxError parse_number_to_string(const uint8_t *expression, size_t expression_size, char *number_string, const size_t result_size) {
    size_t i = 0;
    while (is_zx_space(expression[i]) && i < expression_size) {
        i++;
    }

    if (is_zx_number_start_character(expression[i])) {
        size_t len = 0;
        while (is_zx_number_character(expression[i]) &&
            len < result_size - 1 &&
            i < expression_size) {
            number_string[len] = *get_content_from_token(expression[i]);
            len++;
            i++;
            }
        if (len > 0) {
            number_string[len] = '\0';
            char result[result_size];
            double value;
            ZxError err = make_double(number_string, &value);
            if (err != ERR_OK) {
                return err;
            }
            err = formatted_number(value, result, result_size);
            if (err != ERR_OK) {
                return err;
            }
            strncpy(number_string, result, result_size);
            return ERR_OK;
        }
        return ERR_INVALID_EXPRESSION;
    }
    return ERR_INVALID_EXPRESSION;
}

ZxError parse_number_to_double(const uint8_t *expression, size_t expression_size, double *number, const size_t result_size) {
    char number_string[result_size];

    const ZxError err = parse_number_to_string(expression, expression_size, number_string, result_size);
    if (err != ERR_OK) {
        return err;
    }

    return make_double(number_string, number);
}

ZxError parse_string_literal(const uint8_t *expression, size_t expression_size, char *literal, const size_t result_size) {

    size_t i = 0;
    while (is_zx_space(expression[i]) && i < expression_size) {
        i++;
    }

    if (expression[i] == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
        size_t len = 0;
        i++;
        while (expression[i] != get_token_from_key('"', KEYMAP_MODE_LITERAL) &&
            len < result_size - 1 &&
            i < expression_size) {
            if (!is_zx_printable_character(expression[i])) {
                return ERR_INVALID_STRING_LITERAL;
            }
            literal[len] = *get_content_from_token(expression[i]);
            len++;
            i++;
        }

        if (expression[i] == get_token_from_key('"', KEYMAP_MODE_LITERAL)) {
            literal[len] = '\0';
            return ERR_OK;
        }
        return ERR_UNCLOSED_QUOTES;
    }
    return ERR_INVALID_STRING_LITERAL;
}
ZxError parse_string_literal_with_quotes(const uint8_t *expression, size_t expression_size, char *literal, const size_t result_size) {
    const ZxError err = parse_string_literal(expression, expression_size, literal, result_size);
    if (err != ERR_OK) {
        return err;
    }

    size_t len = result_size;

    while (len > 0) {
        if (literal[len] == '\0'){
            if (len == result_size) {
                len--;
            } else if (len == result_size - 1) {
                literal[len + 1] = '\0';
                literal[len] = '"';
                len--;
            } else {
                literal[len + 2] = '\0';
                literal[len + 1] = '"';
                len--;
            }
        } else {
            literal[len + 1] = literal[len];
            len--;
        }
    }
    literal[0] = '"';
    return ERR_OK;
}

ZxError parse_variable_name(const uint8_t *expression, size_t expression_size, char *variable_name) {
    size_t i = 0;
    while (is_zx_space(expression[i]) && i < expression_size) {
        i++;
    }
    if (i == expression_size) {
        return ERR_SYNTAX_ERROR;
    }

    if (is_zx_alpha(expression[i]) && i < expression_size - 1) {
        if (expression[i+1] == get_token_from_key('$', KEYMAP_MODE_LITERAL)) {
            variable_name[0] = *get_content_from_token(expression[i]);
            variable_name[1] = '$';
            variable_name[2] = '\0';
            return ERR_NOT_IMPLEMENTED; //TODO implement string variables
        }
    }
    size_t len = 0;
    while ((is_zx_alnum(expression[i]) || is_zx_space(expression[i]))
        && i < expression_size) {
        variable_name[len] = *get_content_from_token(expression[i]);
        len++;
        i++;
    }
    variable_name[len] = '\0';

    return ERR_OK;

}
