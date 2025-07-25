#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include <stdint.h>

// SPI transmit one byte and receive one byte
uint8_t spi_transfer(uint8_t byte);

// GPIO control for NSS/CS
void GPIO_WritePin(uint8_t pin, uint8_t value);

#endif // SPI_DRIVER_H
