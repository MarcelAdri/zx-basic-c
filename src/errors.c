//
// Created by Marcel on 18-05-2026.
//

#include <stdio.h>
#include "errors.h"

void error_message(const ZxError error, char *message, const size_t message_size) {

    snprintf(message, message_size, "%s", "DO NOT USE");

}

const char *get_zx_error_message(const ZxError error) {
    switch (error) {
        case ERR_0_OK: return "0 OK";
        case ERR_1_NEXT_WITHOUT_FOR: return "1 NEXT without FOR";
        case ERR_2_VARIABLE_NOT_FOUND: return "2 Variable not found";
        case ERR_3_SUBSCRIPT_WRONG: return "3 Subscript wrong";
        case ERR_4_OUT_OF_MEMORY: return "4 Out of memory";
        case ERR_5_OUT_OF_SCREEN: return "5 Out of screen";
        case ERR_6_NUMBER_TOO_BIG: return "6 Number too big";
        case ERR_7_NO_GOSUB: return "7 RETURN without GO SUB";
        case ERR_8_END_FILE: return "8 End of file";
        case ERR_9_STOP: return "9 STOP statement";
        case ERR_A_INVALID_ARGUMENT: return "A Invalid argument";
        case ERR_B_INTEGER_OUT_OF_RANGE: return "B Integer out of range";
        case ERR_C_NONSENS_IN_BASIC: return "C Nonsense in BASIC";
        case ERR_D_BREAK: return "D BREAK - CONT repeats";
        case ERR_E_OUT_OF_DATA: return "E Out of data";
        case ERR_F_INVALID_FILENAME: return "F Invalid filename";
        case ERR_G_NO_ROOM_FOR_LINE: return "G No room for line";
        case ERR_H_STOP_IN_INPUT: return "H STOP in INPUT";
        case ERR_I_NO_NEXT: return "I FOR without NEXT";
        case ERR_J_INVALID_IO: return "J Invalid I/O device";
        case ERR_K_INVALID_COLOUR: return "K Invalid colour";
        case ERR_L_BREAK_INTO_PROGRAM: return "L BREAK into program";
        case ERR_M_RAMTOP_NO_GOOD: return "M RAMTOP no good";
        case ERR_N_STATEMENT_LOST: return "N Statement lost";
        case ERR_O_INVALID_STREAM: return "O Invalid stream";
        case ERR_P_FN_WITHOUT_DEF: return "P FN without DEF";
        case ERR_Q_PARAMETER_ERROR: return "Q Parameter error";
        case ERR_R_TAPE_LOADING: return "R Tape loading error";
        case ERR_UNKNOWN: return "Unknown error (interpreter)";
        case ERR_NOT_YET_IMPLEMENTED: return "Not yet implemented";
    }
}
