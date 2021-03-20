#ifndef KBC_H
#define KBC_H

#include <lcom/lcf.h>
#include <stdint.h>

#include "i8042.h"

int kbd_subscribe_int();

int kbd_unsubscribe_int();

void(kbc_ih)();

int is_input_buffer_full();

int write_kbc_commands_with_return_value(uint8_t cmd, uint8_t *cmd_byte);

int write_kbc_commands_with_arg(uint8_t cmd, uint8_t cmd_byte);

int enable_kbd_interrupts();

int write_mouse_command(uint8_t agr_cmd);

void parse_mouse_packets();

int mouse_unsubscribe_int();

int mouse_subscribe_int(uint8_t *bit_no);

void mouse_gesture_algorithm(uint8_t x_len, uint8_t tolerance);

uint8_t read_mouse_byte();

void mouse_ph();

enum mouse_states {
    INITIAL,
    UP,
    STOP,
    DOWN,
    COMPLETE,
};

#endif
