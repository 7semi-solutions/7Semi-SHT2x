# 7Semi SHT2x Arduino Library

Arduino driver for the Sensirion SHT2x series temperature and humidity sensors (SHT20, SHT21, SHT25).

The SHT2x sensors provide accurate **relative humidity and temperature measurements** with low power consumption and digital I²C interface, making them ideal for environmental monitoring applications.

## Features

- Full SHT2x command support

- Temperature measurement (°C)

- Relative humidity measurement (%RH)

- Configurable resolution

- Heater control

- CRC validation for data integrity

- Serial number reading

- Low power operation

---

## Connections / Wiring

The SHT2x communicates using **I²C interface**.

## I²C Connection

| SHT2x Pin | MCU Pin | Notes         |
|----------|--------|--------------|
| VCC      | 3.3V   | **3.3V only** |
| GND      | GND    | Common ground |
| SDA      | SDA    | I²C data      |
| SCL      | SCL    | I²C clock     |

---

### I²C Notes

- Default I²C address: `0x40`

- Supported bus speeds:
  - 100 kHz
  - 400 kHz (recommended)

---

## Installation

- Arduino Library Manager

  1. Open Arduino IDE  
  2. Go to Library Manager  
  3. Search for **7Semi SHT2x**  
  4. Click Install  

- Manual Installation

  1. Download this repository as ZIP  
  2. Arduino IDE → Sketch → Include Library → Add .ZIP Library  

---

## Library Overview

### Reading Temperature

```cpp
float temperature;

sensor.readTemperature(temperature);
```

- Returns temperature in °C

## Combined Reading

```cpp
float temperature, humidity;

sensor.readTempHum(temperature, humidity);
```

- Reads both temperature and humidity

## Reads both temperature and humidity

The SHT2x allows adjusting measurement resolution:

```cpp
sensor.setResolution(SHT2x_RES_12_14);
```

## Heater Control

Enable or disable internal heater:

```cpp
sensor.setHeater(true);
```

- Useful for condensation removal

## Reading Serial Number

```cpp
uint64_t serial;

sensor.readSerialNumber(serial);
```
- Returns unique device ID

## Soft Reset

```cpp
sensor.softReset();
```

- Resets sensor to default state
