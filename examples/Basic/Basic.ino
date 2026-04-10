/***************************************************************
 * @file    Basic.ino
 * @brief   Super-simple example for the 7Semi SHT20 sensor.
 *
 * What it does:
 * - Initializes I²C and the SHT20
 * - Prints Temperature (°C) and Humidity (%RH) every second
 *
 * Author  : 7Semi
 * License : MIT
 ***************************************************************/

#include <7Semi_SHT2x.h>

/* ===================== Config ===================== */
#define SHT20_ADDR 0x40
#define I2C_FREQ_HZ 100000UL
#define I2C_SDA_PIN -1  // set pins only on boards that support it
#define I2C_SCL_PIN -1
#define SAMPLE_PERIODMS 1000UL

/* ===================== Globals ===================== */
SHT2x_7Semi sht;
uint32_t lastMs = 0;

/* ===================== Setup ===================== */
/**
 - Begin Serial, I²C, and SHT20
 - Prints a one-time banner or error
*/
void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println(F("\n== 7Semi SHT2x  =="));

  if (!sht.begin(SHT20_ADDR)) {
    // if (!sht.begin(DEFAULT_ADDR, I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ)) {
    Serial.println(F("SHT20 not detected. Check wiring/power."));
    while (1) delay(1000);
  }
  Serial.println(F("Ready. Reading once per second..."));
}

/* ================ Loop ================ */
/**
 - Read temperature and humidity once per second
 - Print values or a simple error
*/
void loop() {
  if (millis() - lastMs < SAMPLE_PERIODMS) return;
  lastMs = millis();

  float t = sht.readTemperature();
  float h = sht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println(F("Read failed (I2C/CRC)."));
    return;
  }

  Serial.print(F("T: "));
  Serial.print(t, 2);
  Serial.print(F(" °C  RH: "));
  Serial.print(h, 1);
  Serial.println(F(" %"));
}
