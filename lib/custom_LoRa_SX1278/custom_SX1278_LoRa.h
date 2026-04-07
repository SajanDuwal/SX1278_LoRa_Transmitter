/**
 * @file    custom_SX1278_LoRa.h
 * @brief   SX1276/77/78/79 LoRa — Register Map, Bit Definitions & Driver API
 *          Based on Semtech SX1276/77/78/79 Datasheet Rev.4 - March 2015
 *
 * How to use:
 *   1. Set pins + config in custom_SX1278_LoRa.cpp
 *   2. Call lora_begin() in setup()
 *   3. Call lora_send() / lora_startReceive() / lora_receive() in loop()
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

// ============================================================
//  CHIP IDENTITY
// ============================================================
#define SX1278_VERSION_REG          0x42   ///< "Who am I" register address
#define SX1278_VERSION_VAL          0x12   ///< Expected chip version value

// ============================================================
//  SECTION 1 — COMMON REGISTERS (LoRa + FSK/OOK)
// ============================================================
#define REG_FIFO                    0x00   ///< FIFO read/write access
#define REG_OP_MODE                 0x01   ///< Operating mode & LoRa/FSK select
#define REG_FRF_MSB                 0x06   ///< Carrier frequency bits [23:16]
#define REG_FRF_MID                 0x07   ///< Carrier frequency bits [15:8]
#define REG_FRF_LSB                 0x08   ///< Carrier frequency bits [7:0]
#define REG_PA_CONFIG               0x09   ///< PA selection and output power
#define REG_PA_RAMP                 0x0A   ///< PA ramp-up/ramp-down time
#define REG_OCP                     0x0B   ///< Over current protection
#define REG_LNA                     0x0C   ///< LNA settings

// ============================================================
//  SECTION 2 — LoRa MODE REGISTERS
//  Only valid when REG_OP_MODE bit7 (LongRangeMode) = 1
// ============================================================

// FIFO Pointers
#define REG_FIFO_ADDR_PTR           0x0D   ///< FIFO SPI pointer
#define REG_FIFO_TX_BASE_ADDR       0x0E   ///< TX base address in FIFO
#define REG_FIFO_RX_BASE_ADDR       0x0F   ///< RX base address in FIFO
#define REG_FIFO_RX_CURRENT_ADDR    0x10   ///< Start address of last received packet

// IRQ / Status
#define REG_IRQ_FLAGS_MASK          0x11   ///< IRQ flag mask (1 = masked/disabled)
#define REG_IRQ_FLAGS               0x12   ///< IRQ flags — write 1 to clear
#define REG_RX_NB_BYTES             0x13   ///< Number of received bytes
#define REG_RX_HEADER_CNT_MSB       0x14
#define REG_RX_HEADER_CNT_LSB       0x15
#define REG_RX_PACKET_CNT_MSB       0x16
#define REG_RX_PACKET_CNT_LSB       0x17
#define REG_MODEM_STAT              0x18   ///< Live modem status
#define REG_PKT_SNR_VALUE           0x19   ///< SNR of last received packet
#define REG_PKT_RSSI_VALUE          0x1A   ///< RSSI of last received packet
#define REG_RSSI_VALUE              0x1B   ///< Current RSSI

// Hop
#define REG_HOP_CHANNEL             0x1C   ///< FHSS start channel & flags

// Modem Config
#define REG_MODEM_CONFIG1           0x1D   ///< BW, Coding Rate, Header mode
#define REG_MODEM_CONFIG2           0x1E   ///< SF, TX mode, CRC, RX timeout MSB
#define REG_SYMB_TIMEOUT_LSB        0x1F   ///< RX timeout LSB (symbols)
#define REG_PREAMBLE_MSB            0x20   ///< Preamble length MSB
#define REG_PREAMBLE_LSB            0x21   ///< Preamble length LSB
#define REG_PAYLOAD_LENGTH          0x22   ///< Payload length
#define REG_MAX_PAYLOAD_LENGTH      0x23   ///< Max payload length
#define REG_HOP_PERIOD              0x24   ///< FHSS hop period
#define REG_FIFO_RX_BYTE_ADDR       0x25   ///< Last byte written in RX FIFO
#define REG_MODEM_CONFIG3           0x26   ///< Low data rate optimize, AGC
#define REG_FEI_MSB                 0x28
#define REG_FEI_MID                 0x29
#define REG_FEI_LSB                 0x2A
#define REG_RSSI_WIDEBAND           0x2C
#define REG_DETECT_OPTIMIZE         0x31   ///< LoRa detection optimize (SF6)
#define REG_INVERT_IQ               0x33   ///< Invert I and Q signals
#define REG_DETECTION_THRESHOLD     0x37   ///< LoRa detection threshold
#define REG_SYNC_WORD               0x39   ///< LoRa sync word

// ============================================================
//  SECTION 3 — MISC / ANALOG REGISTERS
// ============================================================
#define REG_DIO_MAPPING1            0x40   ///< DIO0–DIO3 mapping
#define REG_DIO_MAPPING2            0x41   ///< DIO4–DIO5 mapping
#define REG_VERSION                 0x42   ///< Chip version
#define REG_TCXO                    0x4B   ///< TCXO or XTAL input
#define REG_PA_DAC                  0x4D   ///< High power +20 dBm enable
#define REG_FORMER_TEMP             0x5B   ///< Last image cal temperature
#define REG_AGC_REF                 0x61
#define REG_AGC_THRESH1             0x62
#define REG_AGC_THRESH2             0x63
#define REG_AGC_THRESH3             0x64
#define REG_PLL                     0x70   ///< PLL bandwidth

// ============================================================
//  SECTION 4 — REG_OP_MODE (0x01)
// ============================================================

// Bit flags
#define MODE_LONG_RANGE             0x80   ///< Bit7=1 → LoRa (set only in SLEEP)
#define MODE_ACCESS_SHARED_REG      0x40   ///< Bit6: access FSK regs from LoRa mode
#define MODE_LOW_FREQ_MODE_ON       0x08   ///< Bit3: low freq (sub-GHz) bands

// Mode bits [2:0]
#define MODE_SLEEP                  0x00   ///< 000 — SLEEP
#define MODE_STDBY                  0x01   ///< 001 — STANDBY
#define MODE_FSTX                   0x02   ///< 010 — FS TX
#define MODE_TX                     0x03   ///< 011 — TX
#define MODE_FSRX                   0x04   ///< 100 — FS RX
#define MODE_RX_CONTINUOUS          0x05   ///< 101 — RX continuous
#define MODE_RX_SINGLE              0x06   ///< 110 — RX single
#define MODE_CAD                    0x07   ///< 111 — CAD

// Ready-made LoRa mode combinations
#define MODE_LORA_SLEEP             (MODE_LONG_RANGE | MODE_SLEEP)         ///< 0x80
#define MODE_LORA_STDBY             (MODE_LONG_RANGE | MODE_STDBY)         ///< 0x81
#define MODE_LORA_TX                (MODE_LONG_RANGE | MODE_TX)            ///< 0x83
#define MODE_LORA_RX_CONT           (MODE_LONG_RANGE | MODE_RX_CONTINUOUS) ///< 0x85
#define MODE_LORA_RX_SINGLE         (MODE_LONG_RANGE | MODE_RX_SINGLE)     ///< 0x86
#define MODE_LORA_CAD               (MODE_LONG_RANGE | MODE_CAD)           ///< 0x87

// ============================================================
//  SECTION 5 — REG_PA_CONFIG (0x09)
// ============================================================
#define PA_BOOST                    0x80   ///< Use PA_BOOST pin
#define PA_RFO                      0x00   ///< Use RFO pin (up to +14 dBm)
#define PA_MAX_POWER_MASK           0x70   ///< bits [6:4] MaxPower (RFO only)
#define PA_OUTPUT_POWER_MASK        0x0F   ///< bits [3:0] OutputPower

// PA_BOOST presets  →  Pout = 17 - (15 - OutputPower) dBm
#define PA_BOOST_2DBM               (PA_BOOST | 0x00)  ///< +2  dBm
#define PA_BOOST_5DBM               (PA_BOOST | 0x03)  ///< +5  dBm
#define PA_BOOST_10DBM              (PA_BOOST | 0x08)  ///< +10 dBm
#define PA_BOOST_14DBM              (PA_BOOST | 0x0C)  ///< +14 dBm
#define PA_BOOST_17DBM              (PA_BOOST | 0x0F)  ///< +17 dBm (standard max)
#define PA_BOOST_20DBM              0x8F  ///< Use this for +20 dBm with PA_DAC_BOOST_20DBM

// ============================================================
//  SECTION 6 — REG_PA_DAC (0x4D): +20 dBm HIGH POWER
// ============================================================
#define PA_DAC_DEFAULT              0x84   ///< Normal operation
#define PA_DAC_BOOST_20DBM          0x87   ///< Enable +20 dBm (also raise OCP)

// ============================================================
//  SECTION 7 — REG_OCP (0x0B): OVER-CURRENT PROTECTION
// ============================================================
#define OCP_ON                      0x20   ///< Bit5 = OCP enable
#define OCP_TRIM_MASK               0x1F   ///< bits [4:0]
#define OCP_100MA                   (OCP_ON | 0x0B)  ///< ~100 mA (default)
#define OCP_120MA                   (OCP_ON | 0x0F)  ///< 120 mA
#define OCP_150MA                   (OCP_ON | 0x12)  ///< 150 mA (+20 dBm)
#define OCP_240MA                   (OCP_ON | 0x1B)  ///< 240 mA (max)

// ============================================================
//  SECTION 8 — REG_MODEM_CONFIG1 (0x1D)
//  [7:4] BW  |  [3:1] CR  |  [0] HeaderMode
// ============================================================

// Bandwidth [7:4]
#define BW_7_8KHZ                   0x00
#define BW_10_4KHZ                  0x10
#define BW_15_6KHZ                  0x20
#define BW_20_8KHZ                  0x30
#define BW_31_25KHZ                 0x40
#define BW_41_7KHZ                  0x50
#define BW_62_5KHZ                  0x60
#define BW_125KHZ                   0x70   ///< Common default
#define BW_250KHZ                   0x80
#define BW_500KHZ                   0x90

// Coding Rate [3:1]
#define CR_4_5                      0x02   ///< Lowest overhead
#define CR_4_6                      0x04
#define CR_4_7                      0x06
#define CR_4_8                      0x08   ///< Most robust

// Header mode [0]
#define HEADER_EXPLICIT             0x00   ///< Include header (default)
#define HEADER_IMPLICIT             0x01   ///< No header (fixed length only)

// Combined presets
#define MODEM_CONFIG1_BW125_CR45_EXPL   (BW_125KHZ | CR_4_5 | HEADER_EXPLICIT) ///< 0x72
#define MODEM_CONFIG1_BW250_CR45_EXPL   (BW_250KHZ | CR_4_5 | HEADER_EXPLICIT) ///< 0x82
#define MODEM_CONFIG1_BW500_CR45_EXPL   (BW_500KHZ | CR_4_5 | HEADER_EXPLICIT) ///< 0x92

// ============================================================
//  SECTION 9 — REG_MODEM_CONFIG2 (0x1E)
//  [7:4] SF  |  [3] TxMode  |  [2] CRC  |  [1:0] RxTimeout MSB
// ============================================================

// Spreading Factor [7:4]
#define SF_6                        0x60   ///< Fastest / shortest range
#define SF_7                        0x70
#define SF_8                        0x80
#define SF_9                        0x90
#define SF_10                       0xA0
#define SF_11                       0xB0
#define SF_12                       0xC0   ///< Slowest / longest range

// TX mode [3]
#define TX_MODE_NORMAL              0x00
#define TX_MODE_CONTINUOUS          0x08   ///< Testing only

// CRC [2]
#define RX_PAYLOAD_CRC_OFF          0x00
#define RX_PAYLOAD_CRC_ON           0x04   ///< Recommended

// Combined presets (Normal TX + CRC ON)
#define MODEM_CONFIG2_SF7_CRC_ON    (SF_7  | TX_MODE_NORMAL | RX_PAYLOAD_CRC_ON) ///< 0x74
#define MODEM_CONFIG2_SF8_CRC_ON    (SF_8  | TX_MODE_NORMAL | RX_PAYLOAD_CRC_ON) ///< 0x84
#define MODEM_CONFIG2_SF9_CRC_ON    (SF_9  | TX_MODE_NORMAL | RX_PAYLOAD_CRC_ON) ///< 0x94
#define MODEM_CONFIG2_SF10_CRC_ON   (SF_10 | TX_MODE_NORMAL | RX_PAYLOAD_CRC_ON) ///< 0xA4
#define MODEM_CONFIG2_SF11_CRC_ON   (SF_11 | TX_MODE_NORMAL | RX_PAYLOAD_CRC_ON) ///< 0xB4
#define MODEM_CONFIG2_SF12_CRC_ON   (SF_12 | TX_MODE_NORMAL | RX_PAYLOAD_CRC_ON) ///< 0xC4

// ============================================================
//  SECTION 10 — REG_MODEM_CONFIG3 (0x26)
// ============================================================
#define LOW_DATA_RATE_OPTIMIZE_OFF  0x00   ///< Default (SF7–SF10 @ BW 125kHz)
#define LOW_DATA_RATE_OPTIMIZE_ON   0x08   ///< Mandatory for SF11/SF12 @ BW 125kHz
#define AGC_AUTO_OFF                0x00
#define AGC_AUTO_ON                 0x04   ///< Let hardware control LNA gain

#define MODEM_CONFIG3_DEFAULT       (LOW_DATA_RATE_OPTIMIZE_OFF | AGC_AUTO_ON) ///< 0x04
#define MODEM_CONFIG3_LDRO_ON       (LOW_DATA_RATE_OPTIMIZE_ON  | AGC_AUTO_ON) ///< 0x0C

// ============================================================
//  SECTION 11 — REG_IRQ_FLAGS (0x12) — write 1 to clear
// ============================================================
#define IRQ_RX_TIMEOUT              0x80
#define IRQ_RX_DONE                 0x40
#define IRQ_PAYLOAD_CRC_ERROR       0x20
#define IRQ_VALID_HEADER            0x10
#define IRQ_TX_DONE                 0x08
#define IRQ_CAD_DONE                0x04
#define IRQ_FHSS_CHANGE_CHANNEL     0x02
#define IRQ_CAD_DETECTED            0x01
#define IRQ_ALL                     0xFF   ///< Clear all flags at once

// ============================================================
//  SECTION 12 — FIFO & PAYLOAD DEFAULTS
// ============================================================
#define FIFO_TX_BASE_DEFAULT        0x80   ///< TX starts at byte 128
#define FIFO_RX_BASE_DEFAULT        0x00   ///< RX starts at byte 0
#define MAX_PAYLOAD_LENGTH_DEFAULT  0xFF   ///< Allow full 255-byte payload

// ============================================================
//  SECTION 13 — SYNC WORD
// ============================================================
#define LORA_SYNC_WORD_PUBLIC       0x34   ///< LoRaWAN / public network
#define LORA_SYNC_WORD_PRIVATE      0x12   ///< Private network

// ============================================================
//  SECTION 14 — DETECT OPTIMIZE & THRESHOLD (SF6 only)
// ============================================================
#define DETECT_OPTIMIZE_SF7_SF12    0x03   ///< Default
#define DETECT_OPTIMIZE_SF6         0x05   ///< Required for SF6
#define DETECTION_THRESHOLD_SF7_SF12 0x0A  ///< Default
#define DETECTION_THRESHOLD_SF6     0x0C   ///< Required for SF6

// ============================================================
//  SECTION 15 — LNA (REG_LNA 0x0C)
// ============================================================
#define LNA_GAIN_MAX                0x20   ///< G1 = max gain
#define LNA_GAIN_G2                 0x40
#define LNA_GAIN_G3                 0x60
#define LNA_GAIN_G4                 0x80
#define LNA_GAIN_G5                 0xA0
#define LNA_GAIN_MIN                0xC0   ///< G6 = min gain
#define LNA_BOOST_OFF               0x00
#define LNA_BOOST_ON                0x03   ///< 150% LNA current (sub-GHz)
#define LNA_DEFAULT                 (LNA_GAIN_MAX | LNA_BOOST_ON) ///< 0x23

// ============================================================
//  SECTION 16 — DIO MAPPING (REG_DIO_MAPPING1 0x40)
// ============================================================
#define DIO0_RXDONE                 0x00   ///< DIO0 → RxDone
#define DIO0_TXDONE                 0x40   ///< DIO0 → TxDone
#define DIO0_CADDONE                0x80   ///< DIO0 → CadDone

typedef struct { // LoRa configuration struct — set these fields in main.cpp and call lora_setConfig()
    // Pins
    uint8_t pin_cs;
    uint8_t pin_mosi;
    uint8_t pin_miso;
    uint8_t pin_sck;
    uint8_t pin_reset;
    uint8_t pin_dio0;

    // LoRa config
    uint32_t frequency;
    uint8_t modem_config1;
    uint8_t modem_config2;
    uint8_t modem_config3;
    uint8_t pa_config;
    uint8_t sync_word;
} lora_config_t;

// ============================================================
//  DRIVER API — implemented in custom_SX1278_LoRa.cpp
// ============================================================

/**
 * @brief  Apply LoRa config settings from lora_config_t struct.
 *         Call this before lora_begin() to set custom config.
 */
void lora_setConfig(lora_config_t *cfg);

/**
 * @brief  Write one byte to a register over SPI.
 */
void    lora_writeRegister(uint8_t addr, uint8_t data);

/**
 * @brief  Read one byte from a register over SPI.
 * @return Register value
 */
uint8_t lora_readRegister(uint8_t addr);

/**
 * @brief  Hardware reset the SX1278 via the RESET pin.
 */
void    lora_reset(void);

/**
 * @brief  Set the RF carrier frequency.
 *         Frf register = (freq_hz * 2^19) / 32000000
 *         Uses 64-bit arithmetic to avoid overflow on 32-bit MCUs.
 * @param  freq_hz  Frequency in Hz (e.g. 433000000UL)
 * @return true on success, false if readback fails
 */
bool    lora_setFrequency(uint32_t freq_hz);

/**
 * @brief  Full chip init: reset → LoRa mode → apply all config registers.
 * @return true on success
 */
bool    lora_begin(void);

/**
 * @brief  Transmit a packet (blocking — waits for TX done IRQ).
 * @param  data  Pointer to payload buffer
 * @param  len   Number of bytes (max 255)
 */
void    lora_send(uint8_t *data, uint8_t len);

/**
 * @brief  Start continuous RX mode (non-blocking).
 *         Poll lora_available() to check for incoming packets.
 */
void    lora_startReceive(void);

/**
 * @brief  Non-blocking check — has a full packet arrived?
 * @return true if IRQ_RX_DONE is set
 */
bool    lora_available(void);

/**
 * @brief  Read the last received packet into buf.
 *         Clears the RxDone IRQ flag after reading.
 * @param  buf      Destination buffer
 * @param  max_len  Buffer capacity
 * @return Number of bytes actually copied
 */
uint8_t lora_receive(uint8_t *buf, uint8_t max_len);

/**
 * @brief Calculate LoRa Time on Air (ToA) using settings from lora_cfg
 * @param payload_len   Number of bytes in the payload (lora_tx_buffer_len)
 * @return Time on Air in milliseconds
 */
uint32_t calculateLoRaToA(uint8_t payload_len);