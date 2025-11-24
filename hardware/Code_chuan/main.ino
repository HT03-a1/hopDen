#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MPU9250.h>

// ---------- Pin definitions ----------
static constexpr int I2C_SDA_PIN = 8;
static constexpr int I2C_SCL_PIN = 9;
static constexpr int BUTTON_CANCEL_PIN = 36;
static constexpr int BUTTON_MANUAL_PIN = 37;
static constexpr int BUZZER_PIN = 45;
static constexpr uint8_t MPU_ADDRESS = 0x69;
static constexpr uint8_t OLED_ADDRESS = 0x3C;

// ---------- Display ----------
static constexpr int OLED_WIDTH = 128;
static constexpr int OLED_HEIGHT = 64;
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// ---------- Sensor ----------
MPU9250 imu(Wire, MPU_ADDRESS);
static constexpr float G_TO_MS2 = 9.80665f;
static constexpr float DEG_TO_RAD = PI / 180.0f;

// ---------- Timing ----------
static constexpr uint32_t SENSOR_INTERVAL_MS = 5;    // 200 Hz
static constexpr uint32_t DISPLAY_INTERVAL_MS = 100; // 10 Hz
static constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
static constexpr uint32_t BUZZER_BEEP_INTERVAL_MS = 1000;
static constexpr uint32_t COUNTDOWN_SEC = 30;
static constexpr uint32_t LOCKOUT_SEC = 10;

// ---------- Thresholds ----------
static constexpr float G_SUSPECT = 2.5f;
static constexpr float G_SEVERE = 4.0f;
static constexpr float ANGLE_SUSPECT = 40.0f;

// ---------- State machine ----------
enum SystemState {
  STATE_NORMAL,
  STATE_SUSPECT_COUNTDOWN,
  STATE_SEND_NOW,
  STATE_MANUAL_SOS,
  STATE_CANCELED
};

SystemState currentState = STATE_NORMAL;
uint32_t lastStateChange = 0;
uint32_t countdownStart = 0;
uint32_t lockoutStart = 0;

// ---------- Measurements ----------
float ax = 0, ay = 0, az = 0;
float gx = 0, gy = 0, gz = 0;
float roll = 0, pitch = 0;
float gTotal = 0;
float lastRoll = 0, lastPitch = 0;
float deltaAngle = 0;
uint32_t lastSensorSample = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t lastBuzzerToggle = 0;

// ---------- Buttons ----------
bool cancelPressed = false;
bool manualPressed = false;
uint32_t lastCancelChange = 0;
uint32_t lastManualChange = 0;
bool cancelState = true;
bool manualState = true;

// ---------- Helper ----------
const char *stateToString(SystemState s) {
  switch (s) {
    case STATE_NORMAL: return "NORMAL";
    case STATE_SUSPECT_COUNTDOWN: return "SUSPECT";
    case STATE_SEND_NOW: return "SOS";
    case STATE_MANUAL_SOS: return "MANUAL";
    case STATE_CANCELED: return "CANCELED";
    default: return "UNKNOWN";
  }
}

void setState(SystemState newState) {
  if (newState == currentState) return;
  currentState = newState;
  lastStateChange = millis();
  Serial.printf("[STATE] -> %s\n", stateToString(currentState));
  switch (currentState) {
    case STATE_SUSPECT_COUNTDOWN:
      countdownStart = millis();
      break;
    case STATE_SEND_NOW:
    case STATE_MANUAL_SOS:
      lockoutStart = millis();
      break;
    case STATE_CANCELED:
      lockoutStart = millis();
      break;
    default:
      break;
  }
}

bool withinLockout() {
  if (lockoutStart == 0) return false;
  return millis() - lockoutStart < LOCKOUT_SEC * 1000UL;
}

void initButtons() {
  pinMode(BUTTON_CANCEL_PIN, INPUT_PULLUP);
  pinMode(BUTTON_MANUAL_PIN, INPUT_PULLUP);
}

void readButtons() {
  bool cancelNow = digitalRead(BUTTON_CANCEL_PIN);
  bool manualNow = digitalRead(BUTTON_MANUAL_PIN);
  uint32_t now = millis();

  if (cancelNow != cancelState && (now - lastCancelChange) > BUTTON_DEBOUNCE_MS) {
    cancelState = cancelNow;
    lastCancelChange = now;
    cancelPressed = (cancelState == LOW);
  } else {
    cancelPressed = false;
  }

  if (manualNow != manualState && (now - lastManualChange) > BUTTON_DEBOUNCE_MS) {
    manualState = manualNow;
    lastManualChange = now;
    manualPressed = (manualState == LOW);
  } else {
    manualPressed = false;
  }
}

void initBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void buzzerOff() {
  digitalWrite(BUZZER_PIN, LOW);
}

void buzzerOn() {
  digitalWrite(BUZZER_PIN, HIGH);
}

void buzzerTick() {
  if (currentState == STATE_SUSPECT_COUNTDOWN) {
    uint32_t now = millis();
    if (now - lastBuzzerToggle >= BUZZER_BEEP_INTERVAL_MS) {
      lastBuzzerToggle = now;
      digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN));
    }
  } else if (currentState == STATE_SEND_NOW || currentState == STATE_MANUAL_SOS) {
    buzzerOn();
  } else {
    buzzerOff();
  }
}

void initOLED() {
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED init failed!");
    while (true) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
}

void drawDisplay(uint32_t now) {
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.printf("Ax:%1.2f Ay:%1.2f", ax, ay);
  oled.setCursor(0, 10);
  oled.printf("Az:%1.2f G:%1.2f", az, gTotal);
  oled.setCursor(0, 20);
  oled.printf("Roll:%2.1f Pitch:%2.1f", roll, pitch);
  oled.setCursor(0, 30);
  oled.printf("State:%s", stateToString(currentState));

  if (currentState == STATE_SUSPECT_COUNTDOWN) {
    uint32_t elapsed = (now - countdownStart) / 1000UL;
    int32_t remain = COUNTDOWN_SEC - elapsed;
    if (remain < 0) remain = 0;
    oled.setCursor(0, 40);
    oled.printf("Countdown:%2lds", remain);
  } else if (withinLockout()) {
    uint32_t elapsed = (now - lockoutStart) / 1000UL;
    int32_t remain = LOCKOUT_SEC - elapsed;
    if (remain < 0) remain = 0;
    oled.setCursor(0, 40);
    oled.printf("Lockout:%2lds", remain);
  }

  oled.display();
}

void initIMU() {
  if (imu.begin() != 0) {
    Serial.println("MPU9250 init failed!");
    while (true) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }

  imu.setAccelRange(MPU9250::ACCEL_RANGE_4G);
  imu.setGyroRange(MPU9250::GYRO_RANGE_1000DPS);
  imu.setDlpfBandwidth(MPU9250::DLPF_BANDWIDTH_41HZ);
  imu.setSrd(4); // approx 200 Hz (1 kHz / (1 + SRD))
}

void updatePose(float dt) {
  // Complementary filter
  float accelRoll = atan2(ay, sqrt(ax * ax + az * az)) * 180.0f / PI;
  float accelPitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0f / PI;

  roll = 0.98f * (roll + gx * dt) + 0.02f * accelRoll;
  pitch = 0.98f * (pitch + gy * dt) + 0.02f * accelPitch;

  float dRoll = roll - lastRoll;
  float dPitch = pitch - lastPitch;
  deltaAngle = sqrt(dRoll * dRoll + dPitch * dPitch);

  lastRoll = roll;
  lastPitch = pitch;
}

void readIMU() {
  if (imu.readSensor() != 0) return;

  ax = imu.getAccelX_mss() / G_TO_MS2;
  ay = imu.getAccelY_mss() / G_TO_MS2;
  az = imu.getAccelZ_mss() / G_TO_MS2;

  gx = imu.getGyroX_rads() / DEG_TO_RAD;
  gy = imu.getGyroY_rads() / DEG_TO_RAD;
  gz = imu.getGyroZ_rads() / DEG_TO_RAD;

  gTotal = sqrt(ax * ax + ay * ay + az * az);
}

void crashDetection() {
  uint32_t now = millis();

  if (manualPressed) {
    setState(STATE_MANUAL_SOS);
    return;
  }

  if (cancelPressed && currentState == STATE_SUSPECT_COUNTDOWN) {
    setState(STATE_CANCELED);
    return;
  }

  if (currentState == STATE_MANUAL_SOS || currentState == STATE_SEND_NOW) {
    // stay until lockout finished
    if (!withinLockout()) {
      setState(STATE_NORMAL);
    }
    return;
  }

  if (currentState == STATE_CANCELED) {
    if (!withinLockout()) {
      setState(STATE_NORMAL);
    }
    return;
  }

  if (currentState == STATE_SUSPECT_COUNTDOWN) {
    uint32_t elapsed = (now - countdownStart) / 1000UL;
    if (elapsed >= COUNTDOWN_SEC) {
      setState(STATE_SEND_NOW);
    }
    return;
  }

  // STATE_NORMAL
  if (withinLockout()) return;

  if (gTotal >= G_SEVERE) {
    setState(STATE_SEND_NOW);
    return;
  }

  if (gTotal >= G_SUSPECT && deltaAngle >= ANGLE_SUSPECT) {
    setState(STATE_SUSPECT_COUNTDOWN);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("ESP32-S3 Crash Detection Init");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000); // 400kHz
  initButtons();
  initBuzzer();
  initOLED();
  initIMU();

  lastSensorSample = millis();
  lastDisplayUpdate = millis();
}

void loop() {
  uint32_t now = millis();

  if (now - lastSensorSample >= SENSOR_INTERVAL_MS) {
    float dt = (now - lastSensorSample) / 1000.0f;
    lastSensorSample = now;
    readIMU();
    updatePose(dt);
  }

  readButtons();
  crashDetection();
  buzzerTick();

  if (now - lastDisplayUpdate >= DISPLAY_INTERVAL_MS) {
    lastDisplayUpdate = now;
    drawDisplay(now);
  }

  vTaskDelay(1); // yield to RTOS
}

