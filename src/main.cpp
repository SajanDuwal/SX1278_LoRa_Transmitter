#include <Arduino.h>
#include <Wire.h>

#include "custom_SX1278_LoRa.h"
#include "custom_LoRa_Packet.h"

// ---------------- SX1278 PIN CONFIG ----------------
#define PIN_SPI_CS      GPIO_NUM_10
#define PIN_SPI_MOSI    GPIO_NUM_11
#define PIN_SPI_SCK     GPIO_NUM_12
#define PIN_SPI_MISO    GPIO_NUM_13
#define PIN_RESET       GPIO_NUM_9
#define PIN_DIO0        GPIO_NUM_8

// ---------------- LORA CONFIG ----------------
#define LORA_FREQUENCY_HZ       433000000UL
#define LORA_MODEM_CONFIG1      BW_125KHZ | CR_4_8 | HEADER_EXPLICIT              ///< BW=125kHz, CR=4/5, Explicit header
#define LORA_MODEM_CONFIG2      SF_12 | TX_MODE_NORMAL | RX_PAYLOAD_CRC_ON       ///< SF=12, CRC ON
#define LORA_MODEM_CONFIG3      LOW_DATA_RATE_OPTIMIZE_ON  | AGC_AUTO_ON         ///< AGC ON, LDRO ON for SF12 @ BW125kHz
#define LORA_PA_CONFIG          PA_BOOST_20DBM                                  ///< +20 dBm via PA_BOOST
#define LORA_SYNC_WORD          LORA_SYNC_WORD_PRIVATE                          ///< 0x12

#define MAX_PACKETS     10
#define RX_TIMEOUT_MS   15000

lora_config_t lora_cfg = {
    .pin_cs = PIN_SPI_CS,
    .pin_mosi = PIN_SPI_MOSI,
    .pin_miso = PIN_SPI_MISO,
    .pin_sck = PIN_SPI_SCK,
    .pin_reset = PIN_RESET,
    .pin_dio0 = PIN_DIO0,

    .frequency = LORA_FREQUENCY_HZ,
    .modem_config1 = LORA_MODEM_CONFIG1,
    .modem_config2 = LORA_MODEM_CONFIG2,
    .modem_config3 = LORA_MODEM_CONFIG3,
    .pa_config = LORA_PA_CONFIG,
    .sync_word = LORA_SYNC_WORD
};

lora_packet_t lora_packet;

// ---------------- GLOBALS ----------------
bool SX1278_LoRa_Flag = false;

uint8_t lora_tx_buffer[256] = {0};
uint8_t lora_tx_buffer_len = 0;

uint8_t lora_rx_buffer[256] = {0};
uint8_t lora_rx_len = 0;

uint8_t pkt_counter = 0;
uint64_t unix_timestamp = 1775481385;

uint8_t lora_message[256] = {0};
uint8_t lora_message_len = 0;

unsigned long rxStartTime = 0;

// ---------------- STATE MACHINE ----------------
enum LoRaState {
  STATE_TX,
  STATE_RX
};
LoRaState currentState = STATE_TX;
LoRaState prevState = STATE_TX;

// ---------------- BUILD MESSAGE ----------------
void build_lora_message() {
  lora_message[0] = 0x01;
  lora_message[1] = 0x02;
  lora_message[2] = 0x03;
  lora_message[3] = 0x04;
  lora_message[4] = 0x05;
  lora_message[5] = 0x06;
  lora_message[6] = 0x07;
  lora_message[7] = 0x08;
  lora_message[8] = 0x09;
  lora_message[9] = 0x0A;

  lora_message_len = 10;
}

void setup() {
  // put your setup code here, to run once:
  delay(1000); // Wait for serial to initialize
  printf("***** Starting SX1278, LoRa *****\r\n");
  lora_setConfig(&lora_cfg);
  if (!lora_begin()) {
    printf("❌ LoRa init failed!\r\n");
    SX1278_LoRa_Flag = false;
  }else{
    printf("✅ LoRa init succeeded!\r\n");
    SX1278_LoRa_Flag = true;

    if (currentState == STATE_RX) {
      lora_startReceive();
      rxStartTime = millis();
    }
  }
}
 
void loop() {
  // put your main code here, to run repeatedly:
  if(!SX1278_LoRa_Flag){
    printf("LoRa not initialized, Restarting\r\n");
    esp_restart(); // Restart the ESP32 to attempt re-initialization
  }

  if (currentState != prevState) {
    printf("Current State: %s\n", currentState == STATE_TX ? "TX" : "RX");
    prevState = currentState;
  }

  if(currentState == STATE_TX) {

    if(pkt_counter == 0){
      printf("Waiting peer to be ready for RX...\r\n");
      delay(5000);
    } 

    if(pkt_counter < MAX_PACKETS) {
      printf("📤 Start lora packet [TX %d/%d] Transmitting...\r\n", pkt_counter + 1, MAX_PACKETS);
       //LoRa packet builder
      build_lora_message();
      memset(lora_tx_buffer, '\0' , sizeof(lora_tx_buffer));

      lora_tx_buffer_len = lora_buildPacket(
        lora_tx_buffer, 
        unix_timestamp, 
        0x01, 
        pkt_counter + 1,
        lora_message, 
        lora_message_len);

      // Calculate and print Time on Air
      uint32_t toa_ms = calculateLoRaToA(lora_tx_buffer_len);
      printf("Packet size : %d bytes | Estimated ToA : %lu ms (%.2f sec)\r\n", lora_tx_buffer_len, toa_ms, toa_ms / 1000.0f);
    
      //LoRa packet sender
      lora_send(lora_tx_buffer, lora_tx_buffer_len);
      printf("\r\n");
      memset(lora_tx_buffer, '\0' , sizeof(lora_tx_buffer));

      pkt_counter++;
      delay(toa_ms + 1000);    // wait ToA + margin before next TX
    }

    if(pkt_counter >= MAX_PACKETS) {
      printf("🔄 TX phase done (%d packets) → switching to RX\r\n", MAX_PACKETS);
      memset(lora_rx_buffer, '\0' , sizeof(lora_rx_buffer));
      pkt_counter   = 0;      // ✅ reset for RX phase counting
      currentState  = STATE_RX;
      delay(50);
      lora_startReceive();
      rxStartTime = millis();
    }
  } else if(currentState == STATE_RX) {
    if (lora_available()) {
      printf("Starting continuous RX mode...\n");
      memset(lora_rx_buffer, '\0' , sizeof(lora_rx_buffer));
      lora_rx_len = lora_receive(lora_rx_buffer, 255);

      if (lora_rx_len > 0) {
        printf("📥 [RX %d/%d] %d bytes: ", pkt_counter + 1, MAX_PACKETS, lora_rx_len);
        for(uint8_t i = 0; i < lora_rx_len; i++) {
          printf("0x%02X ", lora_rx_buffer[i]);
        }
        printf("\n");
        lora_parsePacket(lora_rx_buffer, lora_rx_len, &lora_packet);
        memset(lora_rx_buffer, '\0' , sizeof(lora_rx_buffer));
      
        pkt_counter++;      // ✅ count received packets
        rxStartTime = millis(); // reset timeout per packet
      }
    }

    // RX timeout → skip remaining, go back to TX
    if (millis() - rxStartTime > RX_TIMEOUT_MS) {
      printf("⏱ RX timeout (got %d/%d) → switching to TX\r\n", pkt_counter, MAX_PACKETS);
      memset(lora_tx_buffer, '\0' , sizeof(lora_tx_buffer));
      pkt_counter  = 0;       // ✅ reset for next TX phase
      currentState = STATE_TX;
      delay(50);
      return;
    }

    // All 10 RX done → switch back to TX
    if (pkt_counter >= MAX_PACKETS) {
      printf("🔄 RX phase done (%d packets) → switching to TX\r\n", MAX_PACKETS);
      memset(lora_tx_buffer, '\0' , sizeof(lora_tx_buffer));
      pkt_counter  = 0;       // ✅ reset for next TX phase
      currentState = STATE_TX;
      delay(50);
    }
  }
}