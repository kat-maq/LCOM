#include <lcom/lab5.h>
#include <lcom/lcf.h>
#include <lcom/pixmap.h>
#include <lcom/timer.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "i8042.h"
#include "keyboard.h"
#include "video_card.h"

extern uint8_t scancode;
extern unsigned timer_counter;

int main(int argc, char *argv[]) {
    // sets the language of LCF messages (can be either EN-US or PT-PT)
    lcf_set_language("EN-US");

    // enables to log function invocations that are being "wrapped" by LCF
    // [comment this out if you don't want/need it]
    lcf_trace_calls("/home/lcom/labs/lab5/trace.txt");

    // enables to save the output of printf function calls on a file
    // [comment this out if you don't want/need it]
    lcf_log_output("/home/lcom/labs/lab5/output.txt");

    // handles control over to LCF
    // [LCF handles command line arguments and invokes the right function]
    if (lcf_start(argc, argv))
        return 1;

    // LCF clean up tasks
    // [must be the last statement before return]
    lcf_cleanup();

    return 0;
}

int(video_test_init)(uint16_t mode, uint8_t delay) {
    get_mode_info(mode);
    set_mode(mode);
    sleep(delay);
    vg_exit();
    return 0;
}

int(video_test_rectangle)(uint16_t mode, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
    get_mode_info(mode);
    set_mode(mode);
    vg_draw_rectangle(x, y, width, height, color);
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
                    }
                    break;
                default:
                    break;
            }
        } else {
        }
    }
    if (kbd_unsubscribe_int()) return 1;
    vg_exit();
    return 0;
}

int(video_test_pattern)(uint16_t mode, uint8_t no_rectangles, uint32_t first, uint8_t step) {
    get_mode_info(mode);
    set_mode(mode);
    draw_pattern(no_rectangles, first, step);
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
                    }
                    break;
                default:
                    break;
            }
        } else {
        }
    }
    if (kbd_unsubscribe_int()) return 1;
    vg_exit();
    return 0;
}

int(video_test_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y) {
    get_mode_info(0x105);
    set_mode(0x105);
    Sprite *sprite = create_sprite(xpm, 0, x, y);
    draw_xpm(sprite);
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
                    }
                    break;
                default:
                    break;
            }
        } else {
        }
    }
    if (kbd_unsubscribe_int()) return 1;
    vg_exit();
    return 0;
}

int(video_test_move)(xpm_map_t xpm, uint16_t xi, uint16_t yi, uint16_t xf, uint16_t yf,
                     int16_t speed, uint8_t fr_rate) {
    get_mode_info(0x105);
    set_mode(0x105);
    int ipc_status, r;
    message msg;
    uint8_t kbd_bit_no = 0;
    uint8_t timer_bit_no = 0;
    uint8_t time_per_frame = sys_hz() / fr_rate;
    Sprite *sprite = create_sprite(xpm, speed, xi, yi);
    int elapsed_frames = 0;
    draw_xpm(sprite);
    if (kbd_subscribe_int(&kbd_bit_no)) return 1;
    if (timer_subscribe_int(&timer_bit_no)) return 1;
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
                    }
                    if (msg.m_notify.interrupts & BIT(timer_bit_no)) {
                        timer_int_handler();
                        draw_xpm(sprite);
                        if (timer_counter % time_per_frame == 0) {
                            elapsed_frames++;
                            if (speed > 0) {
                                clean_sprite(sprite);
                                move_xpm(sprite, xf, yf);
                            } else if (elapsed_frames == abs(speed)) {
                                elapsed_frames = 0;
                                clean_sprite(sprite);
                                move_xpm(sprite, xf, yf);
                            }
                        }
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
    vg_exit();
    return 0;
}
