/***************************************************************************************************
 * 7Semi_SHT2x.cpp
 * ----------------
 * Library   : 7Semi_SHT2x — Driver for Sensirion SHT2x temperature & humidity sensor
 * Version   : 1.0.0
 * Author    : 7Semi
 * Maintainer: info@7semi.com
 * License   : MIT
 *
 * -----------------------
 * - begin() with fixed pins, begin(addr, sda, scl, freq) with flexible pins
 * - readTemperature(), readHumidity() with CRC-8 check and status-bit clear
 * - User register helpers: read/write, set/get resolution, heater, OTP reload, battery flag
 * - Soft reset
 * - Serial number read (64-bit) with CRC verification
 *
 * Implementation Notes
 * --------------------
 * - Conversion:
 *     * T(°C)  = -46.85 + 175.72 * raw / 65536
 *     * RH(%)  =  -6.00 + 125.00 * raw / 65536
 * - Timing:
 *     * HOLD-mode worst case: T ≈85 ms, RH ≈29 ms
 * - Error handling:
 *     * Functions return false or NAN on I²C/CRC failures; no dynamic allocation.
 ***************************************************************************************************/

#include "7Semi_SHT2x.h"

/* ================= CRC helper ================= */

/**
 - Sensirion CRC-8 (poly 0x31, init 0x00)
 - data    : pointer to bytes
 - nbytes  : number of bytes
 - checksum: expected CRC
 - return  : true if CRC matches
*/
static bool crc8_ok(const uint8_t *data, uint8_t nbytes, uint8_t checksum) {
  uint8_t crc = 0x00;
  const uint8_t poly = 0x31;
  for (uint8_t i = 0; i < nbytes; ++i) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; ++b) {
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ poly) : (uint8_t)(crc << 1);
    }
  }
  return (crc == checksum);
}

/* ================= Constructor ================= */
/**
 - Default constructor (no Wire reference passed)
 - The Wire instance is selected/initialized in begin()
*/
SHT2x_7Semi::SHT2x_7Semi()
  : i2c(&Wire) {}

/* ================= Begin / Init ================= */
/**
 - Initialize with I²C address on default pins
 - Sets i2c = &Wire and begins the bus
 - return: true if device ACKs
*/
bool SHT2x_7Semi::begin(uint8_t ADDR) {
  i2c_addr = ADDR;
  i2c = &Wire;
  i2c->begin();
#if defined(WIRE_HAS_SET_CLOCK) || defined(TWBR)
  i2c->setClock(100000UL);
#endif
  i2c->beginTransmission(i2c_addr);
  return (i2c->endTransmission() == 0);
}

/**
 - Initialize with custom SDA/SCL and clock (where supported)
 - Sets i2c = &Wire and configures pins/clock per platform
 - return: true if device ACKs
*/
bool SHT2x_7Semi::begin(uint8_t ADDR, int sda_pin, int scl_pin, uint32_t freq_hz ) {
  i2c_addr = ADDR;
  i2c = &Wire;

#if defined(ARDUINO_ARCH_ESP32)
  i2c->begin((int)sda_pin, (int)scl_pin, (uint32_t)freq_hz);
#elif defined(ARDUINO_ARCH_ESP8266)
  i2c->begin((sda_pin >= 0) ? sda_pin : SDA, (scl_pin >= 0) ? scl_pin : SCL);
  i2c->setClock(freq_hz);
#elif defined(ARDUINO_ARCH_RP2040)
  i2c->setSDA((int)sda_pin);
  i2c->setSCL((int)scl_pin);
  i2c->begin();
  i2c->setClock(freq_hz);
#elif defined(ARDUINO_ARCH_SAMD)
#ifdef WIRE_HAS_SET_PINS
  i2c->setPins((int)sda_pin, (int)scl_pin);
#endif
  i2c->begin();
  i2c->setClock(freq_hz);
#else
  (void)sda_pin;
  (void)scl_pin;
  i2c->begin();
  i2c->setClock(freq_hz);
#if defined(WIRE_HAS_SET_CLOCK)
  i2c->setClock(freq_hz);
#endif
#endif

  i2c->beginTransmission(i2c_addr);
  return (i2c->endTransmission() == 0);
}

/* ================ Measurements ================ */

/**
 - Read temperature in °C (HOLD mode)
 - return: NAN on I²C/CRC error
*/
float SHT2x_7Semi::readTemperature() {
  i2c->beginTransmission(i2c_addr);
  i2c->write(SHT2x_TEMP_HOLD);
  i2c->endTransmission();

  delay(85);

  i2c->requestFrom(i2c_addr, (uint8_t)3);
  if (i2c->available() < 3) return NAN;

  uint16_t raw = (uint16_t(i2c->read()) << 8) | i2c->read();
  uint8_t crc = i2c->read();
  if (!checkCRC(raw, crc)) return NAN;

  raw &= ~0x0003;
  return -46.85f + 175.72f * (float)raw / 65536.0f;
}

/**
 - Read relative humidity in %RH (HOLD mode)
 - return: NAN on I²C/CRC error
*/
float SHT2x_7Semi::readHumidity() {
  i2c->beginTransmission(i2c_addr);
  i2c->write(SHT2x_HUMI_HOLD);
  i2c->endTransmission();

  delay(29);

  i2c->requestFrom(i2c_addr, (uint8_t)3);
  if (i2c->available() < 3) return NAN;

  uint16_t raw = (uint16_t(i2c->read()) << 8) | i2c->read();
  uint8_t crc = i2c->read();
  if (!checkCRC(raw, crc)) return NAN;

  raw &= ~0x0003;
  return -6.0f + 125.0f * (float)raw / 65536.0f;
}

/* ================ User Register ================ */

/**
 - Read user register (0xFF on failure)
*/
uint8_t SHT2x_7Semi::readUserRegister() {
  i2c->beginTransmission(i2c_addr);
  i2c->write(SHT2x_USER_READ);
  i2c->endTransmission();

  i2c->requestFrom(i2c_addr, (uint8_t)1);
  if (i2c->available()) return i2c->read();
  return 0xFF;
}

/**
 - Write user register
 - value : byte to write
 - return: true on ACK
*/
bool SHT2x_7Semi::writeUserRegister(uint8_t value) {
  i2c->beginTransmission(i2c_addr);
  i2c->write(SHT2x_USER_WRITE);
  i2c->write(value);
  return (i2c->endTransmission() == 0);
}

/**
 - Set measurement resolution (affects bits 7 and 0)
 - res   : enum resolution
 - return: true on success
*/
bool SHT2x_7Semi::setResolution(SHT2x_Resolution res) {
  uint8_t reg = readUserRegister();
  if (reg == 0xFF) return false;
  reg &= ~(uint8_t)0b10000001;
  reg |=  static_cast<uint8_t>(res) & 0b10000001;
  return writeUserRegister(reg);
}

/**
 - Get measurement resolution from user register
 - return: enum (default datasheet resolution on read failure)
*/
SHT2x_Resolution SHT2x_7Semi::getResolution() {
  uint8_t reg = readUserRegister();
  if (reg == 0xFF) return SHT2x_Resolution::RH12_T14;

  switch (reg & 0b10000001) {
    case 0b00000001: return SHT2x_Resolution::RH8_T12;
    case 0b10000000: return SHT2x_Resolution::RH10_T13;
    case 0b10000001: return SHT2x_Resolution::RH11_T11;
    default:         return SHT2x_Resolution::RH12_T14;
  }
}

/**
 - Battery status (bit6)
 - return: true if VDD < 2.25V, false otherwise (false on read error)
*/
bool SHT2x_7Semi::batteryLow() {
  uint8_t reg = readUserRegister();
  if (reg == 0xFF) return false;
  return (reg & 0b01000000) != 0;
}

/**
 - Heater control (bit2)
 - enable: true to enable, false to disable
 - return: true on success
*/
bool SHT2x_7Semi::enableHeater(bool enable) {
  uint8_t reg = readUserRegister();
  if (reg == 0xFF) return false;

  if (enable) reg |= 0b00000100;
  else reg &= ~0b00000100;
  return writeUserRegister(reg);
}

/**
 - OTP reload disable (bit1)
 - disable: true sets bit1=1 (disabled, default); false sets bit1=0 (enabled)
 - return : true on success
*/
bool SHT2x_7Semi::setOtpReloadDisabled(bool disable) {
  uint8_t reg = readUserRegister();
  if (reg == 0xFF) return false;

  if (disable) reg |= 0b00000010;
  else reg &= ~0b00000010;
  return writeUserRegister(reg);
}

/* ================ Reset ================ */

/**
 - Soft reset
*/
void SHT2x_7Semi::softReset() {
  i2c->beginTransmission(i2c_addr);
  i2c->write(SHT2x_SOFT_RESET);
  i2c->endTransmission();
  delay(15);
}

/* ================ CRC over sample ================ */

/**
 - Verify CRC for a 16-bit sample with 1 CRC byte
 - data: 16-bit sample
 - crc : checksum
 - return: true if CRC matches
*/
bool SHT2x_7Semi::checkCRC(uint16_t data, uint8_t crc) {
  uint8_t buf[2] = { (uint8_t)(data >> 8), (uint8_t)(data & 0xFF) };

  uint8_t calc = 0x00;
  const uint8_t poly = 0x31;

  for (uint8_t i = 0; i < 2; ++i) {
    calc ^= buf[i];
    for (uint8_t b = 0; b < 8; ++b) {
      calc = (calc & 0x80) ? (uint8_t)((calc << 1) ^ poly) : (uint8_t)(calc << 1);
    }
  }
  return (calc == crc);
}

/* ================ Serial Number (64-bit) ================ */

/**
 - Read 64-bit serial number with CRC checks
 - sn : out serial (MSB..LSB: SNA1 SNA0 SNB3 SNB2 SNB1 SNB0 SNC1 SNC0)
 - return: true on success
*/
bool SHT2x_7Semi::readSerialNumber(uint64_t &sn) {
  i2c->beginTransmission(i2c_addr);
  i2c->write(0xFA);
  i2c->write(0x0F);
  if (i2c->endTransmission() != 0) return false;

  i2c->requestFrom(i2c_addr, (uint8_t)8);
  if (i2c->available() < 8) return false;

  uint8_t p1[8];
  for (uint8_t i = 0; i < 8; ++i) p1[i] = i2c->read();

  if (!crc8_ok(&p1[0], 1, p1[1])) return false;
  if (!crc8_ok(&p1[2], 1, p1[3])) return false;
  if (!crc8_ok(&p1[4], 1, p1[5])) return false;
  if (!crc8_ok(&p1[6], 1, p1[7])) return false;

  uint8_t SNB3 = p1[0], SNB2 = p1[2], SNB1 = p1[4], SNB0 = p1[6];

  i2c->beginTransmission(i2c_addr);
  i2c->write(0xFC);
  i2c->write(0xC9);
  if (i2c->endTransmission() != 0) return false;

  i2c->requestFrom(i2c_addr, (uint8_t)6);
  if (i2c->available() < 6) return false;

  uint8_t p2[6];
  for (uint8_t i = 0; i < 6; ++i) p2[i] = i2c->read();

  if (!crc8_ok(&p2[0], 2, p2[2])) return false;
  if (!crc8_ok(&p2[3], 2, p2[5])) return false;

  uint8_t SNC1 = p2[0], SNC0 = p2[1];
  uint8_t SNA1 = p2[3], SNA0 = p2[4];

  uint8_t bytes[8] = { SNA1, SNA0, SNB3, SNB2, SNB1, SNB0, SNC1, SNC0 };

  sn = 0;
  for (uint8_t i = 0; i < 8; ++i) sn = (sn << 8) | bytes[i];
  return true;
}

/**
 - Read serial number as uppercase hex (16 chars)
 - return: "" on failure
*/
String SHT2x_7Semi::readSerialNumberHex() {
  uint64_t sn;
  if (!readSerialNumber(sn)) return String("");
  char buf[17];
  for (int i = 15; i >= 0; --i) {
    uint8_t nib = (uint8_t)((sn >> (4 * (15 - i))) & 0x0F);
    buf[15 - i] = (nib < 10) ? char('0' + nib) : char('A' + (nib - 10));
  }
  buf[16] = '\0';
  return String(buf);
}
