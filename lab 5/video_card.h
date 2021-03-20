#ifndef VIDEO_CARD_H
#define VIDEO_CARD_H

#include <lcom/lcf.h>

typedef struct Sprite {
    uint8_t* pixmap;
    uint16_t x;
    uint16_t y;
    int16_t speed;
    uint16_t width;
    uint16_t height;
    unsigned int type;
} Sprite;

void get_mode_info(uint16_t mode);

int set_mode(uint16_t mode);

int(vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color);

int(vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);

uint32_t mask_from_range(int begin, int end);

int get_color_parameters(uint32_t color, uint8_t col, uint8_t row, uint8_t step);

int draw_pattern(uint8_t no_rectangles, uint32_t first, uint8_t step);

uint32_t get_color(uint32_t color);

void draw_xpm(Sprite* sprite);

Sprite* create_sprite(xpm_map_t xpm, uint16_t speed, uint16_t x, uint16_t y);

void move_xpm(Sprite* sprite, uint16_t xf, uint16_t yf);

void clean_sprite(Sprite* sprite);

#endif
