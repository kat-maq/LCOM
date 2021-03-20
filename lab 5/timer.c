#include <lcom/lcf.h>
#include <lcom/timer.h>
#include <stdint.h>

#include "i8254.h"

int timer_hook_id = 0;  //used being a global variable because it is used in the subscribe and unsubscribed functions
unsigned timer_counter = 0;

int(timer_set_frequency)(uint8_t timer, uint32_t freq) {
    if (freq > TIMER_FREQ)
        return 1;

    uint8_t st = 0;
    timer_get_conf(timer, &st);

    uint8_t control_word = (st & FOUR_LSBs);
    switch (timer) {
        case 0:
            control_word = control_word | TIMER_SEL0;
            break;
        case 1:
            control_word = control_word | TIMER_SEL1;
            break;
        case 2:
            control_word = control_word | TIMER_SEL2;
            break;
        default:
            break;
    }
    control_word = control_word | TIMER_LSB_MSB;
    sys_outb(TIMER_CTRL, control_word);

    uint16_t div = TIMER_FREQ / freq;
    uint8_t div_lsb = 0, div_msb = 0;
    util_get_LSB(div, &div_lsb);
    util_get_MSB(div, &div_msb);

    sys_outb(TIMER_0 + timer, div_lsb);
    sys_outb(TIMER_0 + timer, div_msb);

    return 0;
}

int(timer_subscribe_int)(uint8_t *bit_no) {
    *bit_no = timer_hook_id;
    if (sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE, &timer_hook_id) != OK) {
        printf("Error subscribing timer interrupts.\n");
        return 1;
    }
    return 0;
}

int(timer_unsubscribe_int)() {
    if (sys_irqrmpolicy(&timer_hook_id) != OK) {
        printf("Error unsubscribing timer interrupts.\n");
        return 1;
    }
    return 0;
}

void(timer_int_handler)() {
    timer_counter++;
}

int(timer_get_conf)(uint8_t timer, uint8_t *st) {
    uint8_t read_back_command = TIMER_RB_CMD | TIMER_RB_COUNT_ | TIMER_RB_SEL(timer);
    if (sys_outb(TIMER_CTRL, read_back_command) != OK) {
        printf("Error writing read back command\n");
        return 1;
    }
    if (util_sys_inb(TIMER_0 + timer, st) != OK) {
        printf("Error reading timer configuration\n");
        return 1;
    }
    return 0;
}

enum timer_init get_init_mode(uint8_t st) {
    enum timer_init t_int = INVAL_val;
    if ((st & TIMER_LSB_MSB) == TIMER_LSB_MSB)
        t_int = MSB_after_LSB;
    else if ((st & TIMER_LSB) == TIMER_LSB)
        t_int = LSB_only;
    else if ((st & TIMER_MSB) == TIMER_MSB)
        t_int = MSB_only;
    return t_int;
}

int(timer_display_conf)(uint8_t timer, uint8_t st, enum timer_status_field field) {
    union timer_status_field_val conf;
    switch (field) {
        case tsf_all:
            conf.byte = st;
            break;
        case tsf_base:
            conf.bcd = st & TIMER_BCD;
            break;
        case tsf_initial:
            conf.in_mode = get_init_mode(st);
            break;
        case tsf_mode:
            conf.count_mode = (st & TIMER_PROGRAMED_MODE) >> 1;
            break;
        default:
            break;
    }
    timer_print_config(timer, field, conf);
    return 0;
}
