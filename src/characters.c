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
    [32] = " ",
    [33] = "!",
    [34] = "\"",
    [35] = "#",
    [36] = "$",
    [37] = "%",
    [38] = "&",
    [39] = "'",
    [40] = "(",
    [41] = ")",
    [42] = "*",
    [43] = "+",
    [44] = ",",
    [45] = "-",
    [46] = ".",
    [47] = "/",
    [48] = "0",
    [49] = "1",
    [50] = "2",
    [51] = "3",
    [52] = "4",
    [53] = "5",
    [54] = "6",
    [55] = "7",
    [56] = "8",
    [57] = "9",
    [58] = ":",
    [59] = ";",
    [60] = "<",
    [61] = "=",
    [62] = ">",
    [63] = "?",
    [64] = "@",
    [65] = "A",
    [66] = "B",
    [67] = "C",
    [68] = "D",
    [69] = "E",
    [70] = "F",
    [71] = "G",
    [72] = "H",
    [73] = "I",
    [74] = "J",
    [75] = "K",
    [76] = "L",
    [77] = "M",
    [78] = "N",
    [79] = "O",
    [80] = "P",
    [81] = "Q",
    [82] = "R",
    [83] = "S",
    [84] = "T",
    [85] = "U",
    [86] = "V",
    [87] = "W",
    [88] = "X",
    [89] = "Y",
    [90] = "Z",
    [91] = "[",
    [92] = "\\",
    [93] = "]",
    [94] = "^",
    [95] = "_",
    [96] = "£",
    [97] = "a",
    [98] = "b",
    [99] = "c",
    [100] = "d",
    [101] = "e",
    [102] = "f",
    [103] = "g",
    [104] = "h",
    [105] = "i",
    [106] = "j",
    [107] = "k",
    [108] = "l",
    [109] = "m",
    [110] = "n",
    [111] = "o",
    [112] = "p",
    [113] = "q",
    [114] = "r",
    [115] = "s",
    [116] = "t",
    [117] = "u",
    [118] = "v",
    [119] = "w",
    [120] = "x",
    [121] = "y",
    [122] = "z",
    [123] = "{",
    [124] = "|",
    [125] = "}",
    [126] = "~",
    [127] = "©",
    [128] = "↑",
    [165] = "RND ",
    [166] = "IN KEY$ ",
    [167] = "PI ",
    [168] = "FN ",
    [169] = "POINT ",
    [170] = "SCREEN$ ",
    [171] = "ATTR ",
    [172] = "AT ",
    [173] = "TAB ",
    [174] = "VAL$ ",
    [175] = "CODE ",
    [176] = "VAL ",
    [177] = "LEN ",
    [178] = "SIN ",
    [179] = "COS ",
    [180] = "TAN ",
    [181] = "ASN ",
    [182] = "ACS ",
    [183] = "ATN ",
    [184] = "LN ",
    [185] = "EXP ",
    [186] = "INT ",
    [187] = "SQR ",
    [188] = "SGN ",
    [189] = "ABS ",
    [190] = "PEEK ",
    [191] = "IN ",
    [192] = "USR ",
    [193] = "STR$ ",
    [194] = "CHR$ ",
    [195] = "NOT ",
    [196] = "BIN ",
    [197] = "OR ",
    [198] = "AND ",
    [199] = "<=",
    [200] = ">=",
    [201] = "<>",
    [202] = "LINE ",
    [203] = "THEN ",
    [204] = "TO ",
    [205] = "STEP ",
    [206] = "DEF FN ",
    [207] = "CAT ",
    [208] = "FORMAT ",
    [209] = "MOVE ",
    [210] = "ERASE ",
    [211] = "OPEN# ",
    [212] = "CLOSE# ",
    [213] = "MERGE ",
    [214] = "VERIFY ",
    [215] = "BEEP ",
    [216] = "CIRCLE ",
    [217] = "INK ",
    [218] = "PAPER ",
    [219] = "FLASH ",
    [220] = "BRIGHT ",
    [221] = "INVERSE ",
    [222] = "OVER ",
    [223] = "OUT ",
    [224] = "L PRINT ",
    [225] = "L LIST ",
    [226] = "STOP ",
    [227] = "READ ",
    [228] = "DATA ",
    [229] = "RESTORE ",
    [230] = "NEW ",
    [231] = "BORDER ",
    [232] = "CONTINUE ",
    [233] = "DIM ",
    [234] = "REM ",
    [235] = "FOR ",
    [236] = "GO TO ",
    [237] = "GO SUB ",
    [238] = "INPUT ",
    [239] = "LOAD ",
    [240] = "LIST ",
    [241] = "LET ",
    [242] = "PAUSE ",
    [243] = "NEXT ",
    [244] = "POKE ",
    [245] = "PRINT ",
    [246] = "PLOT ",
    [247] = "RUN ",
    [248] = "SAVE ",
    [249] = "RANDOMIZE ",
    [250] = "IF ",
    [251] = "CLS ",
    [252] = "DRAW ",
    [253] = "CLEAR ",
    [254] = "RETURN ",
    [255] = "COPY ",
};
static const uint8_t KEYMAP_LITERAL_MODE[256] = {
    ['1'] = 49,               // 1
    ['2'] = 50,               // 2
    ['3'] = 51,               // 3
    ['4'] = 52,               // 4
    ['5'] = 53,               // 5
    ['6'] = 54,               // 6
    ['7'] = 55,               // 7
    ['8'] = 56,               // 8
    ['9'] = 57,               // 9
    ['0'] = 48,               // 0
    ['a'] = 97, ['A'] = 65,   // aA
    ['b'] = 98, ['B'] = 66,   // bB
    ['c'] = 99, ['C'] = 67,   // cC
    ['d'] = 100, ['D'] = 68,  // dD
    ['e'] = 101, ['E'] = 69,  // eE
    ['f'] = 102, ['F'] = 70,  // fF
    ['g'] = 103, ['G'] = 71,  // gG
    ['h'] = 104, ['H'] = 72,  // hH
    ['i'] = 105, ['I'] = 73,  // iI
    ['j'] = 106, ['J'] = 74,  // jJ
    ['k'] = 107, ['K'] = 75,  // kK
    ['l'] = 108, ['L'] = 76,  // lL
    ['m'] = 109, ['M'] = 77,  // mM
    ['n'] = 110, ['N'] = 78,  // nN
    ['o'] = 111, ['O'] = 79,  // oO
    ['p'] = 112, ['P'] = 80,  // pP
    ['q'] = 113, ['Q'] = 81,  // qQ
    ['r'] = 114, ['R'] = 82,  // rR
    ['s'] = 115, ['S'] = 83,  // sS
    ['t'] = 116, ['T'] = 84,  // tT
    ['u'] = 117, ['U'] = 85,  // uU
    ['v'] = 118, ['V'] = 86,  // vV
    ['w'] = 119, ['W'] = 87,  // wW
    ['x'] = 120, ['X'] = 88,  // xX
    ['y'] = 121, ['Y'] = 89,  // yY
    ['z'] = 122, ['Z'] = 90,  // zZ
    [' '] = 32,               // space
    ['!'] = 33,               // !
    ['\"'] = 34,              // "
    ['#'] = 35,               // #
    ['$'] = 36,               // $
    ['%'] = 37,               // %
    ['&'] = 38,               // &
    ['\''] = 39,              // '
    ['('] = 40,               // (
    [')'] = 41,               // )
    ['*'] = 42,               // *
    ['+'] = 43,               // +
    [','] = 44,               // ,
    ['-'] = 45,               // -
    ['.'] = 46,               // .
    ['/'] = 47,               // /
    [':'] = 58,               // :
    [';'] = 59,               // ;
    ['<'] = 60,               // <
    ['='] = 61,               // =
    ['>'] = 62,               // >
    ['?'] = 63,               // ?
    ['@'] = 64,               // @
    ['['] = 91,               // [
    ['\\'] = 92,              // '\'
    [']'] = 93,               // ]
    ['^'] = 94,               // ^
    ['_'] = 95,               // _
    ['{'] = 123,              // {
    ['|'] = 124,              // |
    ['}'] = 125,              // }
    ['~'] = 126,              // ~
};
static const uint8_t KEYMAP_ON_RED_MODE[256] = {
    ['1'] = 33,               // !
    ['2'] = 64,               // @
    ['3'] = 35,               // #
    ['4'] = 36,               // $
    ['5'] = 37,               // %
    ['6'] = 38,               // &
    ['7'] = 46,               // .
    ['8'] = 40,               // (
    ['9'] = 41,               // )
    ['0'] = 95,               // _
    ['q'] = 199, ['Q'] = 199, // <=
    ['w'] = 201, ['W'] = 201, // <>
    ['e'] = 200, ['E'] = 200, // >=
    ['r'] = 60, ['R'] = 60,   // <
    ['t'] = 62, ['T'] = 62,   // >
    ['y'] = 198, ['Y'] = 198, //AND
    ['u'] = 197, ['U'] = 197, //OR
    ['i'] = 172, ['I'] = 172, //AT
    ['o'] = 59, ['O'] = 59,   // ;
    ['p'] = 34, ['P'] = 34,   // "
    ['a'] = 226, ['A'] = 226, //STOP
    ['s'] = 195, ['S'] = 195, //NOT
    ['d'] = 205, ['D'] = 205, //STEP
    ['f'] = 204, ['F'] = 204, //TO
    ['g'] = 203, ['G'] = 203, //THEN
    ['h'] = 128, ['H'] = 128, // ↑
    ['j'] = 45, ['J'] = 45,   // -
    ['k'] = 43, ['K'] = 43,   // +
    ['l'] = 61, ['L'] = 61,   // =
    ['z'] = 58, ['Z'] = 58,   // :
    ['x'] = 96, ['X'] = 96,   // £
    ['c'] = 63, ['C'] = 63,   // ?
    ['v'] = 47, ['V'] = 47,   // /
    ['b'] = 42, ['B'] = 42,   // *
    ['n'] = 44, ['N'] = 44,   // ,
    ['m'] = 46, ['M'] = 46,   // .
};
static const uint8_t KEYMAP_BELOW_MODE[256] = {
    ['1'] = 206,              //DEF FN
    ['2'] = 168,              //FN
    ['3'] = 202,              //LINE
    ['4'] = 211,              //OPEN#
    ['5'] = 212,              //CLOSE#
    ['6'] = 209,              //MOVE
    ['7'] = 210,              //ERASE
    ['8'] = 169,              //POINT
    ['9'] = 207,              //CAT
    ['0'] = 208,              //FORMAT
    ['q'] = 181, ['Q'] = 181, //ASN
    ['w'] = 182, ['W'] = 182, //ACS
    ['e'] = 183, ['E'] = 183, //ATN
    ['r'] = 214, ['R'] = 214, //VERIFY
    ['t'] = 213, ['T'] = 213, //MERGE
    ['y'] = 91, ['Y'] = 91,   // [
    ['u'] = 93, ['U'] = 93,   // ]
    ['i'] = 191, ['I'] = 191, //IN
    ['o'] = 223, ['O'] = 223, //OUT
    ['p'] = 127, ['P'] = 127, //<copyright>
    ['a'] = 126, ['A'] = 126, // ~
    ['s'] = 124, ['S'] = 124, // |
    ['d'] = 92, ['D'] = 92,   // '\'
    ['f'] = 123, ['F'] = 123, // {
    ['g'] = 125, ['G'] = 125, // }
    ['h'] = 216, ['H'] = 216, //CIRCLE
    ['j'] = 174, ['J'] = 174, //VAL$
    ['k'] = 170, ['K'] = 170, //SCREEN$
    ['l'] = 171, ['L'] = 171, //ATTR
    ['z'] = 215, ['Z'] = 215, //BEEP
    ['x'] = 217, ['X'] = 217, //INK
    ['c'] = 218, ['C'] = 218, //PAPER
    ['v'] = 219, ['V'] = 219, //FLASH
    ['b'] = 220, ['B'] = 220, //BRIGHT
    ['n'] = 222, ['N'] = 222, //OVER
    ['m'] = 221, ['M'] = 221, //INVERSE
};
static const uint8_t KEYMAP_ABOVE_MODE[256] = {
    // ['1'] = ,               //
    // ['2'] = ,               //
    // ['3'] = ,               //
    // ['4'] = ,               //
    // ['5'] = ,               //
    // ['6'] = ,               //
    // ['7'] = ,               //
    // ['8'] = ,               //
    // ['9'] = ,               //
    // ['0'] = ,               //
    ['q'] = 178, ['Q'] = 178, //SIN
    ['w'] = 179, ['W'] = 179, //COS
    ['e'] = 180, ['E'] = 180, //TAN
    ['r'] = 186, ['R'] = 186, //INT
    ['t'] = 165, ['T'] = 165, //RND
    ['y'] = 193, ['Y'] = 193, //STR$
    ['u'] = 194, ['U'] = 194, //CHR$
    ['i'] = 175, ['I'] = 175, //CODE
    ['o'] = 190, ['O'] = 190, //PEEK
    ['p'] = 173, ['P'] = 173, //TAB
    ['a'] = 227, ['A'] = 227, //READ
    ['s'] = 229, ['S'] = 229, //RESTORE
    ['d'] = 228, ['D'] = 228, //DATA
    ['f'] = 188, ['F'] = 188, //SGN
    ['g'] = 189, ['G'] = 189, //ABS
    ['h'] = 187, ['H'] = 187, //SQR
    ['j'] = 176, ['J'] = 176, //VAL
    ['k'] = 177, ['K'] = 177, //LEN
    ['l'] = 192, ['L'] = 192, //USR
    ['z'] = 184, ['Z'] = 184, //LN
    ['x'] = 185, ['X'] = 185, //EXP
    ['c'] = 224, ['C'] = 224, //L PRINT
    ['v'] = 225, ['V'] = 225, //L LIST
    ['b'] = 196, ['B'] = 196, //BIN
    ['n'] = 166, ['N'] = 166, //IN KEY$
    ['m'] = 167, ['M'] = 167, //PI
};
static const uint8_t KEYMAP_KEYWORD_MODE[256] = {
    // ['1'] = ,               //
    // ['2'] = ,               //
    // ['3'] = ,               //
    // ['4'] = ,               //
    // ['5'] = ,               //
    // ['6'] = ,               //
    // ['7'] = ,               //
    // ['8'] = ,               //
    // ['9'] = ,               //
    // ['0'] = ,               //
    ['q'] = 246, ['Q'] = 246, //PLOT
    ['w'] = 252, ['W'] = 252, //DRAW
    ['e'] = 234, ['E'] = 234, //REM
    ['r'] = 247, ['R'] = 247, //RUN
    ['t'] = 249, ['T'] = 249, //RANDOMIZE
    ['y'] = 254, ['Y'] = 254, //RETURN
    ['u'] = 250, ['U'] = 250, //IF
    ['i'] = 238, ['I'] = 238, //INPUT
    ['o'] = 244, ['O'] = 244, //POKE
    ['p'] = 245, ['P'] = 245, //PRINT
    ['a'] = 230, ['A'] = 230, //NEW
    ['s'] = 248, ['S'] = 248, //SAVE
    ['d'] = 233, ['D'] = 233, //DIM
    ['f'] = 235, ['F'] = 235, //FOR
    ['g'] = 236, ['G'] = 236, //GO TO
    ['h'] = 237, ['H'] = 237, //GO SUB
    ['j'] = 239, ['J'] = 239, //LOAD
    ['k'] = 240, ['K'] = 240, //LIST
    ['l'] = 241, ['L'] = 241, //LET
    ['z'] = 255, ['Z'] = 255, //COPY
    ['x'] = 253, ['X'] = 253, //CLEAR
    ['c'] = 232, ['C'] = 232, //CONT
    ['v'] = 251, ['V'] = 251, //CLS
    ['b'] = 231, ['B'] = 231, //BORDER
    ['n'] = 243, ['N'] = 243, //NEXT
    ['m'] = 242, ['M'] = 242, //PAUSE
};

int get_token_from_key (const char key, const char mode) {
    uint8_t token = 0;

    // Voorkom crashes bij vreemde invoer door de char veilig te casten
    unsigned char u_key = (unsigned char)key;

    if (mode == KEYMAP_MODE_KEYWORD) {
        token = KEYMAP_KEYWORD_MODE[u_key];
        // HET VANGNET: Als het geen commando is, val terug op de Literal (L) modus
        // Hierdoor kun je regelnummers en spaties typen in K-modus!
        if (token == 0) {
            token = KEYMAP_LITERAL_MODE[u_key];
        }
    }
    else if (mode == KEYMAP_MODE_LITERAL) token = KEYMAP_LITERAL_MODE[u_key];
    else if (mode == KEYMAP_MODE_ON_RED) token = KEYMAP_ON_RED_MODE[u_key];
    else if (mode == KEYMAP_MODE_ABOVE) token = KEYMAP_ABOVE_MODE[u_key];
    else if (mode == KEYMAP_MODE_BELOW) token = KEYMAP_BELOW_MODE[u_key];

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
            output[i] = (uint8_t)get_token_from_key('?', KEYMAP_MODE_LITERAL);
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
    const char* key = NULL;

    if (ZX_CHARACTERS[token] != NULL) {
        return ZX_CHARACTERS[token];
    }
    return "";
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
    if (length == 0) return KEYMAP_MODE_KEYWORD; // Lege regel = Commando verwacht

    bool in_quotes = false;
    uint8_t quote_token = get_token_from_key('"', KEYMAP_MODE_LITERAL); // Vaak byte 34

    for (size_t i = 0; i < length; i++) {
        if (buffer[i] == quote_token) {
            in_quotes = !in_quotes; // Toggle de status bij elke quote die we zien
        }
    }
    if (in_quotes) {
        return KEYMAP_MODE_LITERAL;
    }
    uint8_t last_byte = buffer[length - 1];

    // Na een dubbele punt of THEN komt er altijd een commando
    // (Gebruik hier straks je mooie #define namen zoals TOKEN_THEN in plaats van 203!)
    if (last_byte == 58 || last_byte == 203) {
        return KEYMAP_MODE_KEYWORD;
    }

    // Regelnummer logica: zijn we geëindigd met een spatie en waren het daarvoor alleen cijfers?
    if (last_byte == 32 && length > 1) {
        int is_line_number = 1;
        for (size_t i = 0; i < length - 1; i++) {
            if (buffer[i] < 48 || buffer[i] > 57) { // ASCII 0-9
                is_line_number = 0;
                break;
            }
        }
        if (is_line_number) return KEYMAP_MODE_KEYWORD;
    }

    return KEYMAP_MODE_LITERAL; // In alle andere gevallen: gewone letters
}

bool is_zx_printable_character(const uint8_t c) {
    if (c >= 32 && c <= 128) {
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
bool is_zx_alnum(const uint8_t c) {
    if ((c >= 48 && c <= 57) || (c >= 65 && c <= 90) ||(c >= 97 && c <= 122)) {
        return true;
    }
    return false;
}
bool is_zx_space(const uint8_t c) {
    if (c == 32) {
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
bool is_zx_number_character(const uint8_t c) {
    if ((c >= 48 && c <= 57) ||  //0-9
        c == 46 ||               //.
        c == 44 ||               //,
        c == 69 ||               //E
        c == 101                 //e
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
        c == 46) { // .
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
        c == ZX_FUN_ABS ||
        c == 195) {   //NOT
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
    if (c == 176 ||  //VAL
        c == ZX_FUN_USR ||
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
