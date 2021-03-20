#include "video_card.h"

#include <lcom/lcf.h>

static vbe_mode_info_t mode_info;
static char* video_mem;
static uint16_t x_res = 0, y_res = 0, bytes_per_pixel;

void get_mode_info(uint16_t mode) {
    vbe_get_mode_info(mode, &mode_info);

    x_res = mode_info.XResolution;
    y_res = mode_info.YResolution;
    bytes_per_pixel = (mode_info.BitsPerPixel + 7) / 8;

    unsigned int vram_base = mode_info.PhysBasePtr;           /* VRAM's physical addresss */
    unsigned int vram_size = x_res * y_res * bytes_per_pixel; /* VRAM's size */

    struct minix_mem_range mr;
    mr.mr_base = (phys_bytes)vram_base;
    mr.mr_limit = mr.mr_base + vram_size;

    int r;

    if (OK != (r = sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr)))
        panic("sys_privctl (ADD_MEM) failed: %d\n", r);

    /* Map memory */

    video_mem = vm_map_phys(SELF, (void*)mr.mr_base, vram_size);

    if (video_mem == MAP_FAILED)
        panic("couldn't map video memory");
}

int set_mode(uint16_t mode) {
    reg86_t reg;
    memset(&reg, 0, sizeof(reg)); /* zero the structure */
    reg.intno = 0x10;
    reg.ah = 0x4F;
    reg.al = 0x02;            // VBE call, function 02 -- set VBE mode
    reg.bx = BIT(14) | mode;  // set bit 14: linear framebuffer
    if (sys_int86(&reg) != OK) {
        printf("set_vbe_mode: sys_int86() failed \n");
        return 1;
    }
    return 0;
}

int draw_pixel(uint16_t x, uint16_t y, uint32_t color) {
    if (x >= x_res || y >= y_res) {
        printf("Can no design out off borders\n");
        return 1;
    }
    /*for (unsigned i = 0; i < bytes_per_pixel; i++)
      *(video_mem + (x + x_res * y) * bytes_per_pixel + i) = (uint8_t)(color >> (8 * i)); */
    memcpy(video_mem + (x + y * x_res) * bytes_per_pixel, &color, bytes_per_pixel);
    return 0;
}

int(vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
    for (uint16_t i = x; i < (x + len); i++) {
        draw_pixel(i, y, color);
    }
    return 0;
}

int(vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
    color = get_color(color);
    for (uint16_t i = y; i < (y + height); i++) {
        vg_draw_hline(x, i, width, color);
    }
    return 0;
}

uint32_t mask_from_range(int begin, int end) {
    uint32_t mask = 0;
    for (int i = begin; i < end; ++i) {
        mask |= BIT(i);
    }
    return mask;
}

uint32_t get_color(uint32_t color) {
    int r, g, b;
    uint32_t mask_b = mask_from_range(0, mode_info.BlueMaskSize);
    uint32_t mask_g = mask_from_range(0, mode_info.GreenMaskSize);
    uint32_t mask_r = mask_from_range(0, mode_info.RedMaskSize);
    b = color & 0xFF;
    b &= mask_b;
    g = (color & 0xFF00) >> 8;
    g &= mask_g;
    r = (color & 0xFF0000) >> 16;
    r &= mask_r;

    return (b | (g << mode_info.BlueMaskSize) | (r << (mode_info.GreenMaskSize + mode_info.BlueMaskSize)));
}

int get_color_parameters(uint32_t color, uint8_t col, uint8_t row, uint8_t step) {
    uint32_t r, g, b;
    b = color & 0xFF;
    g = (color & 0xFF00) >> 8;
    r = (color & 0xFF0000) >> 16;
    r = (r + col * step) % (1 << mode_info.RedMaskSize);
    g = (g + row * step) % (1 << mode_info.GreenMaskSize);
    b = (b + (col + row) * step) % (1 << mode_info.BlueMaskSize);

    uint32_t color_to_return = (((uint32_t)b) | (((uint32_t)g) << 8) | (((uint32_t)r) << 16));
    return get_color(color_to_return);
}

int draw_pattern(uint8_t no_rectangles, uint32_t first, uint8_t step) {
    uint8_t rec_width = x_res / no_rectangles;
    uint8_t rec_height = y_res / no_rectangles;
    uint32_t color = 0;

    for (uint8_t row = 0; row < no_rectangles; ++row) {
        for (uint8_t col = 0; col < no_rectangles; ++col) {
            if (mode_info.MemoryModel == 0x04) {
                color = (first + (row * no_rectangles + col) * step) % (1 << mode_info.BitsPerPixel);
            } else {
                color = get_color_parameters(first, col, row, step);
            }
            vg_draw_rectangle(rec_width * col, rec_height * row, rec_width, rec_height, color);
        }
    }
    return 0;
}

void draw_xpm(Sprite* sprite) {
    for (int i = 0; i < sprite->width; ++i) {
        for (int j = 0; j < sprite->height; ++j) {
            draw_pixel(i + sprite->x, j + sprite->y, sprite->pixmap[j * sprite->width + i]);
        }
    }
}

Sprite* create_sprite(xpm_map_t xpm, uint16_t speed, uint16_t x, uint16_t y) {
    Sprite* sprite = (Sprite*)malloc(sizeof(Sprite));
    xpm_image_t img;
    sprite->pixmap = xpm_load(xpm, XPM_INDEXED, &img);
    sprite->speed = speed;
    sprite->x = x;
    sprite->y = y;
    sprite->width = img.width;
    sprite->height = img.height;
    sprite->type = img.type;
    return sprite;
}

void move_xpm(Sprite* sprite, uint16_t xf, uint16_t yf) {
    if (sprite->speed > 0) {
        sprite->x = (sprite->x + sprite->speed) > xf ? xf : sprite->x + sprite->speed;
        sprite->y = (sprite->y + sprite->speed) > yf ? yf : sprite->y + sprite->speed;
    } else {
        sprite->x = (sprite->x + 1) > xf ? xf : sprite->x + 1;
        sprite->y = (sprite->y + 1) > yf ? yf : sprite->y + 1;
    }
    draw_xpm(sprite);
}

void clean_sprite(Sprite* sprite) {
    for (int i = 0; i < sprite->width; ++i) {
        for (int j = 0; j < sprite->height; ++j) {
            draw_pixel(i + sprite->x, j + sprite->y, xpm_transparency_color(sprite->type));
        }
    }
}
