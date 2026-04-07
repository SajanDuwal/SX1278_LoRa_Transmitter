#pragma once
#include <Arduino.h>
#include <stdint.h>

typedef struct {
    uint8_t station_id;
    uint8_t sequence;
    uint8_t packet_len;
    uint32_t timestamp;
    uint8_t payload[255];
    uint8_t payload_len;
} lora_packet_t;

// Build packet
uint8_t lora_buildPacket(uint8_t *final_packet_buff, uint64_t timStamp, uint8_t STATION_ID, uint8_t message_counter, uint8_t *message, uint8_t message_len);

// Parse packet
void lora_parsePacket(uint8_t *received_buff, uint8_t received_buff_len, lora_packet_t *parsed_packet);