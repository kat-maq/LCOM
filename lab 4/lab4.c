#include <lcom/lcf.h>
#include <stdint.h>
#include <stdio.h>

#include "i8042.h"
#include "kbc.h"
#include "timer.c"

extern uint8_t byte_counter;
extern struct packet pp;
extern unsigned timer_counter;
extern enum mouse_states state;

int main(int argc, char *argv[]) {
    // sets the language of LCF messages (can be either EN-US or PT-PT)
    lcf_set_language("EN-US");

    // enables to log function invocations that are being "wrapped" by LCF
    // [comment this out if you don't want/need/ it]
    lcf_trace_calls("/home/lcom/labs/lab4/trace.txt");

    // enables to save the output of printf function calls on a file
    // [comment this out if you don't want/need it]
    lcf_log_output("/home/lcom/labs/lab4/output.txt");

    // handles control over to LCF
    // [LCF handles command line arguments and invokes the right function]
    if (lcf_start(argc, argv))
        return 1;

    // LCF clean up tasks
    // [must be the last statement before return]
    lcf_cleanup();

    return 0;
}

int(mouse_test_packet)(uint32_t cnt) {
    uint8_t bit_no = 0;
    int ipc_status, r;
    uint32_t packet_counter = 0;
    message msg;
    write_mouse_command(ENABLE_DATA_REPORTING);

    if (mouse_subscribe_int(&bit_no)) return 1;

    while (packet_counter < cnt) {
        if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
            printf("driver_receive failed with: %d", r);
            continue;
        }
        if (is_ipc_notify(ipc_status)) {
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE:
                    if (msg.m_notify.interrupts & BIT(bit_no)) {
                        mouse_ih();
                        if (byte_counter == 3) {
                            byte_counter = 0;
                            parse_mouse_packets();
                            mouse_print_packet(&pp);
                            packet_counter++;
                        }
                    }
                    break;
                default:
                    break;
            }
        } else {
        }
    }
    if (mouse_unsubscribe_int()) return 1;
    write_mouse_command(DISABLE_DATA_REPORTING);
    return 0;
}

int(mouse_test_async)(uint8_t idle_time) {
    uint8_t mouse_bit_no = 0, timer_bit_no = 0;
    int ipc_status, r;
    uint32_t packet_counter = 0;
    message msg;
    write_mouse_command(ENABLE_DATA_REPORTING);

    if (mouse_subscribe_int(&mouse_bit_no)) return 1;
    if (timer_subscribe_int(&timer_bit_no)) return 1;

    while (timer_counter < idle_time * sys_hz()) {
        if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
            printf("driver_receive failed with: %d", r);
            continue;
        }
        if (is_ipc_notify(ipc_status)) {
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE:
                    if (msg.m_notify.interrupts & BIT(mouse_bit_no)) {
                        mouse_ih();
                        if (byte_counter == 3) {
                            byte_counter = 0;
                            parse_mouse_packets();
                            mouse_print_packet(&pp);
                            packet_counter++;
                            timer_counter = 0;
                        }
                    }
                    if (msg.m_notify.interrupts & BIT(timer_bit_no)) {
                        timer_int_handler();
                    }
                    break;
                default:
                    break;
            }
        } else {
        }
    }
    if (mouse_unsubscribe_int()) return 1;
    if (timer_unsubscribe_int()) return 1;
    write_mouse_command(DISABLE_DATA_REPORTING);
    return 0;
}

int(mouse_test_gesture)(uint8_t x_len, uint8_t tolerance) {
    uint8_t bit_no = 0;
    int ipc_status, r;
    message msg;
    write_mouse_command(ENABLE_DATA_REPORTING);

    if (mouse_subscribe_int(&bit_no)) return 1;

    while (state != COMPLETE) {
        if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
            printf("driver_receive failed with: %d", r);
            continue;
        }
        if (is_ipc_notify(ipc_status)) {
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE:
                    if (msg.m_notify.interrupts & BIT(bit_no)) {
                        mouse_ih();
                        if (byte_counter == 3) {
                            byte_counter = 0;
                            parse_mouse_packets();
                            mouse_gesture_algorithm(x_len, tolerance);
                            mouse_print_packet(&pp);
                        }
                    }
                    break;
                default:
                    break;
            }
        } else {
        }
    }
    if (mouse_unsubscribe_int()) return 1;
    write_mouse_command(DISABLE_DATA_REPORTING);
    return 0;
}

int(mouse_test_remote)(uint16_t period, uint8_t cnt) {
    //lcf alreadys disable mouse interrupts and sets remote mode
    uint8_t packet_counter = 0;
    while (packet_counter < cnt) {
        mouse_ph();
        packet_counter++;
        mouse_print_packet(&pp);
        tickdelay(micros_to_ticks(period * 1000));
    }
    write_mouse_command(SET_STREAM_MODE);
    write_mouse_command(DISABLE_DATA_REPORTING);
    write_kbc_commands_with_arg(WRITE_COMMAND_BYTE, minix_get_dflt_kbc_cmd_byte());
    return 0;
}
