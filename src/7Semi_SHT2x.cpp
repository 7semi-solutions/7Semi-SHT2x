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

SHT2x_7Semi::SHT2x_7Semi() {}

bool SHT2x_7Semi::begin(uint8_t i2cAddress, TwoWire &i2cPort, uint32_t i2cSpeed)
{
  i2c_address = i2cAddress;
  i2c = &i2cPort;

  i2c->begin();
  i2c->setClock(i2cSpeed);

  // Check if device connected or not
  i2c->beginTransmission(i2c_address);
  if (i2c->endTransmission() != 0)
    return false;

  // Set default resolution (RH12 / T14)
  if (!setResolution(SHT2x_Resolution::RH12_T14))
    return false;

  return true;
}

bool SHT2x_7Semi::startTemperatureMeasurement()
{
  return sendCommand(SHT2x_TRIGGER_TEMP);
}

bool SHT2x_7Semi::startHumidityMeasurement()
{
  return sendCommand(SHT2x_TRIGGER_HUMI);
}

bool SHT2x_7Semi::readMeasurement(uint16_t &raw_measurement)
{
  uint8_t data[3];
  if (!readData(data, 3))
    return false;

  raw_measurement = (uint16_t(data[0]) << 8) | data[1];
  uint8_t crc = data[2];

  if (!checkCRC(raw_measurement, crc))
    return false;

  return true;
}

inline float SHT2x_7Semi::rawToTemperature(uint16_t raw) const
{
  raw &= ~0x0003;
  return -46.85f + 175.72f * (float)raw / 65536.0f;
}

inline float SHT2x_7Semi::rawToHumidity(uint16_t raw) const
{
  raw &= ~0x0003;
  return -6.0f + 125.0f * (float)raw / 65536.0f;
}

bool SHT2x_7Semi::readTemperatureMeasurement(float &temperature)
{
  uint16_t temperature_raw;

  if (!readMeasurement(temperature_raw))
    return false;

  temperature = rawToTemperature(temperature_raw);
  return true;
}

bool SHT2x_7Semi::readHumidityMeasurement(float &humidity)
{
  uint16_t humidity_raw;

  if (!readMeasurement(humidity_raw))
    return false;

  humidity = rawToHumidity(humidity_raw);
  return true;
}

bool SHT2x_7Semi::readTemperatureRaw(uint16_t &temperatureRaw)
{
  uint8_t data[3];
  if (!burstRead(SHT2x_TRIGGER_TEMP, data, 3))
    return false;

  uint16_t raw = (uint16_t(data[0]) << 8) | data[1];
  uint8_t crc = data[2];

  if (!checkCRC(raw, crc))
    return false;

  temperatureRaw = raw;

  return true;
}

bool SHT2x_7Semi::readTemperature(float &temperatureC)
{
  uint16_t temperature_raw;
  if (!readTemperatureRaw(temperature_raw))
    return false;

  temperatureC = rawToTemperature(temperature_raw);
  return true;
}

bool SHT2x_7Semi::readHumidityRaw(uint16_t &HumidityRaw)
{
  uint8_t data[3];
  if (!burstRead(SHT2x_TRIGGER_HUMI, data, 3))
    return false;

  uint16_t raw = (uint16_t(data[0]) << 8) | data[1];
  uint8_t crc = data[2];

  if (!checkCRC(raw, crc))
    return false;

  HumidityRaw = raw;

  return true;
}

bool SHT2x_7Semi::readHumidity(float &humidity)
{
  uint16_t humidity_raw;
  if (!readHumidityRaw(humidity_raw))
    return false;

  humidity = rawToHumidity(humidity_raw);
  return true;
}

bool SHT2x_7Semi::readUserRegister(uint8_t &data)
{
  return readReg(SHT2x_USER_READ, data);
}

bool SHT2x_7Semi::writeUserRegister(uint8_t data)
{
  return writeReg(SHT2x_USER_WRITE, data);
}

bool SHT2x_7Semi::setResolution(SHT2x_Resolution res)
{
  uint8_t v;
  if (!readUserRegister(v))
    return false;

  v &= ~0x81;             // clear resolution bits
  v |= (uint8_t)res;      // set new resolution

  if (!writeUserRegister(v))
    return false;

  resolution = res;

  return true;
}

bool SHT2x_7Semi::getResolution(SHT2x_Resolution &res)
{
  uint8_t v;
  if (!readUserRegister(v))
    return false;

  switch (v & 0x81)
  {
    case 0x01: res = SHT2x_Resolution::RH8_T12; break;
    case 0x80: res = SHT2x_Resolution::RH10_T13; break;
    case 0x81: res = SHT2x_Resolution::RH11_T11; break;
    default:   res = SHT2x_Resolution::RH12_T14; break;
  }

  return true;
}

bool SHT2x_7Semi::getBatteryStatus(bool &batteryLow)
{
  uint8_t v;
  if (!readUserRegister(v))
    return false;

  batteryLow = (v & 0x40) != 0;

  return true;
}

bool SHT2x_7Semi::setHeater(bool enable)
{
  uint8_t v;
  if (!readUserRegister(v))
    return false;

  if (enable)
    v |= 0x04;
  else
    v &= ~0x04;

  return writeUserRegister(v);
}

bool SHT2x_7Semi::getHeater(bool &enable)
{
  uint8_t v;
  if (!readUserRegister(v))
    return false;

  enable = (v & 0x04) != 0;

  return true;
}

bool SHT2x_7Semi::setOtpReload(bool enable)
{
  uint8_t v;
  if (!readUserRegister(v))
    return false;

  if (enable)
    v &= ~0x02;
  else
    v |= 0x02;

  return writeUserRegister(v);
}

bool SHT2x_7Semi::getOtpReload(bool &enabled)
{
  uint8_t v;
  if (!readUserRegister(v))
    return false;

  enabled = (v & 0x02) == 0;

  return true;
}

bool SHT2x_7Semi::softReset()
{
  if (!sendCommand(SHT2x_SOFT_RESET))
    return false;

  delay(20);

  return true;
}

uint16_t SHT2x_7Semi::getMeasurementTime(SHT2x_Resolution resolution,
                                         SHT2x_SensorType type)
{
  switch (resolution)
  {
    case SHT2x_Resolution::RH12_T14:
      return (type == SHT2x_SensorType::TEMPERATURE) ? 85 : 29;

    case SHT2x_Resolution::RH8_T12:
      return (type == SHT2x_SensorType::TEMPERATURE) ? 22 : 4;

    case SHT2x_Resolution::RH10_T13:
      return (type == SHT2x_SensorType::TEMPERATURE) ? 43 : 9;

    case SHT2x_Resolution::RH11_T11:
      return 11; // same for both

    default:
      return 85;
  }
}

uint16_t SHT2x_7Semi::getMeasurementTime(SHT2x_SensorType type)
{
  return getMeasurementTime(resolution, type);
}

bool SHT2x_7Semi::checkCRC(uint16_t data, uint8_t crc)
{
  uint8_t buf[2] = {
    (uint8_t)(data >> 8),
    (uint8_t)(data & 0xFF)
  };

  uint8_t calc = 0x00;
  const uint8_t poly = 0x31;

  for (uint8_t i = 0; i < 2; ++i)
  {
    calc ^= buf[i];
    for (uint8_t b = 0; b < 8; ++b)
    {
      calc = (calc & 0x80)
             ? (uint8_t)((calc << 1) ^ poly)
             : (uint8_t)(calc << 1);
    }
  }

  return (calc == crc);
}

bool SHT2x_7Semi::readSerialNumber(uint64_t &sn)
{
  i2c->beginTransmission(i2c_address);
  i2c->write(0xFA);
  i2c->write(0x0F);
  if (i2c->endTransmission() != 0)
    return false;

  i2c->requestFrom(i2c_address, (uint8_t)8);
  if (i2c->available() < 8)
    return false;

  uint8_t p1[8];
  for (uint8_t i = 0; i < 8; ++i)
    p1[i] = i2c->read();

  if (!crc8_ok(&p1[0], 1, p1[1]))
    return false;
  if (!crc8_ok(&p1[2], 1, p1[3]))
    return false;
  if (!crc8_ok(&p1[4], 1, p1[5]))
    return false;
  if (!crc8_ok(&p1[6], 1, p1[7]))
    return false;

  uint8_t SNB3 = p1[0], SNB2 = p1[2], SNB1 = p1[4], SNB0 = p1[6];

  i2c->beginTransmission(i2c_address);
  i2c->write(0xFC);
  i2c->write(0xC9);
  if (i2c->endTransmission() != 0)
    return false;

  i2c->requestFrom(i2c_address, (uint8_t)6);
  if (i2c->available() < 6)
    return false;

  uint8_t p2[6];
  for (uint8_t i = 0; i < 6; ++i)
    p2[i] = i2c->read();

  if (!crc8_ok(&p2[0], 2, p2[2]))
    return false;
  if (!crc8_ok(&p2[3], 2, p2[5]))
    return false;

  uint8_t SNC1 = p2[0], SNC0 = p2[1];
  uint8_t SNA1 = p2[3], SNA0 = p2[4];

  uint8_t bytes[8] = {SNA1, SNA0, SNB3, SNB2, SNB1, SNB0, SNC1, SNC0};

  sn = 0;
  for (uint8_t i = 0; i < 8; ++i)
    sn = (sn << 8) | bytes[i];
  return true;
}

String SHT2x_7Semi::readSerialNumberHex()
{
  uint64_t sn;
  if (!readSerialNumber(sn))
    return String("");
  char buf[17];
  for (int i = 15; i >= 0; --i)
  {
    uint8_t nib = (uint8_t)((sn >> (4 * (15 - i))) & 0x0F);
    buf[15 - i] = (nib < 10) ? char('0' + nib) : char('A' + (nib - 10));
  }
  buf[16] = '\0';
  return String(buf);
}

bool SHT2x_7Semi::sendCommand(uint8_t cmd)
{
  if (!i2c)
    return false;

  i2c->beginTransmission(i2c_address);
  i2c->write(cmd);

  return (i2c->endTransmission() == 0);
}

bool SHT2x_7Semi::writeReg(uint8_t reg, uint8_t value)
{
  if (!i2c)
    return false;

  i2c->beginTransmission(i2c_address);
  i2c->write(reg);
  i2c->write(value);

  return (i2c->endTransmission() == 0);
}

bool SHT2x_7Semi::readData(uint8_t *buf, size_t len)
{
  if (!i2c || !buf || len == 0)
    return false;

  size_t got = i2c->requestFrom((int)i2c_address, (int)len);
  if (got != len)
    return false;

  for (size_t i = 0; i < len; i++)
  {
    if (!i2c->available())
      return false;

    buf[i] = i2c->read();
  }

  return true;
}

bool SHT2x_7Semi::readReg(uint8_t reg, uint8_t &value)
{
  return burstRead(reg, &value, 1);
}

bool SHT2x_7Semi::burstRead(uint8_t reg, uint8_t *buf, size_t len)
{
  if (!buf || len == 0)
    return false;

  if (!i2c)
    return false;

  i2c->beginTransmission(i2c_address);
  i2c->write(reg);

  // Repeated start (no STOP)
  if (i2c->endTransmission(false) != 0)
    return false;

  size_t got = 0;
  uint16_t counter = 0;

  // Poll until data is ready
  while (got < len)
  {
    got = i2c->requestFrom((int)i2c_address, (int)len);
    counter++;

    if (counter > 1000)
      return false;

    delay(1);
  }

  for (size_t i = 0; i < len;)
    buf[i++] = i2c->read();

  return true;
}