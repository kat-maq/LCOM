#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <lcom/lcf.h>

#include "i8042.h"

int kbd_subscribe_int();

int kbd_unsubscribe_int();

void(kbc_ih)();

int is_input_buffer_full();

int write_kbc_commands_with_return_value(uint8_t cmd, uint8_t *cmd_byte);

int write_kbc_commands_with_arg(uint8_t cmd, uint8_t cmd_byte);

int enable_kbd_interrupts();

#endif
