/* 
 * File:   display.h
 * Author: kostas
 *
 * Created on October 1, 2017, 5:37 PM
 */

#ifndef DISPLAY_H
#define	DISPLAY_H

#include "main.h"
#include "font.h"

#include <linux/i2c-dev.h>

#define I2C_DEV "/dev/i2c-1"
#define I2C_ADDR 0x3C

#define ROW_SELECTED 0x01

class Display;

class TextCursor {
private:
    Display *display;
    
    int get_sx(int);
    int get_sy(int);
    void fill_row_padding(int row);
public:
    uint8_t *buffer;
    int chs_per_line;
    int row_count;
    
    int line_l_padding = 2;
    int line_b_padding = 1;
    int x, y;
    
    TextCursor(Display *display);
    ~TextCursor();
    void set(int x, int y);
    void print(const char *text, bool wrap);
    void inverse_row(int row);
    void flush();
    void clear();
};

class Display {
private:
    int display_f;
    uint8_t *screen;
    
    void send_bus_commands(uint8_t reg, uint8_t *cmds, int len);
    void send_bus_buffer(uint8_t reg, uint8_t *buff, int len);
public:
    int width = 128;
    int height = 64;
    size_t screen_len = width * 8;
    TextCursor *cursor;
    
    Display();
    virtual ~Display();
    
    void update();
    bool get_pixel(int x, int y);
    void set_pixel(int x, int y);
    void clear_pixel(int x, int y);
    void render_char(const char *bitmap, int sx, int sy);
    void inverse_area(int bx, int by, int ex, int ey);
    void clear_area(int bx, int by, int ex, int ey);
    void draw_line(int bx, int by, int ex, int ry);
    
    void draw_rect(
        int ax, int ay, int bx, int by,
        int cx, int cy,int dx, int dy
    );
    
    void clear();
};

#endif	/* DISPLAY_H */

