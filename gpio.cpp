#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <unistd.h>

#include "gpio.h"

#define SETUP_OK          0
#define SETUP_DEVMEM_FAIL 1
#define SETUP_MALLOC_FAIL 2
#define SETUP_MMAP_FAIL   3

#define INPUT  1 // is really 0 for control register!
#define OUTPUT 0 // is really 1 for control register!
#define ALT0   4

#define HIGH 1
#define LOW  0

#define BCM2708_PERI_BASE   0x20000000
#define GPIO_BASE           (BCM2708_PERI_BASE + 0x200000)
#define OFFSET_FSEL         0   // 0x0000
#define OFFSET_SET          7   // 0x001c / 4
#define OFFSET_CLR          10  // 0x0028 / 4
#define OFFSET_PINLEVEL     13  // 0x0034 / 4
#define OFFSET_PULLUPDN     37  // 0x0094 / 4
#define OFFSET_PULLUPDNCLK  38  // 0x0098 / 4

#define PAGE_SIZE  (4 * 1024)
#define BLOCK_SIZE (4 * 1024)

static volatile uint32_t *gpio_map;

void short_wait(void) {
    int i;
    for (i = 0; i < 150; i++) {
        asm volatile("nop");
    }
}

int init_gpio(void) {
    int mem_fd;
    uint8_t *gpio_mem;

    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0)
        return SETUP_DEVMEM_FAIL;

    if ((gpio_mem = (uint8_t *)malloc(BLOCK_SIZE + (PAGE_SIZE - 1))) == NULL)
        return SETUP_MALLOC_FAIL;

    if ((uint32_t)gpio_mem % PAGE_SIZE)
        gpio_mem += PAGE_SIZE - ((uint32_t)gpio_mem % PAGE_SIZE);

    gpio_map = (uint32_t *)mmap( (void *)gpio_mem, BLOCK_SIZE,
            PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, mem_fd, GPIO_BASE);

    if ((uint32_t)gpio_map < 0)
        return SETUP_MMAP_FAIL;

    return SETUP_OK;
}

void set_pullupdn(int gpio, int pud) {
    int clk_offset = OFFSET_PULLUPDNCLK + (gpio / 32);
    int shift = (gpio % 32);

    if (pud == PUD_DOWN)
       *(gpio_map+OFFSET_PULLUPDN) = (*(gpio_map+OFFSET_PULLUPDN) & ~3) | PUD_DOWN;
    else if (pud == PUD_UP)
       *(gpio_map+OFFSET_PULLUPDN) = (*(gpio_map+OFFSET_PULLUPDN) & ~3) | PUD_UP;
    else  // pud == PUD_OFF
       *(gpio_map+OFFSET_PULLUPDN) &= ~3;

    short_wait();
    *(gpio_map+clk_offset) = 1 << shift;
    short_wait();
    *(gpio_map+OFFSET_PULLUPDN) &= ~3;
    *(gpio_map+clk_offset) = 0;
}

void setup_gpio(int gpio, int direction, int pud) {
    int offset = OFFSET_FSEL + (gpio / 10);
    int shift = (gpio%10)*3;

    set_pullupdn(gpio, pud);
    if (direction == OUTPUT)
        *(gpio_map+offset) = (*(gpio_map+offset) & ~(7<<shift)) | (1<<shift);
    else  // direction == INPUT
        *(gpio_map+offset) = (*(gpio_map+offset) & ~(7<<shift));
}

int gpio_function(int gpio) {
    int offset = OFFSET_FSEL + (gpio / 10);
    int shift = (gpio % 10) * 3;
    int value = *(gpio_map+offset);
    value >>= shift;
    value &= 7;
    return value;
}

void output_gpio(int gpio, int value) {
    int offset;
    if (value) // value == HIGH
        offset = OFFSET_SET + (gpio / 32);
    else       // value == LOW
        offset = OFFSET_CLR + (gpio / 32);
    *(gpio_map+offset) = 1 << gpio % 32;
}


int input_gpio(int gpio) {
   int offset, value, mask;
   offset = OFFSET_PINLEVEL + (gpio / 32);
   mask = (1 << gpio % 32);
   value = *(gpio_map+offset) & mask;
   return value;
}

void gpio_cleanup(void) {
    // fixme - set all gpios back to input
    munmap((void *)gpio_map, BLOCK_SIZE);
}
