#include <lcom/lab3.h>
#include <lcom/lcf.h>
#include <lcom/timer.h>
#include <stdbool.h>
#include <stdint.h>

#include "i8042.h"
#include "keyboard.h"
#include "timer.c"

extern bool available_to_show;
extern uint8_t complete_scancode[2];
extern uint8_t scancode_size;
extern bool make_code;
extern uint8_t scancode;
extern int timer_counter;
int sys_inb_counter = 0;

int main(int argc, char* argv[]) {
    // sets the language of LCF messages (can be either EN-US or PT-PT)
    lcf_set_language("EN-US");

    // enables to log function invocations that are being "wrapped" by LCF
    // [comment this out if you don't want/need it]
    lcf_trace_calls("/home/lcom/labs/lab3_test/trace.txt");

    // enables to save the output of printf function calls on a file
    // [comment this out if you don't want/need it]
    lcf_log_output("/home/lcom/labs/lab3_test/output.txt");

    // handles control over to LCF
    // [LCF handles command line arguments and invokes the right function]
    if (lcf_start(argc, argv))
        return 1;

    // LCF clean up tasks
    // [must be the last statement before return]
    lcf_cleanup();

    return 0;
}

int(kbd_test_scan)() {
    int ipc_status, r;
    message msg;
    uint8_t kbd_bit_no = 0;
    if (kbd_subscribe_int(&kbd_bit_no)) return 1;
    while (scancode != ESC_BREAK_CODE) {
        if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
            printf("driver_receive failed with: %d", r);
            continue;
        }
        if (is_ipc_notify(ipc_status)) {
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE:
                    if (msg.m_notify.interrupts & BIT(kbd_bit_no)) {
                        kbc_ih();
                        if (available_to_show) kbd_print_scancode(make_code, scancode_size, complete_scancode);
                    }
                    break;
                default:
                    break;
            }
        } else {
        }
    }
    kbd_print_no_sysinb(sys_inb_counter);
    if (kbd_unsubscribe_int()) return 1;
    return 0;
}

int(kbd_test_poll)() {
    //reading scancodes with polling
    while (scancode != ESC_BREAK_CODE) {
        kbc_ih();
        if (available_to_show) kbd_print_scancode(make_code, scancode_size, complete_scancode);
        tickdelay(micros_to_ticks(200000));
    }
    //enabling keyboard interrupts
    while (enable_kbd_interrupts()) {
    }

    kbd_print_no_sysinb(sys_inb_counter);
    return 0;
}

int(kbd_test_timed_scan)(uint8_t n) {
    int ipc_status, r;
    message msg;

    uint8_t timer_bit_no = 0;
    uint8_t kbd_bit_no = 0;

    if (kbd_subscribe_int(&kbd_bit_no)) return 1;
    if (timer_subscribe_int(&timer_bit_no)) return 1;

    while (timer_counter < n * 60 && scancode != ESC_BREAK_CODE) {
        if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
            printf("driver_receive failed with: %d", r);
            continue;
        }
        if (is_ipc_notify(ipc_status)) {
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE:
                    if (msg.m_notify.interrupts & BIT(timer_bit_no)) {
                        timer_int_handler();
                    }
                    if (msg.m_notify.interrupts & BIT(kbd_bit_no)) {
                        kbc_ih();
                        if (available_to_show) kbd_print_scancode(make_code, scancode_size, complete_scancode);
                        timer_counter = 0;
                    }
                    break;
                default:
                    break;
            }
        } else {
        }
    }
    if (kbd_unsubscribe_int()) return 1;
    if (timer_unsubscribe_int()) return 1;
    return 0;
}
