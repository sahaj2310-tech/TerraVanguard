#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <ESP32Servo.h>

MPU9250_asukiaaa mySensor;
Servo servoX;
Servo servoY;

const int servoXPin = 39;
const int servoYPin = 40;

float angleX = 90;
float angleY = 90;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Start I2C with custom pins for ESP32-CAM
  Wire.begin(2, 1);
  mySensor.setWire(&Wire);

  mySensor.beginAccel();
  mySensor.beginGyro();
  mySensor.beginMag();

  servoX.attach(servoXPin);
  servoY.attach(servoYPin);

  // Initialize servos at 90 degrees
  servoX.write(90);
  servoY.write(90);

  Serial.println("MPU9250 + Servo initialized.");
}

void loop() {
  mySensor.gyroUpdate();  // Use this instead of update()

  float gyroX = mySensor.gyroX();
  float gyroY = mySensor.gyroY();

  // Smooth output with simple low-pass filter
  float targetX = map(constrain(gyroX, -90, 90), -90, 90, 0, 180);
  float targetY = map(constrain(gyroY, -90, 90), -90, 90, 0, 95);

  angleX = 0.9 * angleX + 0.1 * targetX;
  angleY = 0.9 * angleY + 0.1 * targetY;

  servoX.write((int)angleX);
  servoY.write((int)angleY);

  Serial.print("GyroX: ");
  Serial.print(gyroX);
  Serial.print(" | ServoX: ");
  Serial.print((int)angleX);
  Serial.print(" | GyroY: ");
  Serial.print(gyroY);
  Serial.print(" | ServoY: ");
  Serial.println((int)angleY);

  delay(100);  // 10Hz update
}
