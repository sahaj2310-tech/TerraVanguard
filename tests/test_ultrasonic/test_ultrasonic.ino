#define TRIG1_PIN 3
#define ECHO1_PIN 19

#define TRIG2_PIN 35
#define ECHO2_PIN 36

#define TRIG3_PIN 37
#define ECHO3_PIN 38

long readDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // timeout 30ms
  long distance = duration * 0.034 / 2;
  return distance;
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG1_PIN, OUTPUT);
  pinMode(ECHO1_PIN, INPUT);

  pinMode(TRIG2_PIN, OUTPUT);
  pinMode(ECHO2_PIN, INPUT);

  pinMode(TRIG3_PIN, OUTPUT);
  pinMode(ECHO3_PIN, INPUT);
}

void loop() {
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

  delay(500);
}
