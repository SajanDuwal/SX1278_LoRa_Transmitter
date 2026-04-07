/** Example packet format: Total 1-255 bytes
    [Preamble] -> {0XAA, 0xAA, 0xAA, 0xAA} (4 bytes)
    [PAYLOAD_LEN] -> {0xAB} (1 byte) — payload total length // variable in size
    [SYNC] -> 0xAB (1 byte) — indicates start of payload 
    [STATION_ID] -> 0x01 (1 byte)
    [SEQUENCE_NUM] -> 0x01 (1 byte) - Packet sequence number, increments with each packet
    [TimeStamp] -> 0x5F3759DF (4 bytes) - Unix timestamp of when packet was sent
    [INFORMATION_LEN] -> 0x05 (1 byte) — length of information field
    [INFORMATION] -> {0x...} (N bytes)
    [INFORMATION_CRC] -> {0x01, 0x02} (2 bytes) — simple XOR of payload bytes expect preamble
**/

#include "custom_LoRa_Packet.h"

#define SYNC_BYTE 0xAB

// User-defined CRC
uint16_t calc_CRC(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ byte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8408;
            }
            byte >>= 1;
        }
    }
    return ~crc;   // final XOR
}

uint8_t lora_buildPacket(uint8_t *final_packet_buff, uint64_t timStamp, uint8_t STATION_ID, uint8_t message_counter, uint8_t *message, uint8_t message_len)
{
    final_packet_buff[0] = 0xAA; //Preamble
    final_packet_buff[1] = 0xAA; //Preamble
    final_packet_buff[2] = 0xAA; //Preamble
    final_packet_buff[3] = 0xAA; //Preamble

    final_packet_buff[5] = SYNC_BYTE; // Sync byte
    final_packet_buff[6] = STATION_ID; // Station ID
    final_packet_buff[7] = message_counter; // Sequence number
    final_packet_buff[8] = (timStamp >> 24) & 0xFF; // Timestamp byte 1
    final_packet_buff[9] = (timStamp >> 16) & 0xFF;
    final_packet_buff[10] = (timStamp >> 8) & 0xFF;
    final_packet_buff[11] = timStamp & 0xFF; // Timestamp byte
    final_packet_buff[12] = message_len; // Information length
    for(int i = 0; i < message_len; i++) {
        final_packet_buff[13 + i] = message[i]; // Information
    }

    uint8_t total_packet_len = 4 + 1 + 1 + 1 + 1 + 4 + 1 + message_len + 2; // Preamble + LEN + Sync + Station ID + Seq Num + Timestamp + Info Len + Info + CRC

    final_packet_buff[4] = total_packet_len; // Payload length

    uint16_t crc = calc_CRC(&final_packet_buff[4], 1 + 1 + 1 + 4 + 1 + message_len);
    // BIG ENDIAN
    final_packet_buff[13 + message_len] = (crc >> 8) & 0xFF;
    final_packet_buff[14 + message_len] = crc & 0xFF;

    printf("📡 Packet going AIR\n");
    printf("Total Packet Length: %d bytes\n", final_packet_buff[4]);
    printf("Station ID: %d\n", final_packet_buff[6]);
    printf("Seq: %d\n", final_packet_buff[7]);
    printf("Timestamp: %lu\n", final_packet_buff[8] << 24 | final_packet_buff[9] << 16 | final_packet_buff[10] << 8 | final_packet_buff[11]);
    printf("Payload (%d bytes): ", final_packet_buff[12]);

    for (uint8_t i = 0; i < final_packet_buff[12]; i++) {
        printf("0x%02X ", final_packet_buff[13 + i]);
    }
    printf("\n");

    return total_packet_len;  // total packet size
}

void lora_parsePacket(uint8_t *received_buff, uint8_t received_buff_len, lora_packet_t *parsed_packet){

    if(received_buff[0] != 0xAA || received_buff[1] != 0xAA || received_buff[2] != 0xAA || received_buff[3] != 0xAA) {
        printf("❌ Invalid packet: Preamble mismatch\n");
        return;
    }
    printf("✅ Preamble OKEY!\n");
    if(received_buff[5] != SYNC_BYTE) {
        printf("❌ Invalid packet: Sync byte mismatch\n");
        return;
    }
    printf("✅ Sync Byte OKEY!\n");

    parsed_packet->packet_len = received_buff[4]; // total packet length

    parsed_packet->payload_len = received_buff[12]; // Information length   

    // Verify CRC
    uint16_t received_crc = calc_CRC(&received_buff[4], 1 + 1 + 1 + 4 + 1 + parsed_packet->payload_len);
    uint8_t calc_crc_MSB = (received_crc >> 8) & 0xFF;
    uint8_t calc_crc_LSB = received_crc & 0xFF;

    uint8_t rx_crc_MSB = received_buff[parsed_packet->packet_len - 2];
    uint8_t rx_crc_LSB = received_buff[parsed_packet->packet_len - 1];

    if( calc_crc_LSB != rx_crc_LSB || calc_crc_MSB != rx_crc_MSB) {
        printf("❌ Invalid packet: CRC mismatch\n");
        printf("Received CRC: 0x%04X, Calculated CRC: 0x%04X\n", (rx_crc_MSB << 8 | rx_crc_LSB), received_crc);
        return;
    }

    printf("📡 Packet OK\n");
    parsed_packet->station_id = received_buff[6]; // Station ID
    parsed_packet->sequence = received_buff[7]; // Sequence number
    parsed_packet->timestamp = ((uint32_t)received_buff[8] << 24) | ((uint32_t)received_buff[9] << 16) | ((uint32_t)received_buff[10] << 8) | received_buff[11]; // Timestamp
    for(int i = 0; i < parsed_packet->payload_len; i++) {
        parsed_packet->payload[i] = received_buff[13 + i]; // Information
    }

    printf("Total Packet Length: %d bytes\n", parsed_packet->packet_len);
    printf("Station ID: %d\n", parsed_packet->station_id);
    printf("Seq: %d\n", parsed_packet->sequence);
    printf("Timestamp: %lu\n", parsed_packet->timestamp);
    printf("Payload (%d bytes): ", parsed_packet->payload_len);

    for (uint8_t i = 0; i < parsed_packet->payload_len; i++) {
        printf("0x%02X ", parsed_packet->payload[i]);
    }
    printf("\n\n");
}