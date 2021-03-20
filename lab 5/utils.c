#include <lcom/lcf.h>
#include <stdint.h>

#ifdef LAB3
extern int sys_inb_counter;
#endif

int(util_get_LSB)(uint16_t val, uint8_t *lsb) {
    *lsb = val & 0x00FF;
    return 0;
}

int(util_get_MSB)(uint16_t val, uint8_t *msb) {
    *msb = val >> 8;
    return 0;
}

int(util_sys_inb)(int port, uint8_t *value) {
    uint32_t temp_value = (uint32_t)*value;
    if (sys_inb(port, &temp_value) != OK) {
        printf("Error in utils sys inb\n");
        return 1;
    }
    *value = (uint8_t)temp_value;
#ifdef LAB3
    sys_inb_counter++;
#endif
    return 0;
}
