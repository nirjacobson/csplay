#ifndef GPIO_H
#define GPIO_H

#include <linux/gpio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <cstdint>
#include <cstdlib>
#include <cstdio>

#define GPIO_DEV_NAME     "/dev/gpiochip0"
#define GPIO_DEV_NAME_PI5 "/dev/gpiochip4"

int gpio_init();
void gpio_close(int fd);

void gpio_write(int fd, int pin, int value);

#endif // GPIO_H
