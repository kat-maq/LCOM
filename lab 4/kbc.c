#include "kbc.h"

#include <lcom/lcf.h>

static int kbd_hook_id = 1;
static int mouse_hook_id = 12;
static bool available_to_show = false;
static bool make_code = true;
static uint8_t scancode_size = 1;
static uint8_t scancode = 0;
static uint8_t complete_scancode[2];
uint8_t byte_counter = 0;
static uint8_t bytes[3];
static int total_delta_y = 0;
static int total_delta_x = 0;
enum mouse_states state;
struct packet pp;

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
    }
    util_sys_inb(OUTPUT_BUFFER, cmd_byte);
    return 0;

    printf("Error writing command byte (rv)\n");
    return 1;
}

int write_kbc_commands_with_arg(uint8_t cmd, uint8_t cmd_byte) {
    if (!is_input_buffer_full()) {
        sys_outb(KBC_COMMAND_REG, cmd);
    }
    if (!is_input_buffer_full()) {
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

int write_mouse_command(uint8_t arg_cmd) {
    uint8_t ack = 0;
    uint8_t status = 0;
    while (ack != ACK_OK) {
        if (write_kbc_commands_with_arg(MOUSE_COMMAND, arg_cmd)) continue;
        if (util_sys_inb(STATUS_REGISTER, &status)) continue;
        if (status & BIT(0)) util_sys_inb(OUTPUT_BUFFER, &ack);
    }
    return 0;
}

void(mouse_ih)() {
    uint8_t status = 0;
    util_sys_inb(STATUS_REGISTER, &status);
    if (status & (PARITY | TIMEOUT)) return;
    util_sys_inb(OUTPUT_BUFFER, &bytes[byte_counter]);
    if (byte_counter == 0 && !(bytes[byte_counter] & BIT(3))) return;
    byte_counter++;
}

void parse_mouse_packets() {
    pp.bytes[0] = bytes[0];
    pp.bytes[1] = bytes[1];
    pp.bytes[2] = bytes[2];
    pp.lb = bytes[0] & BIT(0);
    pp.rb = bytes[0] & BIT(1);
    pp.mb = bytes[0] & BIT(2);
    pp.x_ov = bytes[0] & BIT(6);
    pp.y_ov = bytes[0] & BIT(7);
    pp.delta_x = (bytes[0] & BIT(4)) ? (0xFF00 | bytes[1]) : bytes[1];
    pp.delta_y = (bytes[0] & BIT(5)) ? (0xFF00 | bytes[2]) : bytes[2];
}

int mouse_subscribe_int(uint8_t *bit_no) {
    *bit_no = mouse_hook_id;
    if (sys_irqsetpolicy(MOUSE_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &mouse_hook_id) != OK) {
        printf("Error subscribing mouse interrupts\n");
        return 1;
    }
    return 0;
}

int mouse_unsubscribe_int() {
    if (sys_irqrmpolicy(&mouse_hook_id) != OK) {
        printf("Error unsubscribing mouse interrupts\n");
        return 1;
    }
    return 0;
}

void mouse_gesture_algorithm(uint8_t x_len, uint8_t tolerance) {
    switch (state) {
        case INITIAL:
            if (pp.lb && !pp.rb && !pp.mb) state = UP;
            printf("initial\n");
            break;
        case UP:
            printf("up\n");
            if (pp.rb || pp.mb || pp.delta_x < (-1 * abs(tolerance)) || pp.delta_y < (-1 * abs(tolerance))) {
                state = INITIAL;
                break;
            }
            total_delta_x = pp.delta_x > 0 ? total_delta_x + pp.delta_x : total_delta_x;
            total_delta_y = pp.delta_y > 0 ? total_delta_y + pp.delta_y : total_delta_y;
            if (!pp.lb) {
                if ((total_delta_y / total_delta_x) <= 1 || total_delta_x < x_len)
                    state = INITIAL;
                else
                    state = STOP;
            }
            break;
        case STOP:
            printf("stop\n");
            total_delta_x = 0;
            total_delta_y = 0;
            if (pp.rb && !pp.lb && !pp.mb) state = DOWN;
            break;
        case DOWN:
            printf("down\n");
            if (pp.lb || pp.mb || pp.delta_x < (-1 * abs(tolerance)) || pp.delta_y > abs(tolerance)) {
                state = INITIAL;
                break;
            }
            total_delta_x = pp.delta_x > 0 ? total_delta_x + abs(pp.delta_x) : total_delta_x;
            total_delta_y = pp.delta_y < 0 ? total_delta_y + abs(pp.delta_y) : total_delta_y;
            if (!pp.rb) {
                printf("total_delta_y :%d\n", total_delta_y);
                printf("total_delta_x :%d\n", total_delta_x);

                if (total_delta_x < x_len || (total_delta_y / total_delta_x) <= 1) {
                    state = INITIAL;
                } else
                    state = COMPLETE;
            }
            break;
        default:
            break;
    }
}

uint8_t read_mouse_byte() {
    uint8_t byte = 0;
    write_mouse_command(READ_DATA);
    util_sys_inb(OUTPUT_BUFFER, &byte);
    return byte;
}

void mouse_ph() {
    pp.bytes[0] = 0;
    while (!(pp.bytes[0] & BIT(3))) {
        pp.bytes[0] = read_mouse_byte();
    }
    pp.bytes[1] = read_mouse_byte();
    pp.bytes[2] = read_mouse_byte();
    pp.lb = pp.bytes[0] & BIT(0);
    pp.rb = pp.bytes[0] & BIT(1);
    pp.mb = pp.bytes[0] & BIT(2);
    pp.x_ov = pp.bytes[0] & BIT(6);
    pp.y_ov = pp.bytes[0] & BIT(7);
    pp.delta_x = (pp.bytes[0] & BIT(4)) ? (0xFF00 | pp.bytes[1]) : pp.bytes[1];
    pp.delta_y = (pp.bytes[0] & BIT(5)) ? (0xFF00 | pp.bytes[2]) : pp.bytes[2];
}
