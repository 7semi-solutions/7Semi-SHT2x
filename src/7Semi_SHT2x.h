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

#ifndef _7SEMI_SHT2x_H_
#define _7SEMI_SHT2x_H_

#pragma once
#include <Arduino.h>
#include <Wire.h>

#define DEFAULT_ADDR            0x40
#define SHT2x_TRIGGER_TEMP_HOLD 0xE3
#define SHT2x_TRIGGER_HUMI_HOLD 0xE5
#define SHT2x_TRIGGER_TEMP      0xF3
#define SHT2x_TRIGGER_HUMI      0xF5
#define SHT2x_USER_WRITE        0xE6
#define SHT2x_USER_READ         0xE7
#define SHT2x_SOFT_RESET        0xFE

enum class SHT2x_Resolution : uint8_t
{
  RH12_T14 = 0x00, // 12-bit RH / 14-bit T
  RH8_T12  = 0x01, // 8-bit RH  / 12-bit T
  RH10_T13 = 0x80, // 10-bit RH / 13-bit T
  RH11_T11 = 0x81  // 11-bit RH / 11-bit T
};

enum SHT2x_SensorType : uint8_t
{
  TEMPERATURE = 0x00,    // status bits 00
  HUMIDITY    = 0x02     // status bits 10
};

class SHT2x_7Semi
{
public:

  SHT2x_7Semi();

  /**
   * Initialize SHT2x sensor over I²C
   *
   * - Sets I²C address and assigns Wire instance
   * - Starts I²C bus and configures clock speed
   * - Verifies sensor presence using ACK check
   * - Sets default resolution to RH12 / T14
   *
   * - i2cAddress : Sensor I²C address (default 0x40)
   * - i2cPort    : Reference to TwoWire instance (Wire, Wire1, etc.)
   * - i2cSpeed   : I²C clock frequency (e.g., 100000, 400000)
   *
   * - Returns:
   *   - true  → Sensor detected and initialized successfully
   *   - false → I²C communication failed or sensor not responding
   */
  bool begin(uint8_t i2cAddress = 0x40, TwoWire &i2cPort = Wire, uint32_t i2cSpeed = 400000);

  /**
   * Start temperature measurement (NO HOLD mode)
   *
   * - Sends temperature trigger command to sensor
   * - Sensor begins internal conversion (non-blocking)
   * - Must wait before calling readMeasurement()
   * - Max conversion time ≈ 85 ms (14-bit resolution)
   *
   * - Returns:
   *   - true  → Command sent successfully (ACK received)
   *   - false → I²C communication failed
   */
  bool startTemperatureMeasurement();

  /**
   * Start humidity measurement (NO HOLD mode)
   *
   * - Sends humidity trigger command to sensor
   * - Sensor begins internal conversion (non-blocking)
   * - Must wait or poll before calling readMeasurement()
   * - Max conversion time ≈ 29 ms (12-bit resolution)
   *
   * - Returns:
   *   - true  → Command sent successfully (ACK received)
   *   - false → I²C communication failed
   */
  bool startHumidityMeasurement();

  /**
   * Read raw measurement (temperature or humidity)
   *
   * - Reads 3 bytes from sensor (MSB, LSB, CRC)
   * - Combines MSB/LSB into 16-bit raw value
   * - Verifies data integrity using CRC check
   * - Must be called after measurement is ready
   *
   * - Status bits (bit[1:0] of raw value):
   *   - 00 → Temperature measurement
   *   - 10 → Humidity measurement
   *   - These bits are NOT part of sensor data
   *
   * - raw_measurement : Output raw sensor value (includes status bits)
   *
   * - Returns:
   *   - true  → Data read and CRC valid
   *   - false → I²C error or CRC mismatch
   *
   * - Notes:
   *   - Clear status bits before conversion using:
   *     raw_measurement &= ~0x0003;
   */
  bool readMeasurement(uint16_t &raw_measurement);

  /**
   * Convert raw value to temperature in °C
   *
   * - Raw value should have status bits cleared before use
   * - raw : 16-bit raw temperature value
   *
   * - Returns:
   *   - Temperature in degrees Celsius
   */
  inline float rawToTemperature(uint16_t raw) const;

  /**
   * Convert raw value to relative humidity in %RH
   *
   * - raw : 16-bit raw humidity value
   * - Raw value should have status bits cleared before use
   *
   * - Returns:
   *   - Relative humidity in percent (%RH)
   */
  inline float rawToHumidity(uint16_t raw) const;

  /**
   * Read temperature measurement (NO HOLD mode)
   *
   * - Reads raw measurement and converts to °C
   * - Ensure measurement is ready before calling
   *
   * - temperature : Output temperature °C
   *
   * - Returns:
   *   - true  → Successful read and conversion
   *   - false → Read or CRC failure
   */
  bool readTemperatureMeasurement(float &temperature);

  /**
   * Read humidity measurement (NO HOLD mode)
   *
   * - Reads raw measurement and converts to %RH
   * - Ensure measurement is ready before calling
   *
   * - humidity : Output humidity value
   *
   * - Returns:
   *   - true  → Successful read and conversion
   *   - false → Read or CRC failure
   */
  bool readHumidityMeasurement(float &humidity);

  /**
   * Read raw temperature (HOLD mode)
   *
   * - Sends temperature command and waits until data is ready
   * - Reads raw value and validates using CRC
   *
   * - temperatureRaw : Output raw temperature value
   *
   * - Returns:
   *   - true  → Successful read and valid CRC
   *   - false → I²C or CRC failure
   */
  bool readTemperatureRaw(uint16_t &temperatureRaw);

  /**
   * Read temperature in °C (HOLD mode)
   *
   * - Reads raw temperature and converts to °C
   *
   * - temperatureC : Output temperature value
   *
   * - Returns:
   *   - true  → Successful read
   *   - false → Read or CRC failure
   */
  bool readTemperature(float &temperatureC);

  /**
   * Read raw humidity (HOLD mode)
   *
   * - Sends humidity command and waits until data is ready
   * - Reads raw value and validates using CRC
   *
   * - HumidityRaw : Output raw humidity value
   *
   * - Returns:
   *   - true  → Successful read and valid CRC
   *   - false → I²C or CRC failure
   */
  bool readHumidityRaw(uint16_t &HumidityRaw);

  /**
   * Read relative humidity in %RH (HOLD mode)
   *
   * - Reads raw humidity and converts to %RH
   *
   * - humidity : Output humidity value
   *
   * - Returns:
   *   - true  → Successful read
   *   - false → Read or CRC failure
   */
  bool readHumidity(float &humidity);

  /**
   * Read user register
   *
   * - Reads 8-bit user register from sensor
   *
   * - data : Output register value
   *
   * - Returns:
   *   - true  → Read successful
   *   - false → I²C communication failed
   */
  bool readUserRegister(uint8_t &data);

  /**
   * Write user register
   *
   * - Writes 8-bit value to user register
   *
   * - data : Register value to write
   *
   * - Returns:
   *   - true  → Write successful (ACK received)
   *   - false → I²C communication failed
   */
  bool writeUserRegister(uint8_t data);

  /**
   * Set measurement resolution
   *
   * - Updates resolution bits (bit7 and bit0) in user register
   * - Other bits remain unchanged
   *
   * - res : Resolution enum (RH/T combination)
   *
   * - Returns:
   *   - true  → Resolution updated successfully
   *   - false → Read/write operation failed
   *
   * - Notes:
   *   - Updates internal resolution cache
   */
  bool setResolution(SHT2x_Resolution res);

  /**
   * Get measurement resolution
   *
   * - Reads resolution bits from user register
   *
   * - res : Output resolution enum
   *
   * - Returns:
   *   - true  → Read successful
   *   - false → I²C read failed
   */
  bool getResolution(SHT2x_Resolution &res);

  /**
   * Get battery status
   *
   * - Reads battery flag (bit6) from user register
   *
   * - batteryLow : true if VDD < 2.25V
   *
   * - Returns:
   *   - true  → Read successful
   *   - false → I²C read failed
   */
  bool getBatteryStatus(bool &batteryLow);

  /**
   * Enable or disable heater
   *
   * - Controls heater bit (bit2) in user register
   *
   * - enable : true = heater ON, false = OFF
   *
   * - Returns:
   *   - true  → Write successful
   *   - false → I²C operation failed
   */
  bool setHeater(bool enable);

  /**
   * Get heater status
   *
   * - Reads heater state (bit2)
   *
   * - enable : true if heater is ON
   *
   * - Returns:
   *   - true  → Read successful
   *   - false → I²C read failed
   */
  bool getHeater(bool &enable);

  /**
   * Configure OTP reload behavior
   *
   * - Controls OTP reload disable bit (bit1)
   *
   * - enable : true  → OTP reload enabled (bit = 0)
   *            false → OTP reload disabled (bit = 1)
   *
   * - Returns:
   *   - true  → Write successful
   *   - false → I²C operation failed
   */
  bool setOtpReload(bool enable);

  /**
   * Get OTP reload status
   *
   * - Reads OTP reload state (bit1)
   *
   * - enabled : true if OTP reload is enabled
   *
   * - Returns:
   *   - true  → Read successful
   *   - false → I²C read failed
   */
  bool getOtpReload(bool &enable);

  /**
   * Perform soft reset
   *
   * - Resets sensor to default state
   * - Clears user register settings
   *
   * - Returns:
   *   - true  → Command sent successfully
   *   - false → I²C communication failed
   */
  bool softReset();

  /**
   * Get measurement delay for given resolution
   *
   * - Returns maximum conversion time (safe delay)
   *
   * - resolution : Measurement resolution setting
   * - type       : Sensor type (Temperature / Humidity)
   *
   * - Returns:
   *   - Delay in milliseconds
   */
  uint16_t getMeasurementTime(SHT2x_Resolution resolution, SHT2x_SensorType type);

  /**
   * Get measurement delay using current resolution
   *
   * - Uses internally stored resolution value
   *
   * - type : Sensor type (Temperature / Humidity)
   *
   * - Returns:
   *   - Delay in milliseconds
   */
  uint16_t getMeasurementTime(SHT2x_SensorType type);

  /**
   * Read 64-bit serial number
   *
   * - Reads sensor unique ID in two parts
   *
   * - sn : Output 64-bit serial number
   *
   * - Returns:
   *   - true  → Read successful and CRC valid
   *   - false → I²C or CRC failure
   */
  bool readSerialNumber(uint64_t &sn);

  /**
   * - Read serial number as hex (16 chars)
   * - return:
   *   - empty string on failure
   */
  String readSerialNumberHex();

private:
  TwoWire *i2c;
  uint8_t i2c_address = DEFAULT_ADDR;
  SHT2x_Resolution resolution;

  /**
   * Verify CRC for 16-bit measurement
   *
   * - Uses Sensirion CRC-8 (poly = 0x31, init = 0x00)
   *
   * - data : 16-bit raw measurement
   * - crc  : Received CRC byte
   *
   * - Returns:
   *   - true  → CRC matches
   *   - false → CRC mismatch
   *
   * - Notes:
   *   - CRC must be checked before clearing status bits
   */
  bool checkCRC(uint16_t data, uint8_t crc);

  bool sendCommand(uint8_t cmd);

  bool writeReg(uint8_t reg, uint8_t value);

  bool readData(uint8_t *buf, size_t len);

  bool readReg(uint8_t reg, uint8_t &value);

  bool burstRead(uint8_t reg, uint8_t *buf, size_t len);

  bool crc8_ok(const uint8_t *data, uint8_t nbytes, uint8_t checksum);
};

#endif
