#ifndef _7SEMI_SHT2x_H_
#define _7SEMI_SHT2x_H_

#include <Arduino.h>
#include <Wire.h>

/**
 * 7semi_SHT2x.h
 * --------------
 * Library   : 7semi_SHT2x — Driver for Sensirion SHT2x temperature & humidity sensor
 * Version   : 1.0.0
 * Author    : 7Semi
 * Maintainer: info@7semi.com
 * License   : MIT
 *
 * Features
 * --------
 * - Flexible begin(): fixed pins or custom SDA/SCL + clock (where supported)
 * - Temperature (°C) and Relative Humidity (%) with CRC-8 validation
 * - User register helpers: read/write, resolution, heater, OTP reload, battery flag
 * - Soft reset
 * - Electronic Serial Number (64-bit) with CRC verification (hex helper)
 *
 * Notes
 * -----
 * - HOLD-mode worst-case times: Temperature ≈ 85 ms, Humidity ≈ 29 ms
 * - Conversions:
 *     T(°C)  = -46.85 + 175.72 * raw / 65536
 *     RH(%)  =  -6.00 + 125.00 * raw / 65536
 * - Functions return false/NAN on I²C or CRC errors. No dynamic allocation.
 */

/* ===================== SHT2x Commands & Address ===================== */
#define DEFAULT_ADDR        0x40
#define SHT2x_TEMP_HOLD     0xE3
#define SHT2x_HUMI_HOLD     0xE5
#define SHT2x_USER_WRITE    0xE6
#define SHT2x_USER_READ     0xE7
#define SHT2x_SOFT_RESET    0xFE

/* ===================== Global Resolution Enum ===================== */
/**
 - Measurement resolution control (bits 7 and 0)
 - ‘00’: RH 12-bit / T 14-bit
 - ‘01’: RH  8-bit / T 12-bit
 - ‘10’: RH 10-bit / T 13-bit
 - ‘11’: RH 11-bit / T 11-bit
*/
enum class SHT2x_Resolution : uint8_t {
  RH12_T14 = 0b00000000,
  RH8_T12  = 0b00000001,
  RH10_T13 = 0b10000000,
  RH11_T11 = 0b10000001
};

/* ========================= Class ========================= */
class SHT2x_7Semi {
public:
  /**
   * - Default constructor (no Wire reference passed)
   */
  SHT2x_7Semi();


  /**
   * - Initialize with I²C address on an already-begun bus
   * - ADDR   : device address (default 0x40)
   * - return : true if device ACKs
   */
  bool begin(uint8_t ADDR = DEFAULT_ADDR);

  /**
   * - Initialize with I²C address and custom SDA/SCL/Clock (where supported)
   * - ADDR    : device address
   * - sda_pin : SDA pin (ignored on cores without remap)
   * - scl_pin : SCL pin (ignored on cores without remap)
   * - freq_hz : I²C clock in Hz (default 100000)
   * - return  : true if device ACKs
   */
  bool begin(uint8_t ADDR, int sda_pin, int scl_pin, uint32_t freq_hz = 100000UL);

  /**
   * - Read temperature in °C (HOLD mode)
   * - return : NAN on I²C/CRC error
   */
  float readTemperature();

  /**
   * - Read relative humidity in %RH (HOLD mode)
   * - return : NAN on I²C/CRC error
   */
  float readHumidity();

  /**
   * - Read user register
   * - return : register byte (0xFF on failure)
   */
  uint8_t readUserRegister();

  /**
   * - Write user register
   * - value  : byte to write
   * - return : true on ACK
   */
  bool writeUserRegister(uint8_t value);

  /**
   * - Set measurement resolution (only bits 7 and 0 changed)
   * - res    : resolution enum
   * - return : true on success
   */
   bool setResolution(SHT2x_Resolution res);

  /**
   * - Get measurement resolution from user register
   * - return : resolution enum (datasheet default on read failure)
   */
  SHT2x_Resolution getResolution();
  /**
   * - Battery status flag (bit6)
   * - return : true if VDD < 2.25V, false otherwise (false on read error)
   */
  bool batteryLow();

  /**
   * - On-chip heater control (bit2)
   * - enable : true=on, false=off
   * - return : true on success
   */
  bool enableHeater(bool enable);

  /**
   * - OTP reload disable (bit1)
   * - disable : true sets bit1=1 (disabled, default); false sets bit1=0 (enabled)
   * - return  : true on success
   */
  bool setOtpReloadDisabled(bool disable);

  /**
   * - Soft reset (15 ms typical)
   */
  void softReset();

  /**
   * - Read 64-bit electronic serial number
   * - sn     : out serial (MSB..LSB: SNA1 SNA0 SNB3 SNB2 SNB1 SNB0 SNC1 SNC0)
   * - return : true on success; false on I²C/CRC error
   */
  bool readSerialNumber(uint64_t &sn);

  /**
   * - Read serial number as uppercase hex (16 chars)
   * - return : empty string on failure
   */
  String readSerialNumberHex();

private:
  TwoWire *i2c;
  uint8_t  i2c_addr = DEFAULT_ADDR;

  /**
   * - Validate CRC for a 16-bit sample (poly 0x31)
   * - data  : 16-bit sample
   * - crc   : checksum byte
   * - return: true if CRC matches
   */
  bool checkCRC(uint16_t data, uint8_t crc);
};

#endif  // _7SEMI_SHT2x_H_
