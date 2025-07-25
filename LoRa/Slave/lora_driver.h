#ifndef LORA_DRIVER_H
#define LORA_DRIVER_H

#include <stdint.h>

void lora_init(void);
void lora_write_reg(uint8_t addr, uint8_t value);
uint8_t lora_read_reg(uint8_t addr);

#endif // LORA_DRIVER_H
