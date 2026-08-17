//
// Created by Marcel on 22-05-2026.
//
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "characters.h"
#include "errors.h"
#include "machine.h"

static const char *const ZX_CHARACTERS[256] = {
    #define X(name, id, str) [id] = str,
    ZX_TOKEN_MASTER_LIST
    #undef X
};

typedef enum {
    ROW_LITERAL = 0,
    ROW_ON_RED = 1,
    ROW_BELOW = 2,
    ROW_ABOVE = 3,
    ROW_KEYWORD = 4,
    NUM_KEYMAP_ROWS = 5
} ZxKeymapRow;

static const uint8_t ZX_KEYMAPS[NUM_KEYMAP_ROWS][256] = {
    [ROW_LITERAL] = {
        ['1'] = ZX_DIGIT_ONE,
        ['2'] = ZX_DIGIT_TWO,
        ['3'] = ZX_DIGIT_THREE,
        ['4'] = ZX_DIGIT_FOUR,
        ['5'] = ZX_DIGIT_FIVE,
        ['6'] = ZX_DIGIT_SIX,
        ['7'] = ZX_DIGIT_SEVEN,
        ['8'] = ZX_DIGIT_EIGHT,
        ['9'] = ZX_DIGIT_NINE,
        ['0'] = ZX_DIGIT_ZERO,
        ['a'] = ZX_LCASE_A,
        ['A'] = ZX_UCASE_A,
        ['b'] = ZX_LCASE_B,
        ['B'] = ZX_UCASE_B,
        ['c'] = ZX_LCASE_C,
        ['C'] = ZX_UCASE_C,
        ['d'] = ZX_LCASE_D,
        ['D'] = ZX_UCASE_D,
        ['e'] = ZX_LCASE_E,
        ['E'] = ZX_UCASE_E,
        ['f'] = ZX_LCASE_F,
        ['F'] = ZX_UCASE_F,
        ['g'] = ZX_LCASE_G,
        ['G'] = ZX_UCASE_G,
        ['h'] = ZX_LCASE_H,
        ['H'] = ZX_UCASE_H,
        ['i'] = ZX_LCASE_I,
        ['I'] = ZX_UCASE_I,
        ['j'] = ZX_LCASE_J,
        ['J'] = ZX_UCASE_J,
        ['k'] = ZX_LCASE_K,
        ['K'] = ZX_UCASE_K,
        ['l'] = ZX_LCASE_L,
        ['L'] = ZX_UCASE_L,
        ['m'] = ZX_LCASE_M,
        ['M'] = ZX_UCASE_M,
        ['n'] = ZX_LCASE_N,
        ['N'] = ZX_UCASE_N,
        ['o'] = ZX_LCASE_O,
        ['O'] = ZX_UCASE_O,
        ['p'] = ZX_LCASE_P,
        ['P'] = ZX_UCASE_P,
        ['q'] = ZX_LCASE_Q,
        ['Q'] = ZX_UCASE_Q,
        ['r'] = ZX_LCASE_R,
        ['R'] = ZX_UCASE_R,
        ['s'] = ZX_LCASE_S,
        ['S'] = ZX_UCASE_S,
        ['t'] = ZX_LCASE_T,
        ['T'] = ZX_UCASE_T,
        ['u'] = ZX_LCASE_U,
        ['U'] = ZX_UCASE_U,
        ['v'] = ZX_LCASE_V,
        ['V'] = ZX_UCASE_V,
        ['w'] = ZX_LCASE_W,
        ['W'] = ZX_UCASE_W,
        ['x'] = ZX_LCASE_X,
        ['X'] = ZX_UCASE_X,
        ['y'] = ZX_LCASE_Y,
        ['Y'] = ZX_UCASE_Y,
        ['z'] = ZX_LCASE_Z,
        ['Z'] = ZX_UCASE_Z,
        [27] = ZX_TOKEN_ESC,
        [' '] = ZX_CHAR_SPACE,
        ['!'] = ZX_CHAR_EXCL,
        ['\"'] = ZX_CHAR_QUOTES,
        ['#'] = ZX_CHAR_HASH,
        ['$'] = ZX_CHAR_DOLLAR,
        ['%'] = ZX_CHAR_PERCENT,
        ['&'] = ZX_CHAR_ET,
        ['\''] = ZX_CHAR_QUOTE,
        ['('] = ZX_CHAR_BRACKET_OPEN,
        [')'] = ZX_CHAR_BRACKET_CLOSE,
        ['*'] = ZX_OP_MULTIPLY,
        ['+'] = ZX_OP_PLUS,
        [','] = ZX_CHAR_COMMA,
        ['-'] = ZX_OP_MINUS,
        ['.'] = ZX_CHAR_POINT,
        ['/'] = ZX_OP_DIVIDE,
        [':'] = ZX_CHAR_COLON,
        [';'] = ZX_CHAR_SEMICOLON,
        ['<'] = ZX_OP_LESS,
        ['='] = ZX_OP_EQUAL,
        ['>'] = ZX_OP_GREATER,
        ['?'] = ZX_CHAR_QUESTION,
        ['@'] = ZX_CHAR_AT,
        ['['] = ZX_CHAR_SQ_BRACK_OPEN,
        ['\\'] = ZX_CHAR_BACKSLASH,
        [']'] = ZX_CHAR_SQ_BRACK_CLOSE,
        ['^'] = ZX_OP_POWER,
        ['_'] = ZX_CHAR_UNDERSCORE,
        ['{'] = ZX_CHAR_CRLY_BRACK_OPEN,
        ['|'] = ZX_CHAR_PIPE,
        ['}'] = ZX_CHAR_CRLY_BRACK_CLOSE,
        ['~'] = ZX_CHAR_TILDE,
    },
    [ROW_ON_RED] = {
        ['1'] = ZX_CHAR_EXCL,
        ['2'] = ZX_CHAR_AT,
        ['3'] = ZX_CHAR_HASH,
        ['4'] = ZX_CHAR_DOLLAR,
        ['5'] = ZX_CHAR_PERCENT,
        ['6'] = ZX_CHAR_ET,
        ['7'] = ZX_CHAR_POINT,
        ['8'] = ZX_CHAR_BRACKET_OPEN,
        ['9'] = ZX_CHAR_BRACKET_CLOSE,
        ['0'] = ZX_CHAR_UNDERSCORE,
        ['q'] = ZX_OP_LESS_EQ,
        ['Q'] = ZX_OP_LESS_EQ,
        ['w'] = ZX_OP_NOT_EQ,
        ['W'] = ZX_OP_NOT_EQ,
        ['e'] = ZX_OP_GTR_EQ,
        ['E'] = ZX_OP_GTR_EQ,
        ['r'] = ZX_OP_LESS,
        ['R'] = ZX_OP_LESS,
        ['t'] = ZX_OP_GREATER,
        ['T'] = ZX_OP_GREATER,
        ['y'] = ZX_OP_AND,
        ['Y'] = ZX_OP_AND,
        ['u'] = ZX_OP_OR,
        ['U'] = ZX_OP_OR,
        ['i'] = ZX_TOKEN_AT,
        ['I'] = ZX_TOKEN_AT,
        ['o'] = ZX_CHAR_SEMICOLON,
        ['O'] = ZX_CHAR_SEMICOLON,
        ['p'] = ZX_CHAR_QUOTES,
        ['P'] = ZX_CHAR_QUOTES,
        ['a'] = ZX_STATEMENT_STOP,
        ['A'] = ZX_STATEMENT_STOP,
        ['s'] = ZX_OP_NOT,
        ['S'] = ZX_OP_NOT,
        ['d'] = ZX_TOKEN_STEP,
        ['D'] = ZX_TOKEN_STEP,
        ['f'] = ZX_TOKEN_TO,
        ['F'] = ZX_TOKEN_TO,
        ['g'] = ZX_TOKEN_THEN,
        ['G'] = ZX_TOKEN_THEN,
        ['h'] = ZX_OP_POWER,
        ['H'] = ZX_OP_POWER,
        ['j'] = ZX_OP_MINUS,
        ['J'] = ZX_OP_MINUS,
        ['k'] = ZX_OP_PLUS,
        ['K'] = ZX_OP_PLUS,
        ['l'] = ZX_OP_EQUAL,
        ['L'] = ZX_OP_EQUAL,
        ['z'] = ZX_CHAR_COLON,
        ['Z'] = ZX_CHAR_COLON,
        ['x'] = ZX_CHAR_UKP,
        ['X'] = ZX_CHAR_UKP,
        ['c'] = ZX_CHAR_QUESTION,
        ['C'] = ZX_CHAR_QUESTION,
        ['v'] = ZX_OP_DIVIDE,
        ['V'] = ZX_OP_DIVIDE,
        ['b'] = ZX_OP_MULTIPLY,
        ['B'] = ZX_OP_MULTIPLY,
        ['n'] = ZX_CHAR_COMMA,
        ['N'] = ZX_CHAR_COMMA,
        ['m'] = ZX_CHAR_POINT,
        ['M'] = ZX_CHAR_POINT,
    },
    [ROW_BELOW] = {
        ['1'] = ZX_STATEMENT_DEF_FN,
        ['2'] = ZX_FUN_FN,
        ['3'] = ZX_TOKEN_LINE,
        ['4'] = ZX_STATEMENT_OPEN_H,
        ['5'] = ZX_STATEMENT_CLOSE_H,
        ['6'] = ZX_STATEMENT_MOVE,
        ['7'] = ZX_STATEMENT_ERASE,
        ['8'] = ZX_FUN_POINT,
        ['9'] = ZX_TOKEN_CAT,
        ['0'] = ZX_STATEMENT_FORMAT,
        ['q'] = ZX_FUN_ASN,
        ['Q'] = ZX_FUN_ASN,
        ['w'] = ZX_FUN_ACS,
        ['W'] = ZX_FUN_ACS,
        ['e'] = ZX_FUN_ATN,
        ['E'] = ZX_FUN_ATN,
        ['r'] = ZX_STATEMENT_VERIFY,
        ['R'] = ZX_STATEMENT_VERIFY,
        ['t'] = ZX_STATEMENT_MERGE,
        ['T'] = ZX_STATEMENT_MERGE,
        ['y'] = ZX_CHAR_SQ_BRACK_OPEN,
        ['Y'] = ZX_CHAR_SQ_BRACK_OPEN,
        ['u'] = ZX_CHAR_SQ_BRACK_CLOSE,
        ['U'] = ZX_CHAR_SQ_BRACK_CLOSE,
        ['i'] = ZX_FUN_IN,
        ['I'] = ZX_FUN_IN,
        ['o'] = ZX_STATEMENT_OUT,
        ['O'] = ZX_STATEMENT_OUT,
        ['p'] = ZX_CHAR_COPYRIGHT,
        ['P'] = ZX_CHAR_COPYRIGHT,
        ['a'] = ZX_CHAR_TILDE,
        ['A'] = ZX_CHAR_TILDE,
        ['s'] = ZX_CHAR_PIPE,
        ['S'] = ZX_CHAR_PIPE,
        ['d'] = ZX_CHAR_BACKSLASH,
        ['D'] = ZX_CHAR_BACKSLASH,
        ['f'] = ZX_CHAR_CRLY_BRACK_OPEN,
        ['F'] = ZX_CHAR_CRLY_BRACK_OPEN,
        ['g'] = ZX_CHAR_CRLY_BRACK_CLOSE,
        ['G'] = ZX_CHAR_CRLY_BRACK_CLOSE,
        ['h'] = ZX_STATEMENT_CIRCLE,
        ['H'] = ZX_STATEMENT_CIRCLE,
        ['j'] = ZX_FUN_VAL_S,
        ['J'] = ZX_FUN_VAL_S,
        ['k'] = ZX_FUN_SCREEN_S,
        ['K'] = ZX_FUN_SCREEN_S,
        ['l'] = ZX_FUN_ATTR,
        ['L'] = ZX_FUN_ATTR,
        ['z'] = ZX_STATEMENT_BEEP,
        ['Z'] = ZX_STATEMENT_BEEP,
        ['x'] = ZX_STATEMENT_INK,
        ['X'] = ZX_STATEMENT_INK,
        ['c'] = ZX_STATEMENT_PAPER,
        ['C'] = ZX_STATEMENT_PAPER,
        ['v'] = ZX_STATEMENT_FLASH,
        ['V'] = ZX_STATEMENT_FLASH,
        ['b'] = ZX_STATEMENT_BRIGHT,
        ['B'] = ZX_STATEMENT_BRIGHT,
        ['n'] = ZX_STATEMENT_OVER,
        ['N'] = ZX_STATEMENT_OVER,
        ['m'] = ZX_STATEMENT_INVERSE,
        ['M'] = ZX_STATEMENT_INVERSE,
    },
    [ROW_ABOVE] = {
        ['q'] = ZX_FUN_SIN,
        ['Q'] = ZX_FUN_SIN,
        ['w'] = ZX_FUN_COS,
        ['W'] = ZX_FUN_COS,
        ['e'] = ZX_FUN_TAN,
        ['E'] = ZX_FUN_TAN,
        ['r'] = ZX_FUN_INT,
        ['R'] = ZX_FUN_INT,
        ['t'] = ZX_FUN_RND,
        ['T'] = ZX_FUN_RND,
        ['y'] = ZX_FUN_STR_S,
        ['Y'] = ZX_FUN_STR_S,
        ['u'] = ZX_FUN_CHR_S,
        ['U'] = ZX_FUN_CHR_S,
        ['i'] = ZX_FUN_CODE,
        ['I'] = ZX_FUN_CODE,
        ['o'] = ZX_FUN_PEEK,
        ['O'] = ZX_FUN_PEEK,
        ['p'] = ZX_TOKEN_TAB,
        ['P'] = ZX_TOKEN_TAB,
        ['a'] = ZX_STATEMENT_READ,
        ['A'] = ZX_STATEMENT_READ,
        ['s'] = ZX_STATEMENT_RESTORE,
        ['S'] = ZX_STATEMENT_RESTORE,
        ['d'] = ZX_STATEMENT_DATA,
        ['D'] = ZX_STATEMENT_DATA,
        ['f'] = ZX_FUN_SGN,
        ['F'] = ZX_FUN_SGN,
        ['g'] = ZX_FUN_ABS,
        ['G'] = ZX_FUN_ABS,
        ['h'] = ZX_FUN_SQR,
        ['H'] = ZX_FUN_SQR,
        ['j'] = ZX_FUN_VAL,
        ['J'] = ZX_FUN_VAL,
        ['k'] = ZX_FUN_LEN,
        ['K'] = ZX_FUN_LEN,
        ['l'] = ZX_FUN_USR,
        ['L'] = ZX_FUN_USR,
        ['z'] = ZX_FUN_LN,
        ['Z'] = ZX_FUN_LN,
        ['x'] = ZX_FUN_EXP,
        ['X'] = ZX_FUN_EXP,
        ['c'] = ZX_STATEMENT_LPRINT,
        ['C'] = ZX_STATEMENT_LPRINT,
        ['v'] = ZX_STATEMENT_LLIST,
        ['V'] = ZX_STATEMENT_LLIST,
        ['b'] = ZX_TOKEN_BIN,
        ['B'] = ZX_TOKEN_BIN,
        ['n'] = ZX_FUN_INKEY_S,
        ['N'] = ZX_FUN_INKEY_S,
        ['m'] = ZX_FUN_PI,
        ['M'] = ZX_FUN_PI,
    },
    [ROW_KEYWORD] = {
        ['q'] = ZX_STATEMENT_PLOT,
        ['Q'] = ZX_STATEMENT_PLOT,
        ['w'] = ZX_STATEMENT_DRAW,
        ['W'] = ZX_STATEMENT_DRAW,
        ['e'] = ZX_STATEMENT_REM,
        ['E'] = ZX_STATEMENT_REM,
        ['r'] = ZX_STATEMENT_RUN,
        ['R'] = ZX_STATEMENT_RUN,
        ['t'] = ZX_STATEMENT_RANDOMIZE,
        ['T'] = ZX_STATEMENT_RANDOMIZE,
        ['y'] = ZX_STATEMENT_RETURN,
        ['Y'] = ZX_STATEMENT_RETURN,
        ['u'] = ZX_STATEMENT_IF,
        ['U'] = ZX_STATEMENT_IF,
        ['i'] = ZX_STATEMENT_INPUT,
        ['I'] = ZX_STATEMENT_INPUT,
        ['o'] = ZX_STATEMENT_POKE,
        ['O'] = ZX_STATEMENT_POKE,
        ['p'] = ZX_STATEMENT_PRINT,
        ['P'] = ZX_STATEMENT_PRINT,
        ['a'] = ZX_STATEMENT_NEW,
        ['A'] = ZX_STATEMENT_NEW,
        ['s'] = ZX_STATEMENT_SAVE,
        ['S'] = ZX_STATEMENT_SAVE,
        ['d'] = ZX_STATEMENT_DIM,
        ['D'] = ZX_STATEMENT_DIM,
        ['f'] = ZX_STATEMENT_FOR,
        ['F'] = ZX_STATEMENT_FOR,
        ['g'] = ZX_STATEMENT_GO_TO,
        ['G'] = ZX_STATEMENT_GO_TO,
        ['h'] = ZX_STATEMENT_GO_SUB,
        ['H'] = ZX_STATEMENT_GO_SUB,
        ['j'] = ZX_STATEMENT_LOAD,
        ['J'] = ZX_STATEMENT_LOAD,
        ['k'] = ZX_STATEMENT_LIST,
        ['K'] = ZX_STATEMENT_LIST,
        ['l'] = ZX_STATEMENT_LET,
        ['L'] = ZX_STATEMENT_LET,
        ['z'] = ZX_STATEMENT_COPY,
        ['Z'] = ZX_STATEMENT_COPY,
        ['x'] = ZX_STATEMENT_CLEAR,
        ['X'] = ZX_STATEMENT_CLEAR,
        ['c'] = ZX_STATEMENT_CONTINUE,
        ['C'] = ZX_STATEMENT_CONTINUE,
        ['v'] = ZX_STATEMENT_CLS,
        ['V'] = ZX_STATEMENT_CLS,
        ['b'] = ZX_STATEMENT_BORDER,
        ['B'] = ZX_STATEMENT_BORDER,
        ['n'] = ZX_STATEMENT_NEXT,
        ['N'] = ZX_STATEMENT_NEXT,
        ['m'] = ZX_STATEMENT_PAUSE,
        ['M'] = ZX_STATEMENT_PAUSE,
    },
};
static ZxKeymapRow map_mode_to_row(const char mode) {
    switch (mode) {
        case KEYMAP_MODE_LITERAL: return ROW_LITERAL;
        case KEYMAP_MODE_ON_RED:  return ROW_ON_RED;
        case KEYMAP_MODE_BELOW:   return ROW_BELOW;
        case KEYMAP_MODE_ABOVE:   return ROW_ABOVE;
        case KEYMAP_MODE_KEYWORD: return ROW_KEYWORD;
        default:                  return ROW_LITERAL;
    }
}
int get_token_from_key(const char key, const char mode) {
    unsigned char u_key = (unsigned char)key;
    ZxKeymapRow row = map_mode_to_row(mode);

    uint8_t token = ZX_KEYMAPS[row][u_key];

    if (mode == KEYMAP_MODE_KEYWORD && token == 0) {
        token = ZX_KEYMAPS[ROW_LITERAL][u_key];
    }

    return token > 0 ? token : UNDEFINED_KEYSTROKE;
}
ZxError string_to_zx_characters (const char *input, const size_t input_length, uint8_t *output, const size_t output_length, size_t *bytes_written) {
    if (input == NULL || output == NULL) {
        return ERR_UNKNOWN;
    }
    if (input_length > output_length) {
        return ERR_UNKNOWN; // Of een specifiekere foutcode zodra je die hebt
    }
    for (size_t i = 0; i < input_length; i++) {
        int get_token = get_token_from_key(input[i], KEYMAP_MODE_LITERAL);
        if (get_token == UNDEFINED_KEYSTROKE) {
            output[i] = ZX_CHAR_QUESTION;
        } else {
            output[i] = (uint8_t)get_token;
        }

    }
    if (bytes_written != NULL) {
        *bytes_written = input_length;
    }
    return ERR_0_OK;
}
const char* get_content_from_token (const uint8_t token) {
    if (ZX_CHARACTERS[token] != NULL) {
        return ZX_CHARACTERS[token];
    }
    return "?";
}
const char* get_printable_content_from_token (const uint8_t token) {
    if (token != 13 && token < 165 && ZX_CHARACTERS[token] != NULL) {
        return ZX_CHARACTERS[token];
    }
    return "?";
}

ZxError build_zx_sentence (const uint8_t *characters, const size_t length, char *result) {
    if (result == NULL || characters == NULL) {
        return ERR_UNKNOWN;
    }

    size_t len = 0;
    for (int i = 0; i < length; i++) {
        const char *token = ZX_CHARACTERS[characters[i]];
        if (token == NULL) {
            return ERR_UNKNOWN;
        }

        const size_t token_len = strlen(token);
        if  (len + token_len >= MAX_TEXT_SENTENCE_LEN - 1) {
            return ERR_UNKNOWN;
        }

        memcpy(result + len, token, token_len);
        len += token_len;

    }
    result[len] = '\0';
    return ERR_0_OK;

}

// C bepaalt de syntax-context!
char get_expected_cursor_mode(const uint8_t *buffer, const size_t length) {
    if (length == 0) return KEYMAP_MODE_KEYWORD; // Volledig lege regel = Commando/Regelnummer verwacht

    // 1. Controleer eerst op openstaande aanhalingstekens
    bool in_quotes = false;
    uint8_t quote_token = ZX_CHAR_QUOTES;
    for (size_t i = 0; i < length; i++) {
        if (buffer[i] == quote_token) {
            in_quotes = !in_quotes;
        }
    }
    if (in_quotes) {
        return KEYMAP_MODE_LITERAL; // Binnen strings altijd letterlijke invoer
    }

    // 2. Bepaal de "effectieve lengte" door alle trailing spaties aan het einde te tellen
    size_t trailing_spaces = 0;
    while (trailing_spaces < length && buffer[length - 1 - trailing_spaces] == 32) {
        trailing_spaces++;
    }

    // FIX 1: Als de regel UITSLUITEND uit spaties bestaat (een eenzame of meerdere),
    // dan verwachten we nog steeds een commando of regelnummer!
    if (trailing_spaces == length) {
        return KEYMAP_MODE_KEYWORD;
    }

    // Bereken het einde van de tekst zónder de trailing spaties
    size_t effective_length = length - trailing_spaces;
    uint8_t last_meaningful_byte = buffer[effective_length - 1];

    // Na een dubbele punt (58) of THEN (203) komt er altijd weer een keyword
    if (last_meaningful_byte == 58 || last_meaningful_byte == 203) {
        return KEYMAP_MODE_KEYWORD;
    }

    // FIX 2: Verbeterde Regelnummer-logica
    // Als er minstens één spatie is getypt (trailing_spaces > 0), én alles daavóór
    // (de effectieve lengte) bestaat puur uit cijfers, dan zijn we klaar met het regelnummer!
    if (trailing_spaces > 0) {
        bool is_line_number = true;
        for (size_t i = 0; i < effective_length; i++) {
            if (buffer[i] < 48 || buffer[i] > 57) { // Geen ASCII cijfer 0-9
                is_line_number = false;
                break;
            }
        }
        if (is_line_number) {
            return KEYMAP_MODE_KEYWORD;
        }
    }

    return KEYMAP_MODE_LITERAL; // In alle andere gevallen (midden in expressies): letters
}

bool is_zx_printable_character(const uint8_t c) {
    if (c >= 32 && c <= 128) {
        return true;
    }
    return false;
}
bool is_zx_graphics_character(const uint8_t c) {
    if (c >= 129 && c <= 164) {
        return true;
    }
    return false;
}
bool is_zx_alpha(const uint8_t c) {
    if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
        return true;
    }
    return false;
}
bool is_zx_print_modifier(const uint8_t c) {
    if (c == ZX_TOKEN_TAB ||
        c == ZX_STATEMENT_INK ||
        c == ZX_STATEMENT_PAPER ||
        c == ZX_STATEMENT_FLASH ||
        c == ZX_STATEMENT_BRIGHT ||
        c == ZX_STATEMENT_INVERSE ||
        c == ZX_STATEMENT_OVER) {
        return true;
    }
    return false;
}
bool is_zx_alnum(const uint8_t c) {
    if ((c >= 48 && c <= 57) || (c >= 65 && c <= 90) ||(c >= 97 && c <= 122)) {
        return true;
    }
    return false;
}
bool is_zx_space(const uint8_t c) {
    if (c == ZX_CHAR_SPACE) {
        return true;
    }
    return false;
}
bool is_zx_break(const uint8_t c) {
    if (c == ZX_TOKEN_ESC) {
        return true;
    }
    return false;
}
bool is_no(const uint8_t c) {
    if (c == ZX_UCASE_N ||  // N
        c == ZX_LCASE_N || // n
        c == ZX_CHAR_SPACE ||  // <space>
        c == ZX_STATEMENT_STOP) { // STOP
        return true;
        }
    return false;
}
bool is_zx_colon(const uint8_t c) {
    if (c == ZX_CHAR_COLON) {
        return true;
    }
    return false;
}
bool is_zx_plus_character(const uint8_t c) {
    if (c == ZX_OP_PLUS) {
        return true;
    }
    return false;
}
bool is_zx_minus_character(const uint8_t c) {
    if (c == ZX_OP_MINUS) {
        return true;
    }
    return false;
}
bool is_zx_asterisk_character(const uint8_t c) {
    if (c == ZX_OP_MULTIPLY) {
        return true;
    }
    return false;
}
bool is_zx_slash_character(const uint8_t c) {
    if (c == ZX_OP_DIVIDE) {
        return true;
    }
    return false;
}
bool is_zx_power_character(const uint8_t c) {
    if (c == ZX_OP_POWER) {
        return true;
    }
    return false;
}
bool is_zx_quotes(const uint8_t c) {
    if (c == ZX_CHAR_QUOTES) {
        return true;
    }
    return false;
}
bool is_zx_digit_character(const uint8_t c) {
    if (c >= 48 && c <= 57) {
        return true;
    }
    return false;
}
bool is_zx_number_character(const uint8_t c) {
    if ((c >= 48 && c <= 57) ||  //0-9
        c == ZX_CHAR_POINT ||               //.
        c == ZX_UCASE_E ||               //E
        c == ZX_LCASE_E                 //e
        ) {
        return true;
    }
    return false;
}
bool is_zx_relational_character(const uint8_t c) {
    if (c == ZX_OP_LESS  ||   // <
        c == ZX_OP_EQUAL  ||   // =
        c == ZX_OP_GREATER  ||   // >
        c == ZX_OP_LESS_EQ ||   // <=
        c == ZX_OP_GTR_EQ ||   // >=
        c == ZX_OP_NOT_EQ       // <>
        ) {
        return true;
    }
    return false;
}
bool is_zx_number_start_character(const uint8_t c) {
    if ((c >= 48 && c <= 57) || //0-9
        c == ZX_CHAR_POINT) { // .
        return true;
    }
    return false;
}
bool is_num_function_num_arg(const uint8_t c) {
    if (c == ZX_FUN_SIN ||
        c == ZX_FUN_COS ||
        c == ZX_FUN_TAN ||
        c == ZX_FUN_ASN ||
        c == ZX_FUN_ACS ||
        c == ZX_FUN_ATN ||
        c == ZX_FUN_LN ||
        c == ZX_FUN_EXP ||
        c == ZX_FUN_IN ||
        c == ZX_FUN_INT ||
        c == ZX_FUN_PEEK ||
        c == ZX_FUN_SQR ||
        c == ZX_FUN_SGN ||
        c == ZX_FUN_USR ||
        c == ZX_FUN_ABS) {
        return true;
    }
    return false;
}
bool is_num_function_no_arg(const uint8_t c) {
    if (c == ZX_FUN_RND ||
        c == ZX_FUN_PI) {
        return true;
    }
    return false;
}
bool is_num_function_str_arg(const uint8_t c) {
    if (c == ZX_FUN_VAL ||
        c == ZX_FUN_USR ||
        c == ZX_FUN_CODE ||
        c == ZX_FUN_LEN
        ) {
        return true;
    }
    return false;
}
bool is_num_function_coordinate_arg(const uint8_t c) {
    if (c == ZX_FUN_POINT ||
        c == ZX_FUN_ATTR) {
        return true;
    }
    return false;
}
bool is_num_function(const uint8_t c) {
    return is_num_function_num_arg(c) || is_num_function_no_arg(c) || is_num_function_str_arg(c) ||
        is_num_function_coordinate_arg(c);
}
bool is_string_function_str_argument(const uint8_t c) {
    if (c == ZX_FUN_VAL_S) {
        return true;
    }
    return false;
}
bool is_string_function_num_argument(const uint8_t c) {
    if (c == ZX_FUN_STR_S ||
        c == ZX_FUN_CHR_S) {
        return true;
    }
    return false;
}
bool is_string_function_no_argument(const uint8_t c) {
    if (c == ZX_FUN_INKEY_S) {
        return true;
    }
    return false;
}
bool is_string_function_coordinate_argument(const uint8_t c) {
    if (c == ZX_FUN_SCREEN_S) {
        return true;
    }
    return false;
}
bool is_string_function(const uint8_t c) {
    return is_string_function_str_argument(c) || is_string_function_num_argument(c) || is_string_function_coordinate_argument(c) ||
        is_string_function_no_argument(c);
}
bool is_argument_function(const uint8_t c) {
    return is_string_function_num_argument(c) ||
        is_string_function_str_argument(c) ||
        is_num_function_num_arg(c) ||
        is_num_function_str_arg(c);
}
bool is_coordinate_function(const uint8_t c) {
    return is_string_function_coordinate_argument(c) || is_num_function_coordinate_arg(c);
}
bool is_no_arg_function(const uint8_t c) {
    return is_num_function_no_arg(c) || is_string_function_no_argument(c);
}
bool is_function(const uint8_t c) {
    return is_num_function(c) || is_string_function(c);
}
bool is_function_no_coordinate(const uint8_t c) {
    return is_function(c) && !is_coordinate_function(c);
}
