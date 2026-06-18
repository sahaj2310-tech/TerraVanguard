#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <ESP32Servo.h>
#include "DHT.h"

// ----------------- Motor Driver Pins ------------------
#define IN1 14   // Motor A
#define IN2 45  // Motor A
#define IN3 48  // Motor B
#define IN4 47  // Motor B

// ----------------- DHT11 Sensor ------------------
#define DHTPIN 21
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ----------------- MPU9250 ------------------
MPU9250_asukiaaa mySensor;
float angleX = 90;
float angleY = 90;

// ----------------- Servo Pins ------------------
Servo servoX;
Servo servoY;
const int servoXPin = 39;
const int servoYPin = 40;

// ----------------- Ultrasonic Sensor Pins ------------------
// Updated to 3 sensors:
#define TRIG1_PIN 3
#define ECHO1_PIN 19

#define TRIG2_PIN 35
#define ECHO2_PIN 36

#define TRIG3_PIN 37
#define ECHO3_PIN 38

// ----------------- Timing Variables ------------------
unsigned long previousDHTMillis = 0;
const long dhtInterval = 100;

unsigned long previousMPUMillis = 0;
const long mpuInterval = 100;

unsigned long previousUltrasonicMillis = 0;
const long ultrasonicInterval = 100;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize Motor Driver Pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Initialize DHT11 Sensor
  dht.begin();

  // Initialize MPU9250
  Wire.begin(2, 1);
  Wire.setClock(400000);
  mySensor.setWire(&Wire);
  mySensor.beginAccel();
  mySensor.beginGyro();
  mySensor.beginMag();

  // Initialize Servos
  servoX.attach(servoXPin);
  servoY.attach(servoYPin);
  servoX.write(90);
  servoY.write(90);

  // Initialize Ultrasonic Sensor Pins
  pinMode(TRIG1_PIN, OUTPUT);
  pinMode(ECHO1_PIN, INPUT);

  pinMode(TRIG2_PIN, OUTPUT);
  pinMode(ECHO2_PIN, INPUT);

  pinMode(TRIG3_PIN, OUTPUT);
  pinMode(ECHO3_PIN, INPUT);

  Serial.println("L298N Motor + Sensors Starting...");
}

void loop() {
  unsigned long currentMillis = millis();

  // DHT11 Sensor Reading
  if (currentMillis - previousDHTMillis >= dhtInterval) {
    previousDHTMillis = currentMillis;
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.print(" %\t Temperature: ");
      Serial.print(temperature);
      Serial.println(" °C");
    }
  }

  // MPU9250 Reading and Servo Control
  if (currentMillis - previousMPUMillis >= mpuInterval) {
    previousMPUMillis = currentMillis;
    mySensor.gyroUpdate();

    float gyroX = mySensor.gyroX();
    float gyroY = mySensor.gyroY();

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
  }

  // Ultrasonic Sensor Reading and Motor Control (with 3 sensors)
  if (currentMillis - previousUltrasonicMillis >= ultrasonicInterval) {
    previousUltrasonicMillis = currentMillis;

    long dist1 = readDistanceCM(TRIG1_PIN, ECHO1_PIN);
    long dist2 = readDistanceCM(TRIG2_PIN, ECHO2_PIN);
    long dist3 = readDistanceCM(TRIG3_PIN, ECHO3_PIN);

    Serial.print("Distance 1: ");
    Serial.print(dist1);
    Serial.print(" cm | ");

    Serial.print("Distance 2: ");
    Serial.print(dist2);
    Serial.print(" cm | ");

    Serial.print("Distance 3: ");
    Serial.print(dist3);
    Serial.println(" cm");

    // Use obstacle algorithm based on any sensor detecting an obstacle < 20cm
    if ((dist1 > 0 && dist1 < 60) || (dist2 > 0 && dist2 < 80) || (dist3 > 0 && dist3 < 60)) {
      Serial.println("Obstacle detected! Turning right.");
      turnRight();
    } else {
      Serial.println("Path clear. Moving forward.");
      moveForward();
    }
  }

  yield(); // Feed the watchdog
}

// Function to read distance from ultrasonic sensor
long readDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout
  long distance = duration * 0.034 / 2;
  return distance;
}

// ----------------- Motor Control Functions ------------------

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnLeft() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
