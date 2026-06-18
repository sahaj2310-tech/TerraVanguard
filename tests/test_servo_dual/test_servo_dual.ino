#include <Arduino.h>
#include <ESP32Servo.h>

Servo servo1;
Servo servo2;

const int SERVO1_PIN = 39;
const int SERVO2_PIN = 40;

void setup() {
  Serial.begin(115200);
  Serial.println("Custom Servo Pin Test Starting...");

  servo1.setPeriodHertz(50);
  servo1.attach(SERVO1_PIN, 500, 2500);

  servo2.setPeriodHertz(50);
  servo2.attach(SERVO2_PIN, 500, 2500);

  delay(1000);
}

void loop() {
  for (int angle = 0; angle <= 180; angle += 5) {
    servo1.write(angle);
    servo2.write(180 - angle);
    delay(30);
  }

  for (int angle = 180; angle >= 0; angle -= 5) {
    servo1.write(angle);
    servo2.write(180 - angle);
    delay(30);
  }

  servo1.write(90);
  servo2.write(90);
  delay(1000);
}
