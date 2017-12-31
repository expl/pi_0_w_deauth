/*
 * Adafruit 128X64 Monochrome OLED display driver and basic drawing routines
 * by Kostas Petrikas
 */

#include "display.h"
#include "gpio.h"
#include <math.h>

#define _D 1.0

using namespace std;

// Set of serial commands to initialize OLED display
static const uint8_t init_payload[] = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00,
    0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8,
    0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB,
    0x40, 0xA4, 0xA6, 0xAF
};

// Set of serial commands to update display
static const uint8_t update_payload[] = {
    0x21, 0x00, 0x7F, 0x22, 0x00, 0x07
};

Display::Display() {
    if ((this->display_f = open(I2C_DEV, O_RDWR)) < 0) {
        cout << "Failed opening i2c bus!\n";
        
        exit(1);
    }
    
    if (ioctl(this->display_f, I2C_SLAVE, I2C_ADDR) < 0) {
        cout << "Failed to acquire i2c bus access and/or talk to slave!\n";
            
        exit(1);
    }
    
    this->send_bus_commands(
        0x00, (uint8_t *)init_payload,
        sizeof(init_payload)
    );
    
    this->screen = new uint8_t[this->screen_len];
    memset(this->screen, 0, this->screen_len);
    
    this->cursor = new TextCursor(this);
}

Display::~Display() {
    uint8_t off_cmd = 0xAE;
    this->send_bus_commands(0x00, &off_cmd, 1);
    
    close(this->display_f);
    
    delete this->screen;
    delete this->cursor;
}

void Display::send_bus_commands(uint8_t reg, uint8_t* cmds, int len) {
    uint8_t b[2];
    
    for (int i = 0; i != len; i++) {
        b[0] = reg;
        b[1] = cmds[i];
        
        write(this->display_f, b, 2);
    }
}

void Display::send_bus_buffer(uint8_t reg, uint8_t* buff, int len) {
    unsigned char b[17];
    
    for (int p = 0; p != len;) {
        b[0] = reg;
        memcpy(&(b[1]), &(buff[p]), 16);
        write(this->display_f, b, 17);
        
        p += 16;
    }
}

void Display::update() {
    this->send_bus_commands(
        0x00, (uint8_t *)update_payload,
        sizeof(update_payload)
    );

    this->send_bus_buffer(0x40, this->screen, this->screen_len);
}

bool Display::get_pixel(int x, int y) {
    int row = y / 8;
    int y_off = y % 8;
    
    // Check screen limits instead of crashing
    if (x > this->width || y > this->height || y < 0 || x < 0)
        return false;
    
    uint8_t page = this->screen[(row * this->width) + x];
    
    return (bool)((page >> y_off) & 1);
}

void Display::set_pixel(int x, int y) {
    int row = y / 8;
    int y_off = y % 8;
    
    // Check screen limits instead of crashing
    if (x > this->width - 1 || y > this->height - 1 || y < 0 || x < 0)
        return;

    this->screen[(row * this->width) + x] |= (1 << y_off);
}

void Display::clear_pixel(int x, int y) {
    int row = y / 8;
    int y_off = y % 8;
    
    // Check screen limits instead of crashing
    if (x > this->width || y > this->height || y < 0 || x < 0)
        return;

    this->screen[(row * this->width) + x] &= ~(1 << y_off);
}

void Display::render_char(const char *bitmap, int sx, int sy) {
    int x, y;
    int set;
    for (x = 0; x != FONT_W; x++) {
        for (y = 0; y != FONT_H; y++) {
            set = bitmap[x] & 1 << y;
            
            if (set) {
                this->set_pixel(sx + x, sy + y);
            } else {
                this->clear_pixel(sx + x, sy + y);
            }
        }
    }
}

void Display::inverse_area(int bx, int by, int ex, int ey) {
    for (int x = bx; x != ex; x++) {
        for (int y = by; y != ey; y++) {
            if (this->get_pixel(x, y)) {
                this->clear_pixel(x, y);
            } else {
                this->set_pixel(x, y);
            }
        }
    }
}

void Display::clear_area(int bx, int by, int ex, int ey) {
    for (int y = by; y != ey; y++){
        for (int x = bx; x != ex; x++) {
            this->clear_pixel(x, y);
        }
    }
}

void Display::draw_line(int bx, int by, int ex, int ey) {
    int dx = ex - bx;
    int dy = ey - by;
    int adx = abs(dx);
    int ady = abs(dy);
    int steps = adx > ady ? adx : ady;
    float xi = (float) dx / (float) steps;
    float yi = (float) dy / (float) steps;
    
    float x = bx, y = by;
    for(int i = 0; i < steps; i++) {
        x += xi;
        y += yi;
        
        this->set_pixel((int) roundf(x), (int) roundf(y));
    }
}

void Display::draw_rect(
    int ax, int ay, int bx, int by,
    int cx, int cy, int dx, int dy
) {
    this->draw_line(ax, ay, bx, by);
    this->draw_line(bx, by, cx, cy);
    this->draw_line(cx, cy, dx, dy);
    this->draw_line(dx, dy, ax, ay);
}

void Display::clear() {
    memset(this->screen, 0, this->screen_len);
    this->cursor->clear();
}

TextCursor::TextCursor(Display* display) {
    this->display = display;
    this->x = 0;
    this->y = 0;
    
    this->chs_per_line = (display->width - this->line_l_padding) / FONT_W;
    this->row_count = display->height / (FONT_H + this->line_b_padding);
    
    size_t buff_len = this->row_count * this->chs_per_line;
    this->buffer = new uint8_t[buff_len];
    memset(this->buffer, 0, buff_len);
}

TextCursor::~TextCursor() {
    delete this->buffer;
}

void TextCursor::clear() {
    size_t buff_len = this->row_count * this->chs_per_line;
    memset(this->buffer, 0, buff_len);
    
    this->set(0, 0);
}

void TextCursor::set(int x, int y) {
    this->x = x > this->chs_per_line - 1 ? this->chs_per_line : x;
    this->y = y > this->row_count - 1 ? this->row_count : y;
}

int TextCursor::get_sx(int x) {
    return x * FONT_W + this->line_l_padding;
}

int TextCursor::get_sy(int y) {
    return y * (FONT_H + this->line_b_padding);
}

void TextCursor::print(const char *text, bool wrap) {
    int x_space;
    
    for (int i = 0; text[i] != (char)0x00; i++) {
        x_space = this->chs_per_line - this->x;
        
        if (x_space == 0) {
            if (wrap == false || this->y == this->row_count - 1) {
                return;
            } else {
                this->set(0, this->y + 1);
            }
        }
        
        this->buffer[this->y * (this->chs_per_line) + this->x] = (uint8_t)text[i];
        
        this->x++;
    }
    
    this->flush();
}

void TextCursor::flush() {
    int i = 0;
    
    for (int y = 0; y != this->row_count; y++) {
        for (int x = 0; x != this->chs_per_line; x++, i++) {
            this->display->render_char(
                &font_5x7[(this->buffer[i]) * FONT_W],
                this->get_sx(x), this->get_sy(y)
            );
        }
    }
}

void TextCursor::inverse_row(int row) {
    int bx = 0;
    int by = row * (FONT_H + this->line_b_padding);
    int ex = this->display->width;
    int ey = by + FONT_H + this->line_b_padding;
    
    this->display->inverse_area(bx, by, ex, ey);
}
