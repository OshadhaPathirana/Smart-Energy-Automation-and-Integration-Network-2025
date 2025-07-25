#ifndef LORA_DRIVER_H
#define LORA_DRIVER_H

#include <stdint.h>

// ====== Core LoRa Setup ======
void lora_init(void);
void lora_write_reg(uint8_t addr, uint8_t value);
uint8_t lora_read_reg(uint8_t addr);

// ====== FIFO and Transmission ======
void lora_write_fifo(uint8_t *data, uint8_t len);
void lora_send_packet(const char *msg);

// ====== ACK Handling ======
// Waits for @ACK:ID message, returns 1 on success
uint8_t lora_wait_for_ack(const char *expected_id, uint32_t timeout_ms);

// Sends msg and retries until ACK received or timeout
uint8_t lora_send_with_ack(const char *msg, const char *inv_id);

// ====== Timing ======
// You must define get_millis() to support timing
uint32_t get_millis(void);

#endif // LORA_DRIVER_H
