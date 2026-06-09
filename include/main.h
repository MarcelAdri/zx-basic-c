//
// Created by Marcel on 08-06-2026.
//

#ifndef ZX_BASIC_C_MAIN_H
#define ZX_BASIC_C_MAIN_H

void UI_trigger_load(ZxMachine machine);
void UI_trigger_save(ZxMachine machine, const char* filename);
void UI_trigger_edit(uint16_t line_number, const uint8_t* tokens, size_t length);

#endif //ZX_BASIC_C_MAIN_H
