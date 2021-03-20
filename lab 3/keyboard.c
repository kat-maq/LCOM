#include "keyboard.h"

#include <lcom/lcf.h>

int kbd_hook_id = 1;
bool available_to_show = false;
bool make_code = true;
uint8_t scancode_size = 1;
uint8_t scancode = 0;
uint8_t complete_scancode[2];

int kbd_subscribe_int(uint8_t *bit_no) {
    *bit_no = kbd_hook_id;
    if (sys_irqsetpolicy(KEYBOARD_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &kbd_hook_id)) {
        printf("Error in subscribing keyboard interruptions.\n");
        return 1;
    }
    return 0;
}

int kbd_unsubscribe_int() {
    if (sys_irqrmpolicy(&kbd_hook_id)) {
        printf("Error in unsubscribing interrupt.\n");
        return 1;
    }
    return 0;
}

void(kbc_ih)() {
    uint8_t status = 0;
    bool invalid_scancode = false;
    available_to_show = false;

    util_sys_inb(STATUS_REGISTER, &status);
    if (status & OUTPUT_BUFFER_FULL) {
        if (status & (PARITY | TIMEOUT | AUX)) invalid_scancode = true;
        util_sys_inb(OUTPUT_BUFFER, &scancode);
        if (invalid_scancode) {
            available_to_show = false;
            return;
        }
        if (scancode == TWO_BYTES_SCANCODE) {
            scancode_size = 2;
            complete_scancode[0] = scancode;
            available_to_show = false;
            return;
        }
        if (scancode_size == 2) {
            complete_scancode[1] = scancode;
            available_to_show = true;
        } else {
            complete_scancode[0] = scancode;
            scancode_size = 1;
            available_to_show = true;
        }
        make_code = !(scancode & MSB_ACTIVATED);
    }
}

int is_input_buffer_full() {
    uint8_t status = 0;
    if (util_sys_inb(STATUS_REGISTER, &status)) return 1;
    if (status & (PARITY | TIMEOUT)) return 1;
    return (status & BIT(1));
}

int write_kbc_commands_with_return_value(uint8_t cmd, uint8_t *cmd_byte) {
    if (!is_input_buffer_full()) {
        sys_outb(KBC_COMMAND_REG, cmd);
        util_sys_inb(OUTPUT_BUFFER, cmd_byte);
        return 0;
    }
    printf("Error writing command byte (rv)\n");
    return 1;
}

int write_kbc_commands_with_arg(uint8_t cmd, uint8_t cmd_byte) {
    if (!is_input_buffer_full()) {
        sys_outb(KBC_COMMAND_REG, cmd);
        sys_outb(OUTPUT_BUFFER, cmd_byte);
        return 0;
    }
    printf("Error writing command byte (a)\n");
    return 1;
}

int enable_kbd_interrupts() {
    uint8_t cmd_byte = 0;
    if (write_kbc_commands_with_return_value(READ_COMMAND_BYTE, &cmd_byte)) return 1;
    cmd_byte |= BIT(0);
    if (write_kbc_commands_with_arg(WRITE_COMMAND_BYTE, cmd_byte)) return 1;
    return 0;
}
