#include <7Semi_SHT2x.h>

SHT2x_7Semi sht;

void setup()
{
  Serial.begin(115200);

  if (!sht.begin())
  {
    Serial.println("SHT2x not found!");
    while (1);
  }
}

void loop()
{
  float temperature, humidity;

  // ---- Temperature ----
  sht.startTemperatureMeasurement();
  delay(sht.getMeasurementTime(TEMPERATURE));

  if (sht.readTemperatureMeasurement(temperature))
  {
    Serial.print("Temp: ");
    Serial.print(temperature);
    Serial.print(" °C");
  }
  else
  {
    Serial.println("Temp read failed");
  }

  // ---- Humidity ----
  sht.startHumidityMeasurement();
  delay(sht.getMeasurementTime(HUMIDITY));

  if (sht.readHumidityMeasurement(humidity))
  {
    Serial.print("  | Humidity: ");
    Serial.print(humidity);
    Serial.println(" %RH");
  }
  else
  {
    Serial.println("Humidity read failed");
  }
  delay(500);
}