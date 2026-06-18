#include <MPU9250_asukiaaa.h>
#include <Wire.h>

MPU9250_asukiaaa mySensor;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Set up Wire on custom pins for ESP32-S3
  Wire.begin(2, 1); // SDA = GPIO2, SCL = GPIO1

  mySensor.setWire(&Wire);
  mySensor.beginAccel();
  mySensor.beginGyro();
  mySensor.beginMag();

  Serial.println("MPU9250 initialized");
}

void loop() {
  mySensor.accelUpdate();
  mySensor.gyroUpdate();
  mySensor.magUpdate();

  Serial.print("Accel X: "); Serial.print(mySensor.accelX());
  Serial.print(" Y: "); Serial.print(mySensor.accelY());
  Serial.print(" Z: "); Serial.println(mySensor.accelZ());

  Serial.print("Gyro X: "); Serial.print(mySensor.gyroX());
  Serial.print(" Y: "); Serial.print(mySensor.gyroY());
  Serial.print(" Z: "); Serial.println(mySensor.gyroZ());

  Serial.print("Mag X: "); Serial.print(mySensor.magX());
  Serial.print(" Y: "); Serial.print(mySensor.magY());
  Serial.print(" Z: "); Serial.println(mySensor.magZ());

  delay(500);
}
