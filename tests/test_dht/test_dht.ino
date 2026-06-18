#include "DHT.h"
//"DHT sensor Library" by adafruit
#define DHTPIN 21        // DHT11 data pin connected to GPIO21
#define DHTTYPE DHT11    // Define sensor type

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("DHT11 Sensor Test on GPIO21");
  dht.begin();
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  delay(2000);  // Wait 2 seconds between readings
}
