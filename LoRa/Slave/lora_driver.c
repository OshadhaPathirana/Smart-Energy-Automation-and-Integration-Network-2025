#include "spi_driver.h"   // Your SPI functions
#include <stdint.h>

#define LORA_NSS_PIN   GPIO_PIN_X   // Replace with your NSS pin definition
#define LORA_SYNC_WORD 0x02         // This inverter's unique sync word
#define REG_SYNC_WORD  0x39         // SX1278 register for sync word

// Helper: Write 1 byte to LoRa register
void lora_write_reg(uint8_t addr, uint8_t value) {
    GPIO_WritePin(LORA_NSS_PIN, 0);             // NSS low
    spi_transfer(addr | 0x80);                  // MSB=1 → write mode
    spi_transfer(value);
    GPIO_WritePin(LORA_NSS_PIN, 1);             // NSS high
}

// Helper: Read 1 byte from LoRa register
uint8_t lora_read_reg(uint8_t addr) {
    GPIO_WritePin(LORA_NSS_PIN, 0);             // NSS low
    spi_transfer(addr & 0x7F);                  // MSB=0 → read mode
    uint8_t val = spi_transfer(0x00);
    GPIO_WritePin(LORA_NSS_PIN, 1);             // NSS high
    return val;
}

// LoRa init sequence (only key parts shown)
void lora_init(void) {
    // Set LoRa mode (RegOpMode = 0x01)
    lora_write_reg(0x01, 0x80);  // LoRa + standby mode

    // Set frequency, power, etc. (not shown here)

    // Set Sync Word
    lora_write_reg(REG_SYNC_WORD, LORA_SYNC_WORD);

    // Optional: verify
    uint8_t verify = lora_read_reg(REG_SYNC_WORD);
    if (verify == LORA_SYNC_WORD) {
        // Sync word set successfully
    }

    // Put LoRa into continuous receive mode (RX)
    lora_write_reg(0x01, 0x85);  // LoRa + RX continuous mode
}
