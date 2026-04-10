/***************************************************************
 * @file    Config.ino
 * @brief   Example for using the 7Semi SHT2x temperature and
 *          humidity sensor over I²C with the 7Semi driver.
 *
 * Features demonstrated:
 * - Sensor init with optional custom SDA/SCL and I²C clock
 * - Periodic reading of Temperature (°C) and Humidity (%RH)
 * - Reading battery status flag and serial number (hex)
 * - Changing resolution and toggling heater via serial menu
 *
 * Sensor configuration used:
 * - I²C Frequency : 100 kHz (bring-up recommended)
 * - Address       : 0x40 (DEFAULT_ADDR)
 *
 * Connections (typical):
 * - SDA -> Board SDA (or custom pin if supported)
 * - SCL -> Board SCL (or custom pin if supported)
 * - VIN -> 3.3V / 5V (module dependent)
 * - GND -> GND
 *
 * Author   : 7Semi
 * License  : MIT
 * Version  : 1.0
 * Date     : 28 October 2025
 ***************************************************************/

#include <7Semi_SHT2x.h>

/* ===================== User configuration ===================== */
#define I2C_ADDR      0x40
#define I2C_FREQ_HZ   100000UL
#define I2C_SDA_PIN   -1
#define I2C_SCL_PIN   -1
// Uncomment and modify if using custom pins
// #define I2C_SDA_PIN   21   // I²C data pin
// #define I2C_SCL_PIN   22   // I²C clock pin
#define READ_PERIOD_MS 1000UL

/* ===================== Globals ===================== */
SHT2x_7Semi sht;
uint32_t lastRead = 0;
bool heaterOn = false;
SHT2x_Resolution curRes = SHT2x_Resolution::RH12_T14;

/* ===================== Helpers ===================== */
/**
 - Print one-line measurement
 - tC : temperature in °C
 - rh : relative humidity in %
*/
static void printMeasurement(float tC, float rh) {
  Serial.print(F("T: "));
  Serial.print(tC, 2);
  Serial.print(F(" °C  RH: "));
  Serial.print(rh, 1);
  Serial.println(F(" %"));
}

/**
 - Print current resolution enum in human text
*/
static void printResolution(SHT2x_Resolution r) {
  Serial.print(F("Resolution: "));
  switch (r) {
    case SHT2x_Resolution::RH12_T14: Serial.println(F("RH12 / T14")); break;
    case SHT2x_Resolution::RH8_T12 : Serial.println(F("RH8  / T12")); break;
    case SHT2x_Resolution::RH10_T13: Serial.println(F("RH10 / T13")); break;
    case SHT2x_Resolution::RH11_T11: Serial.println(F("RH11 / T11")); break;
  }
}

/**
 - Cycle to next resolution (enum order)
 - Measurement resolution control
 -  RH8_T12    RH 12-bit / T 14-bit
 -  RH10_T13   RH  8-bit / T 12-bit
 -  RH11_T11   RH 10-bit / T 13-bit
 -  RH12_T14   RH 11-bit / T 11-bit
 - return: next enum
*/
static SHT2x_Resolution nextResolution(SHT2x_Resolution r) {
  switch (r) {
    case SHT2x_Resolution::RH12_T14: return SHT2x_Resolution::RH8_T12;
    case SHT2x_Resolution::RH8_T12 : return SHT2x_Resolution::RH10_T13;
    case SHT2x_Resolution::RH10_T13: return SHT2x_Resolution::RH11_T11;
    default:                         return SHT2x_Resolution::RH12_T14;
  }
}

/**
 - Print a compact serial menu
*/
static void printMenu() {
  Serial.println(F("\n[Menu] r:Res cycle  h:Heater toggle  b:Battery  s:Serial  m:Menu\n"));
}

/* ===================== Setup ===================== */
void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println(F("\n== 7Semi SHT2x Example =="));

  if (!sht.begin(I2C_ADDR, I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ)) {
    Serial.println(F("SHT2x not detected (custom begin). Check wiring/power."));
    while (1) delay(1000);
  }

  curRes = sht.getResolution();
  printResolution(curRes);

  bool low = sht.batteryLow();
  Serial.print(F("Battery flag (bit6): "));
  Serial.println(low ? F("LOW (<2.25V)") : F("OK"));

  String sn = sht.readSerialNumberHex();
  if (sn.length() == 16) {
    Serial.print(F("Serial: 0x"));
    Serial.println(sn);
  } else {
    Serial.println(F("Serial read failed."));
  }

  printMenu();
}

/* ================ Loop ================ */
/**
 - Periodically read T/RH
 - Handle serial commands for resolution, heater, battery, serial
*/
void loop() {
  if (Serial.available()) {
    char c = (char)Serial.read();

    if (c == 'r' || c == 'R') {
      SHT2x_Resolution n = nextResolution(curRes);
      if (sht.setResolution(n)) {
        curRes = n;
        printResolution(curRes);
      } else {
        Serial.println(F("Resolution set failed."));
      }
    } else if (c == 'h' || c == 'H') {
      heaterOn = !heaterOn;
      if (sht.enableHeater(heaterOn)) {
        Serial.print(F("Heater: "));
        Serial.println(heaterOn ? F("ON") : F("OFF"));
      } else {
        Serial.println(F("Heater toggle failed."));
      }
    } else if (c == 'b' || c == 'B') {
      bool low = sht.batteryLow();
      Serial.print(F("Battery flag: "));
      Serial.println(low ? F("LOW (<2.25V)") : F("OK"));
    } else if (c == 's' || c == 'S') {
      String sn = sht.readSerialNumberHex();
      if (sn.length() == 16) {
        Serial.print(F("Serial: 0x"));
        Serial.println(sn);
      } else {
        Serial.println(F("Serial read failed."));
      }
    } else if (c == 'm' || c == 'M') {
      printMenu();
    }
  }

  if (millis() - lastRead >= READ_PERIOD_MS) {
    lastRead = millis();

    float t = sht.readTemperature();
    float h = sht.readHumidity();

    if (isnan(t) || isnan(h)) {
      Serial.println(F("Read failed (I2C/CRC)."));
    } else {
      printMeasurement(t, h);
    }
  }
}
