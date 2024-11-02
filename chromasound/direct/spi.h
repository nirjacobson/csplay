#ifndef SPI_H
#define SPI_H

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <cerrno>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <cstdint>
#include <cstdlib>
#include <cstdio>

#define SPI_DEV_NAME "/dev/spidev0.0"
#define SPI_SPEED 4000000;

int spi_init();
void spi_close(int fd);

void spi_xfer(int fd, uint8_t* tx, uint8_t* rx);
void spi_write(int fd, uint8_t val);

#endif // SPI_H
