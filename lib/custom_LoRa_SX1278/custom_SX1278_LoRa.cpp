/**
 * @file    custom_SX1278_LoRa.cpp
 * @brief   SX1278 LoRa driver — SPI, reset, init, TX, RX
 *          All hardware logic lives here. main.cpp only sets config + data.
 */

#include "custom_SX1278_LoRa.h"

static lora_config_t lora_cfg;

void lora_setConfig(lora_config_t *cfg) {
    lora_cfg = *cfg;
}

// ============================================================
//  SPI READ / WRITE
// ============================================================

void lora_writeRegister(uint8_t addr, uint8_t data) {
    digitalWrite(lora_cfg.pin_cs, LOW);
    SPI.transfer(addr | 0x80);   // MSB = 1 → write
    SPI.transfer(data);
    digitalWrite(lora_cfg.pin_cs, HIGH);
}

uint8_t lora_readRegister(uint8_t addr) {
    digitalWrite(lora_cfg.pin_cs, LOW);
    SPI.transfer(addr & 0x7F);   // MSB = 0 → read
    uint8_t val = SPI.transfer(0x00);
    digitalWrite(lora_cfg.pin_cs, HIGH);
    return val;
}

// ============================================================
//  RESET
// ============================================================

void lora_reset(void) {
    pinMode(lora_cfg.pin_reset, OUTPUT);
    digitalWrite(lora_cfg.pin_reset, LOW);
    delay(10);
    digitalWrite(lora_cfg.pin_reset, HIGH);
    delay(10);
}

// ============================================================
//  SET FREQUENCY
//  Frf = (freq_hz * 2^19) / 32000000
//  Must use uint64_t intermediate — 433MHz << 19 overflows uint32_t
// ============================================================

bool lora_setFrequency(uint32_t freq_hz) {
    uint32_t frf = ((uint64_t)freq_hz << 19) / 32000000UL;

    lora_writeRegister(REG_FRF_MSB, (frf >> 16) & 0xFF);
    lora_writeRegister(REG_FRF_MID, (frf >>  8) & 0xFF);
    lora_writeRegister(REG_FRF_LSB,  frf        & 0xFF);

    // Readback verify
    uint8_t msb = lora_readRegister(REG_FRF_MSB);
    uint8_t mid = lora_readRegister(REG_FRF_MID);
    uint8_t lsb = lora_readRegister(REG_FRF_LSB);

    if (msb != ((frf >> 16) & 0xFF) ||
        mid != ((frf >>  8) & 0xFF) ||
        lsb != ( frf        & 0xFF)) {
        printf("❌ Frequency set failed! Got: 0x%02X 0x%02X 0x%02X\n", msb, mid, lsb);
        return false;
    }

    printf("✅ Frequency: %lu Hz  (FRF: 0x%02X 0x%02X 0x%02X)\n", freq_hz, msb, mid, lsb);
    return true;
}

// ============================================================
//  INIT
// ============================================================

bool lora_begin(void) {
    // SPI setup
    pinMode(lora_cfg.pin_cs, OUTPUT);
    digitalWrite(lora_cfg.pin_cs, HIGH);
    SPI.begin(lora_cfg.pin_sck, lora_cfg.pin_miso, lora_cfg.pin_mosi, lora_cfg.pin_cs);

    // Check chip is present
    printf("-> Reading version register...\n");
    uint8_t version = lora_readRegister(SX1278_VERSION_REG);
    if (version != SX1278_VERSION_VAL) {
        printf("❌ SX1278 not detected! Got: 0x%02X\n", version);
        return false;
    }
    printf("✅ SX1278 detected (version: 0x%02X)\n", version);

    printf(" Starting SX1278 LoRa Initiallization: \n");
    // Hardware reset
    printf("Resetting SX1278...\n");
    lora_reset();
    delay(1000);

    // Step 1: FSK sleep — required state before switching to LoRa mode
    lora_writeRegister(REG_OP_MODE, MODE_SLEEP);
    delay(10);

    // Step 2: LoRa sleep — LongRangeMode bit can only be set while in SLEEP
    lora_writeRegister(REG_OP_MODE, MODE_LORA_SLEEP);
    delay(10);
    if (lora_readRegister(REG_OP_MODE) != MODE_LORA_SLEEP) {
        printf("❌ Failed to enter LoRa sleep: 0x%02X\n", lora_readRegister(REG_OP_MODE));
        return false;
    }

    // Step 3: Standby
    lora_writeRegister(REG_OP_MODE, MODE_LORA_STDBY);
    delay(10);
    if (lora_readRegister(REG_OP_MODE) != MODE_LORA_STDBY) {
        printf("❌ Failed to enter standby: 0x%02X\n", lora_readRegister(REG_OP_MODE));
        return false;
    }

    // Step 4: Frequency
    if (!lora_setFrequency(lora_cfg.frequency)) {
        return false;
    }

    // Step 5: Set Preamble Length → Recommended 12 for long range (was default 8)
    lora_writeRegister(REG_PREAMBLE_MSB, 0x00);
    lora_writeRegister(REG_PREAMBLE_LSB, 12);     // 12 symbols preamble

    // Step 6: Sync word
    lora_writeRegister(REG_SYNC_WORD,lora_cfg.sync_word);

    // Step 7: ====================== PA Configuration (+17dBm or +20dBm) ======================
    if (lora_cfg.pa_config == PA_BOOST_20DBM) {
        // === +20 dBm High Power Mode ===
        lora_writeRegister(REG_PA_CONFIG, lora_cfg.pa_config);
        lora_writeRegister(REG_PA_DAC,    PA_DAC_BOOST_20DBM); // 0x87  ← This is the key for +20dBm
        // Increase Over Current Protection (very important for +20dBm)
        lora_writeRegister(REG_OCP, OCP_150MA);                // Safe value for +20dBm
    }else {
        // === Normal Mode (+2 to +17 dBm) ===
        lora_writeRegister(REG_PA_CONFIG, lora_cfg.pa_config);
        lora_writeRegister(REG_PA_DAC,    PA_DAC_DEFAULT);     // 0x84
    
        // Default OCP for normal power
        lora_writeRegister(REG_OCP, OCP_100MA);
    }

    // Step 8: Modem config
    lora_writeRegister(REG_MODEM_CONFIG1,lora_cfg.modem_config1);
    lora_writeRegister(REG_MODEM_CONFIG2,lora_cfg.modem_config2);
    lora_writeRegister(REG_MODEM_CONFIG3,lora_cfg.modem_config3);

    // Step 9: FIFO base addresses
    lora_writeRegister(REG_FIFO_TX_BASE_ADDR, FIFO_TX_BASE_DEFAULT);
    lora_writeRegister(REG_FIFO_RX_BASE_ADDR, FIFO_RX_BASE_DEFAULT);
    lora_writeRegister(REG_FIFO_ADDR_PTR,     FIFO_TX_BASE_DEFAULT);

    // Step 10: Max payload
    lora_writeRegister(REG_MAX_PAYLOAD_LENGTH, MAX_PAYLOAD_LENGTH_DEFAULT);

    // Step 11: DIO0 Mapping → Default to RxDone
    lora_writeRegister(REG_DIO_MAPPING1, DIO0_RXDONE);

    return true;
}

// ============================================================
//  TRANSMIT  (blocking — waits for TxDone IRQ)
// ============================================================

void lora_send(uint8_t *data, uint8_t len) {
    // Must be in standby before touching FIFO
    lora_writeRegister(REG_OP_MODE, MODE_LORA_STDBY);

    // Reset FIFO pointer to TX base
    lora_writeRegister(REG_FIFO_ADDR_PTR, FIFO_TX_BASE_DEFAULT);

    // Burst-write payload into FIFO
    digitalWrite(lora_cfg.pin_cs, LOW);
    SPI.transfer(REG_FIFO | 0x80);   // FIFO register, write mode (MSB=1)
    for (uint8_t i = 0; i < len; i++) {
        SPI.transfer(data[i]);
    }
    digitalWrite(lora_cfg.pin_cs, HIGH);

    // Set payload length
    lora_writeRegister(REG_PAYLOAD_LENGTH, len);

    // Clear all IRQ flags
    lora_writeRegister(REG_IRQ_FLAGS, IRQ_ALL);

    // Start TX
    lora_writeRegister(REG_OP_MODE, MODE_LORA_TX);

    // timeout protection until 4sec TxDone 
    unsigned long start = millis();
    while ((lora_readRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE) == 0) {
        if (millis() - start > 4000) {   // safety timeout ~4s for SF12
            printf("TX timeout!\r\n");
            break;
        }
    }

    // Print sent bytes
    printf(" --> Transmission successful [%d bytes]: ", len);
    for (uint8_t i = 0; i < len; i++) {
        printf("0x%02X ", data[i]);
    }
    printf("\n");

    // Clear TxDone flag
    lora_writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE);

    printf("✅ LORA TX DONE ✅\n\n");
}

// ============================================================
//  RECEIVE — start continuous RX mode
// ============================================================

void lora_startReceive(void) {
    lora_writeRegister(REG_FIFO_RX_BASE_ADDR, FIFO_RX_BASE_DEFAULT);
    lora_writeRegister(REG_FIFO_ADDR_PTR,     FIFO_RX_BASE_DEFAULT);
    lora_writeRegister(REG_IRQ_FLAGS,          IRQ_ALL);              // clear flags
    lora_writeRegister(REG_OP_MODE,            MODE_LORA_RX_CONT);    // start listening
}

// Non-blocking poll
bool lora_available(void) {
    return (lora_readRegister(REG_IRQ_FLAGS) & IRQ_RX_DONE) != 0;
}

// Read last received packet
uint8_t lora_receive(uint8_t *buf, uint8_t max_len) {
    uint8_t irq_flags = lora_readRegister(REG_IRQ_FLAGS);

    printf("⚡ IRQ: 0x%02X -> ", irq_flags);

    // No packet received yet
    if (!(irq_flags & IRQ_RX_DONE)) {
        lora_writeRegister(REG_IRQ_FLAGS, IRQ_ALL);   // Clear all flags
        return 0;
    }

    // Check for CRC Error
    if (irq_flags & IRQ_PAYLOAD_CRC_ERROR) {
        printf("⚠️ RX CRC Error — packet discarded\n");
        lora_writeRegister(REG_IRQ_FLAGS, IRQ_ALL);   // Clear all flags
        return 0;
    }

    // Get payload length
    uint8_t len = lora_readRegister(REG_RX_NB_BYTES);
    if (len == 0 || len > max_len) {
        len = max_len;
    }

    // Set FIFO pointer to the start of the received packet
    uint8_t fifo_addr = lora_readRegister(REG_FIFO_RX_CURRENT_ADDR);
    lora_writeRegister(REG_FIFO_ADDR_PTR, fifo_addr);

    // Read RSSI BEFORE SPI burst
    uint8_t raw_rssi = lora_readRegister(REG_PKT_RSSI_VALUE);

    // Read the payload from FIFO
    digitalWrite(lora_cfg.pin_cs, LOW);
    SPI.transfer(REG_FIFO & 0x7F);        // Read mode
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = SPI.transfer(0x00);
    }
    digitalWrite(lora_cfg.pin_cs, HIGH);

    // Clear ALL IRQ flags (important!)
    lora_writeRegister(REG_IRQ_FLAGS, IRQ_ALL);

    int rssi = -157 + raw_rssi; // RSSI = -157 + raw_value (for SF7-12)
    printf("RX (%d bytes) RSSI:%d -> ", len, rssi);

    // Return to Continuous RX mode (very important!)
    lora_writeRegister(REG_OP_MODE, MODE_LORA_RX_CONT);

    return len;
}

/**
 * @brief Calculate LoRa Time on Air (ToA) using settings from lora_cfg
 * @param payload_len   Number of bytes in the payload (lora_tx_buffer_len)
 * @return Time on Air in milliseconds
 */
uint32_t calculateLoRaToA(uint8_t payload_len)
{
    if (payload_len == 0) return 0;

    // ====================== Extract settings from lora_cfg ======================

    // Spreading Factor from modem_config2
    uint8_t SF = (lora_cfg.modem_config2 >> 4) & 0x0F;           // bits [7:4]

    // Bandwidth from modem_config1
    uint8_t bw_code = (lora_cfg.modem_config1 >> 4) & 0x0F;
    uint32_t BW = 0;
    switch(bw_code) {
        case 0x00: BW =  7800;   break;
        case 0x01: BW = 10400;   break;
        case 0x02: BW = 15600;   break;
        case 0x03: BW = 20800;   break;
        case 0x04: BW = 31250;   break;
        case 0x05: BW = 41700;   break;
        case 0x06: BW = 62500;   break;
        case 0x07: BW = 125000;  break;
        case 0x08: BW = 250000;  break;
        case 0x09: BW = 500000;  break;
        default:   BW = 125000;  break;
    }

    // Coding Rate
    uint8_t cr_code = (lora_cfg.modem_config1 >> 1) & 0x07;
    uint8_t CR = cr_code + 4;                    // CR = 5,6,7,8 for 4/5 ... 4/8

    // Low Data Rate Optimization (LDRO)
    bool ldro = (lora_cfg.modem_config3 & 0x08) != 0;

    // Preamble length (we set it to 12, but we can read it if you want)
    uint16_t preamble = 12;   // You can make this readable from registers later

    bool explicitHeader = (lora_cfg.modem_config1 & 0x01) == 0;  // 0 = explicit
    bool crcEnabled     = (lora_cfg.modem_config2 & 0x04) != 0;  // bit 2

    // ====================== Actual ToA Calculation ======================
    float Ts = (1UL << SF) / (float)BW;                    // Symbol duration in seconds

    // Preamble duration (4.25 symbols extra for sync)
    float T_preamble = (preamble + 4.25f) * Ts;

    // Payload calculation
    int32_t numerator = 8 * payload_len 
                      - 4 * SF 
                      + 28 
                      + 16 * crcEnabled 
                      - 20 * (!explicitHeader);

    if (ldro) numerator += 4;   // Some references add this for DE bit

    float denominator = 4.0f * (SF - 2 * ldro);
    float payload_symbols = 8.0f + ceilf( numerator / denominator ) * (CR + 4);

    float T_payload = payload_symbols * Ts;

    float ToA_seconds = T_preamble + T_payload;

    return (uint32_t)roundf(ToA_seconds * 1000.0f);
}