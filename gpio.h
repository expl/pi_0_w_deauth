#ifndef GPIO_H
#define	GPIO_H

int init_gpio(void);
void setup_gpio(int gpio, int direction, int pud);
void output_gpio(int gpio, int value);
int input_gpio(int gpio);
void gpio_cleanup(void);
int gpio_function(int gpio);
void set_pullupdn(int gpio, int pud);

#define PUD_OFF  0
#define PUD_DOWN 1
#define PUD_UP   2

#endif	/* GPIO_H */

