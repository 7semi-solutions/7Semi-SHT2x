#include <7Semi_SHT2x.h>

SHT2x_7Semi sht;

void setup()
{
  Serial.begin(115200);
  delay(1000);

  if (!sht.begin(0x40))
  {
    Serial.println("SHT2x not found!");
    while (1);
  }

  Serial.println("SHT2x initialized");
}

void loop()
{
  float temperature, humidity;

  if (sht.readTemperature(temperature))
  {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
  }
  else
  {
    Serial.println("Temp read failed");
  }

  if (sht.readHumidity(humidity))
  {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %RH");
  }
  else
  {
    Serial.println("Humidity read failed");
  }

  Serial.println("----------------------");
  delay(500);
}