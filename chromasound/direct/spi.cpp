#include "spi.h"

int spi_init() {
    uint32_t speed = SPI_SPEED;
    int ret;

    int fd = open(SPI_DEV_NAME, O_RDWR);
    if (fd < 0) {
        printf("Unable to open %s: %s", SPI_DEV_NAME, strerror(errno));
        return -1;
    }

    uint32_t mode = 0;
    uint32_t request = 0;
    ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
    if (ret == -1) {
        printf("Unable to set SPI mode: %s", strerror(errno));
        return -1;
    }

    ret = ioctl(fd, SPI_IOC_RD_MODE32, &mode);
    if (ret == -1) {
        printf("Unable to read SPI mode: %s", strerror(errno));
        return -1;
    }

    if (mode != request) {
        printf("SPI mode 0x%x unsupported, using mode 0x%x instead: %s", request, mode, strerror(errno));
    }

    uint8_t bits = 8;
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        printf("Unable to set bits per word: %s", strerror(errno));
        return -1;
    }

    request = speed;
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1) {
        printf("Unable to set max SPI speed: %s", strerror(errno));
        return -1;
    }
    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1) {
        printf("Unable to read max SPI speed: %s", strerror(errno));
        return -1;
    }

    if (speed != request) {
        printf("SPI speed %d unsupported, using mode %d instead: %s", request, speed, strerror(errno));
    }

    return fd;
}

void spi_close(int fd) {
    close(fd);
}

void spi_xfer(int fd, uint8_t* tx, uint8_t* rx) {
    int ret;

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 1,
    };

    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret == -1) {
        printf("Unable to make SPI transfer: %s", strerror(errno));
        return;
    }
}

void spi_write(int fd, uint8_t val) {
    uint8_t tx, rx;
    tx = val;
    spi_xfer(fd, &tx, &rx);
}
