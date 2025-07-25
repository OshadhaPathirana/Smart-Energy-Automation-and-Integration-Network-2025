#include "lora_driver.h"
#include "spi_driver.h"
#include <stdint.h>

// Define GPIO pin used for NSS (chip select)
#define LORA_NSS_PIN  4  // Replace with actual GPIO pin number

// LoRa register addresses
#define REG_OP_MODE     0x01
#define REG_SYNC_WORD   0x39

// Sync word specific to this inverter
#define LORA_SYNC_WORD  0x02

// Helper: Write 1 byte to LoRa register
void lora_write_reg(uint8_t addr, uint8_t value) {
    GPIO_WritePin(LORA_NSS_PIN, 0);             // NSS low
    spi_transfer(addr | 0x80);                  // Write mode
    spi_transfer(value);
    GPIO_WritePin(LORA_NSS_PIN, 1);             // NSS high
}

// Helper: Read 1 byte from LoRa register
uint8_t lora_read_reg(uint8_t addr) {
    GPIO_WritePin(LORA_NSS_PIN, 0);             // NSS low
    spi_transfer(addr & 0x7F);                  // Read mode
    uint8_t val = spi_transfer(0x00);
    GPIO_WritePin(LORA_NSS_PIN, 1);             // NSS high
    return val;
}

// LoRa initialization
void lora_init(void) {
    // Set LoRa mode and standby
    lora_write_reg(REG_OP_MODE, 0x80);  // LoRa + standby mode

    // Set Sync Word (unicast identity)
    lora_write_reg(REG_SYNC_WORD, LORA_SYNC_WORD);

    // Optional: verify the sync word
    uint8_t verify = lora_read_reg(REG_SYNC_WORD);
    if (verify != LORA_SYNC_WORD) {
        // Handle sync word mismatch error (e.g., blink LED, log error)
    }

    // Set to receive mode (RX continuous)
    lora_write_reg(REG_OP_MODE, 0x85);  // LoRa + RX continuous
}
