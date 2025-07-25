#include "lora_driver.h"
#include "spi_driver.h"
#include <stdint.h>
#include <string.h>

#define LORA_NSS_PIN     4
#define LORA_SYNC_WORD   0x02

#define REG_OP_MODE      0x01
#define REG_SYNC_WORD    0x39
#define REG_FIFO         0x00
#define REG_FIFO_ADDR    0x0D
#define REG_PAYLOAD_LEN  0x22
#define REG_IRQ_FLAGS    0x12

#define TX_MODE          0x83
#define RX_MODE_CONT     0x85
#define STDBY_MODE       0x81

#define MAX_RETRIES      3
#define ACK_TIMEOUT_MS   300

void lora_write_reg(uint8_t addr, uint8_t value) {
    GPIO_WritePin(LORA_NSS_PIN, 0);
    spi_transfer(addr | 0x80);
    spi_transfer(value);
    GPIO_WritePin(LORA_NSS_PIN, 1);
}

uint8_t lora_read_reg(uint8_t addr) {
    GPIO_WritePin(LORA_NSS_PIN, 0);
    spi_transfer(addr & 0x7F);
    uint8_t val = spi_transfer(0x00);
    GPIO_WritePin(LORA_NSS_PIN, 1);
    return val;
}

void lora_write_fifo(uint8_t *data, uint8_t len) {
    lora_write_reg(REG_FIFO_ADDR, 0x00);
    GPIO_WritePin(LORA_NSS_PIN, 0);
    spi_transfer(REG_FIFO | 0x80);
    for (uint8_t i = 0; i < len; i++) {
        spi_transfer(data[i]);
    }
    GPIO_WritePin(LORA_NSS_PIN, 1);
    lora_write_reg(REG_PAYLOAD_LEN, len);
}

void lora_send_packet(const char *msg) {
    lora_write_reg(REG_OP_MODE, STDBY_MODE);
    lora_write_fifo((uint8_t *)msg, strlen(msg));
    lora_write_reg(REG_OP_MODE, TX_MODE);

    // Wait for TX done (poll IRQ flags)
    while (!(lora_read_reg(REG_IRQ_FLAGS) & 0x08));
    lora_write_reg(REG_IRQ_FLAGS, 0x08);  // Clear TX done
    lora_write_reg(REG_OP_MODE, RX_MODE_CONT);
}

uint8_t lora_wait_for_ack(const char *expected_id, uint32_t timeout_ms) {
    uint32_t start = get_millis();  // You must implement this based on timer
    while ((get_millis() - start) < timeout_ms) {
        if (lora_read_reg(REG_IRQ_FLAGS) & 0x40) {
            lora_write_reg(REG_IRQ_FLAGS, 0x40); // Clear RX done
            uint8_t len = lora_read_reg(REG_PAYLOAD_LEN);
            lora_write_reg(REG_FIFO_ADDR, 0x00);

            char buf[64] = {0};
            GPIO_WritePin(LORA_NSS_PIN, 0);
            spi_transfer(REG_FIFO & 0x7F);
            for (int i = 0; i < len && i < 63; i++) {
                buf[i] = spi_transfer(0x00);
            }
            GPIO_WritePin(LORA_NSS_PIN, 1);

            if (strncmp(buf, "@ACK:", 5) == 0 && strstr(buf, expected_id)) {
                return 1;  // ACK received
            }
        }
    }
    return 0;  // Timeout
}

uint8_t lora_send_with_ack(const char *msg, const char *inv_id) {
    for (int i = 0; i < MAX_RETRIES; i++) {
        lora_send_packet(msg);
        if (lora_wait_for_ack(inv_id, ACK_TIMEOUT_MS)) {
            return 1;  // Success
        }
    }
    return 0;  // Failure after retries
}

void lora_init(void) {
    lora_write_reg(REG_OP_MODE, 0x80);
    lora_write_reg(REG_SYNC_WORD, LORA_SYNC_WORD);

    uint8_t verify = lora_read_reg(REG_SYNC_WORD);
    if (verify != LORA_SYNC_WORD) {
        // handle error
    }

    lora_write_reg(REG_OP_MODE, RX_MODE_CONT);
}
