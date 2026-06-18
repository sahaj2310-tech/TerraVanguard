// === Integrated ESP32-S3 Robot Control with Camera Web Server ===
#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <ESP32Servo.h>
#include "DHT.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "driver/ledc.h"
#include "sdkconfig.h"
#include "WiFi.h"
#include "Arduino.h"

// ----------------- Motor Driver Pins ------------------
#define IN1 0   // Motor A
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
#define TRIG1_PIN 3
#define ECHO1_PIN 19

#define TRIG2_PIN 35
#define ECHO2_PIN 36

#define TRIG3_PIN 37
#define ECHO3_PIN 38

// --- Camera Pins ---
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5
#define Y2_GPIO_NUM      11
#define Y3_GPIO_NUM       9
#define Y4_GPIO_NUM       8
#define Y5_GPIO_NUM      10
#define Y6_GPIO_NUM      12
#define Y7_GPIO_NUM      18
#define Y8_GPIO_NUM      17
#define Y9_GPIO_NUM      16
#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM    13

// --- WiFi Credentials ---
// Replace with your own network credentials before flashing.
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// --- Stream Server Globals ---
httpd_handle_t camera_httpd = NULL;

// ----------------- Timing Variables ------------------
unsigned long previousDHTMillis = 0;
const long dhtInterval = 2000;

unsigned long previousMPUMillis = 0;
const long mpuInterval = 100;

unsigned long previousUltrasonicMillis = 0;
const long ultrasonicInterval = 100;

unsigned long previousWatchdogMillis = 0;
const long watchdogInterval = 1000; // Feed watchdog every second

// --- Rolling Average Filter ---
typedef struct {
    size_t size, index, count;
    int sum, *values;
} ra_filter_t;
static ra_filter_t ra_filter;

static ra_filter_t *ra_filter_init(ra_filter_t *filter, size_t sample_size) {
    memset(filter, 0, sizeof(ra_filter_t));
    filter->values = (int *)malloc(sample_size * sizeof(int));
    if (!filter->values) return NULL;
    memset(filter->values, 0, sample_size * sizeof(int));
    filter->size = sample_size;
    return filter;
}

static int ra_filter_run(ra_filter_t *filter, int value) {
    if (!filter->values) return value;
    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += filter->values[filter->index];
    filter->index = (filter->index + 1) % filter->size;
    if (filter->count < filter->size) filter->count++;
    return filter->sum / filter->count;
}

// --- Camera Setup ---
void startCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    if(psramFound()){
      config.frame_size = FRAMESIZE_QVGA;
      config.jpeg_quality = 10;
      config.fb_count = 2;
    } else {
      config.frame_size = FRAMESIZE_QVGA;
      config.jpeg_quality = 12;
      config.fb_count = 1;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
      Serial.printf("Camera init failed with error 0x%x\n", err);
      return;
    }
    Serial.println("Camera initialized successfully");
}

// --- Stream Handler ---
#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t *fb = NULL;
    struct timeval _timestamp;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char part_buf[128];
    static int64_t last_frame = 0;
    if (!last_frame) last_frame = esp_timer_get_time();

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) return res;

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "60");

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera frame buffer null");
            res = ESP_FAIL;
        } else {
            _timestamp.tv_sec = fb->timestamp.tv_sec;
            _timestamp.tv_usec = fb->timestamp.tv_usec;
            if (fb->format != PIXFORMAT_JPEG) {
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                fb = NULL;
                if (!jpeg_converted) res = ESP_FAIL;
            } else {
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
        }
        if (res == ESP_OK) res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if (res == ESP_OK) {
            size_t hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);

        if (fb) esp_camera_fb_return(fb);
        else if (_jpg_buf) free(_jpg_buf);

        if (res != ESP_OK) break;

        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ra_filter_run(&ra_filter, frame_time);
        
        // Yield to prevent watchdog reset during streaming
        yield();
    }
    last_frame = 0;
    return res;
}

// --- HTML Web Page ---
const char index_web[]=R"rawliteral(
<html>
  <head><title>Robot Video Streaming</title></head>
  <body>
    <h1>Robot Camera Stream</h1>
    <p><img id="stream" src="" style="transform:rotate(180deg)"/></p>
    <p><i>Robot streaming with sensor control active.</i></p>
  </body>
  <script>
  document.addEventListener('DOMContentLoaded', function () {
    const view = document.getElementById('stream');
    view.src = 'http://' + window.location.hostname + '/stream';
  });
  </script>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req) {
    esp_err_t err = httpd_resp_set_type(req, "text/html");
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
        err = httpd_resp_send(req, (const char *)index_web, strlen(index_web));
    } else {
        err = httpd_resp_send_500(req);
    }
    return err;
}

void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL
    };

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL
    };

    ra_filter_init(&ra_filter, 20);

    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &stream_uri);
        Serial.println("HTTP server started on port 80");
    } else {
        Serial.println("Failed to start HTTP server");
    }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize WiFi first
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    yield(); // Feed watchdog during WiFi connection
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

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

  // Initialize Camera and Web Server
  startCamera();
  startCameraServer();

  Serial.println("Integrated Robot System Starting...");
  Serial.println("Camera streaming available at: http://" + WiFi.localIP().toString());

  digitalWrite(IN1, 0);
  digitalWrite(IN2, 0);
  digitalWrite(IN3, 0);
  digitalWrite(IN4, 0);
  stopMotors();
}

void loop() {
  unsigned long currentMillis = millis();

  // Handle millis() rollover (approximately every 50 days)
  static unsigned long lastMillis = 0;
  if (currentMillis < lastMillis) {
    // millis() has rolled over, reset all timing variables
    previousDHTMillis = 0;
    previousMPUMillis = 0;
    previousUltrasonicMillis = 0;
    previousWatchdogMillis = 0;
    Serial.println("millis() rollover detected - timing reset");
  }
  lastMillis = currentMillis;

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
    if ((dist1 > 0 && dist1 < 20) || (dist2 > 0 && dist2 < 20) || (dist3 > 0 && dist3 < 20)) {
      Serial.println("Obstacle detected! Turning right.");
      turnRight();
    } 
    
    else if ((dist1 > 0 && dist1 > 30) || (dist2 > 0 && dist2 > 30) || (dist3 > 0 && dist3 > 30)) {
      Serial.println("Path clear. Moving forward.");
      moveForward();
    }

    else 
    stopMotors();
  }

  // Regular watchdog feeding and WiFi check
  if (currentMillis - previousWatchdogMillis >= watchdogInterval) {
    previousWatchdogMillis = currentMillis;
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected! Attempting reconnection...");
      WiFi.begin(ssid, password);
    }
    
    yield(); // Feed the watchdog
  }

  // Additional yield to prevent watchdog reset
  yield();
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