/*
 * HỘP ĐEN Ô TÔ / XE MÁY - ESP32-S3 (VERSION 2)
 * 
 * MCU: ESP32-S3 (dual-core)
 * Framework: Arduino + FreeRTOS
 * 
 * PHÂN CHIA CORE:
 * - Core 0: SD Card (ghi log) - TẠM THỜI BỎ GHI ÂM
 * - Core 1: Sensors + Logic (MPU, DHT11, GPS, DS1307, SIM 4G, nút, buzzer, OLED)
 * 
 * CHỨC NĂNG CẬP NHẬT:
 * 1. Ghi nhật ký liên tục mỗi 30 giây (GPS, MPU, DHT11, RTC, trạng thái SOS từ phần cứng)
 * 2. [TẠM THỜI BỎ] Ghi âm vòng tròn 50 file, mỗi file 2 phút, tên file theo thời gian kết thúc
 * 3. Phát hiện va chạm/tai nạn từ MPU9250 với đếm ngược 30s + buzzer bíp
 * 4. SOS cứu hộ thủ công (nhấn giữ 5s, log vào nhật ký, hiển thị trên OLED)
 * 5. Gửi ID + vị trí + trạng thái SOS + ghi chú tự động lên server
 * 6. Đồng bộ 2 chiều với server (server hủy → box hủy, trạm hoàn thành → box hủy)
 * 7. [ĐÃ XÓA] Webserver thiết lập - không còn chức năng này
 * 8. Cảnh báo nhiệt độ/độ ẩm vượt ngưỡng
 */

#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <DHT.h>
#include <RTClib.h>
#include <TinyGPS++.h>
#include <U8g2lib.h>
// #include <driver/i2s.h>  // Tạm thời bỏ ghi âm
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <ArduinoJson.h>
// #include <WiFi.h>  // BỎ WEBSERVER - KHÔNG CẦN WIFI
// #include <WebServer.h>  // BỎ WEBSERVER
#include <EEPROM.h>
#include <string.h>  // Cho strlen, memset
// TinyGSM for SIM 4G
#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

// ============================================
// ĐỊNH NGHĨA CHÂN GPIO (THEO SƠ ĐỒ MỚI)
// ============================================

// I2S Audio (INMP441) - TẠM THỜI BỎ GHI ÂM
// #define I2S_WS   42   // LRCL / WS
// #define I2S_SD   40   // DOUT from INMP441
// #define I2S_SCK  41   // BCLK / SCK

// SD Card (SPI)
#define SD_MOSI  11
#define SD_MISO  13
#define SD_SCK   12
#define SD_CS    10

// I2C (MPU9250, DS1307, OLED)
#define I2C_SDA_PIN     8
#define I2C_SCL_PIN     9

// MPU9250 (I2C, address 0x69)
// DS1307 (I2C, address 0x50 hoặc 0x68)
// OLED SSD1306 (I2C, address 0x3C)

// DHT11
#define DHT_PIN         35
#define DHT_TYPE        DHT11

// GPS NEO-8M (UART)
#define GPS_RX_PIN      7
#define GPS_TX_PIN      6

// SIM 4G (UART)
#define SIM_RX_PIN      18
#define SIM_TX_PIN      17

// Nút nhấn
#define BUTTON_1_PIN    36  // Nút SOS tai nạn
#define BUTTON_2_PIN    37  // Nút SOS cứu hộ

// Buzzer
#define BUZZER_PIN      15

// ============================================
// THAM SỐ HỆ THỐNG
// ============================================

// Audio parameters - TẠM THỜI BỎ GHI ÂM
// #define AUDIO_SAMPLE_RATE       16000
// #define AUDIO_BITS_PER_SAMPLE   16
// #define AUDIO_CHANNELS          1
// #define AUDIO_FILE_DURATION_SEC 120  // 2 phút
// #define AUDIO_MAX_FILES         50   // 50 file
// #define AUDIO_BUFFER_SIZE       1024

#define LOG_INTERVAL_MS         30000  // Ghi log mỗi 30 giây
#define OLED_UPDATE_MS          100    // Cập nhật OLED mỗi 100ms
#define TELEMETRY_INTERVAL_MS   30000  // Gửi telemetry mỗi 30 giây
#define SOS_SYNC_INTERVAL_MS    5000   // Đồng bộ SOS với server mỗi 5 giây

#define ACCIDENT_COUNTDOWN_SEC  30     // Đếm ngược 30 giây
#define BUTTON_HOLD_MS          5000   // Nhấn giữ 5 giây
#define BUTTON_DEBOUNCE_MS      50     // Thời gian debounce (ms)
// #define BUTTON_BOTH_HOLD_MS     10000  // BỎ WEBSERVER - Nhấn đồng thời 2 nút 10 giây

// Ngưỡng mặc định
#define DEFAULT_ACCIDENT_THRESHOLD_G      2.5f    // Gia tốc tuyến tính: 2.5g
#define DEFAULT_ANGULAR_ACCEL_THRESHOLD   300.0f  // Gia tốc góc: 300 deg/s² (khoảng 5.24 rad/s²)
#define DEFAULT_MIN_SPEED_FOR_ACCIDENT    10.0f   // Tốc độ tối thiểu: 10 km/h
#define DEFAULT_TEMP_THRESHOLD            50.0f   // Nhiệt độ: 50°C
#define DEFAULT_HUMI_THRESHOLD            90.0f   // Độ ẩm: 90%

// ID tài khoản và thiết bị mặc định
#define DEFAULT_USER_ID      "U0001"   // ID tài khoản
#define DEFAULT_DEVICE_ID    "DHW001"  // ID thiết bị

// MPU9250 I2C Address
#define MPU9250_ADDRESS 0x69

// MPU9250 Registers
#define MPU9250_WHO_AM_I      0x75
#define MPU9250_PWR_MGMT_1    0x6B
#define MPU9250_ACCEL_CONFIG  0x1C
#define MPU9250_GYRO_CONFIG   0x1B
#define MPU9250_CONFIG        0x1A
#define MPU9250_SMPLRT_DIV    0x19
#define MPU9250_ACCEL_XOUT_H  0x3B

// ============================================
// CẤU TRÚC DỮ LIỆU
// ============================================

struct SensorData {
  float lat, lon, speed;
  float ax, ay, az;
  float gx, gy, gz;
  float temp, humi;
  DateTime rtcTime;
  bool valid;
};

enum SystemStatus {
  STATUS_NORMAL,
  STATUS_ACCIDENT_COUNTDOWN,
  STATUS_ACCIDENT_SOS_SENT,
  STATUS_RESCUE_COUNTDOWN,
  STATUS_RESCUE_SOS_SENT,
  STATUS_SOS_CANCELED
};

enum SOSSource {
  SOS_SOURCE_NONE,
  SOS_SOURCE_HARDWARE_ACCIDENT,
  SOS_SOURCE_HARDWARE_RESCUE
};

struct SystemState {
  SystemStatus status;
  unsigned long countdownStart;
  int countdownRemaining;
  bool buzzerOn;
  SOSSource currentSOSSource;
  String currentSOSId;  // ID SOS từ server (nếu có)
  SensorData lastSensorData;
  char logFileName[32];
  // Audio variables - TẠM THỜI BỎ GHI ÂM
  // int audioFileIndex;
  // unsigned long audioFileStartTime;
  // bool audioRecording;
};

// Cấu hình thiết bị (lưu trong EEPROM)
struct DeviceConfig {
  char deviceId[32];
  char userId[32];
  float accidentThreshold;      // Ngưỡng gia tốc tuyến tính (g) - mặc định: 2.5g
  float angularAccelThreshold;  // Ngưỡng gia tốc góc (deg/s²) - mặc định: 300 deg/s²
  float minSpeedForAccident;    // Tốc độ tối thiểu để phát hiện va chạm (km/h) - mặc định: 10 km/h
  float tempThreshold;          // Ngưỡng nhiệt độ (°C)
  float humiThreshold;          // Ngưỡng độ ẩm (%)
  char sosPhoneNumbers[256];    // Số điện thoại SOS (phân cách bằng dấu phẩy)
  bool valid;
};

// ============================================
// BIẾN TOÀN CỤC
// ============================================

// Đối tượng cảm biến
DHT dht(DHT_PIN, DHT_TYPE);
RTC_DS1307 rtc;
TinyGPSPlus gps;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0);

// Serial
HardwareSerial SerialGPS(1);
HardwareSerial SerialSIM(2);

// TinyGSM
TinyGsm modem(SerialSIM);
TinyGsmClient client(modem);

// WebServer - ĐÃ XÓA HOÀN TOÀN

// APN
const char apn[] = "m3-world";
const char user[] = "";
const char pass[] = "";

// Backend server
const char server[] = "hopdenthongminh.cloud";
const int serverPort = 80;

// Trạng thái hệ thống
SystemState systemState;
SensorData currentSensorData;
DeviceConfig deviceConfig;

// Mutex và Queue
SemaphoreHandle_t sensorDataMutex;
SemaphoreHandle_t systemStateMutex;
SemaphoreHandle_t configMutex;
QueueHandle_t smsQueue;
QueueHandle_t callQueue;
QueueHandle_t httpQueue;

struct HTTPRequest {
  char endpoint[64];
  char jsonData[512];
};

// Biến SIM 4G
bool sim4gInitialized = false;
bool sim4gNetworkOpen = false;
unsigned long lastTelemetrySend = 0;
unsigned long lastSOSSync = 0;

// Biến GPS
float lastGPSLat = 0;
float lastGPSLon = 0;
unsigned long lastGPSTime = 0;
bool hasLastGPS = false;

// Biến GPS để hiển thị OLED (tránh nhấp nháy)
float displayedGPSLat = 0;
float displayedGPSLon = 0;
unsigned long lastValidGPSTime = 0;
bool hasDisplayedGPS = false;

// Biến để tính gia tốc góc (từ gyroscope)
float lastGyroX = 0;
float lastGyroY = 0;
float lastGyroZ = 0;
unsigned long lastGyroTime = 0;
bool hasLastGyro = false;

// Biến Audio - TẠM THỜI BỎ GHI ÂM
// int16_t audioBuffer[AUDIO_BUFFER_SIZE];
// File currentAudioFile;
// bool audioFileOpen = false;

// Biến nút nhấn
unsigned long button1PressTime = 0;
unsigned long button2PressTime = 0;
bool button1Pressed = false;
bool button2Pressed = false;
bool button1LastState = false;
bool button2LastState = false;
unsigned long button1LastDebounceTime = 0;
unsigned long button2LastDebounceTime = 0;
// Biến webserver - ĐÃ XÓA
// bool bothButtonsPressed = false;
// unsigned long bothButtonsPressTime = 0;

// EEPROM
#define EEPROM_SIZE 512
#define EEPROM_CONFIG_ADDR 0

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== HOP DEN O TO / XE MAY - ESP32-S3 V2 ===");
  
  // Khởi tạo EEPROM và load config
  EEPROM.begin(EEPROM_SIZE);
  loadDeviceConfig();
  
  // Khởi tạo mutex và queue
  sensorDataMutex = xSemaphoreCreateMutex();
  systemStateMutex = xSemaphoreCreateMutex();
  configMutex = xSemaphoreCreateMutex();
  smsQueue = xQueueCreate(5, sizeof(char[160]));
  callQueue = xQueueCreate(5, sizeof(char[20]));
  httpQueue = xQueueCreate(10, sizeof(HTTPRequest));
  
  // Khởi tạo trạng thái
  systemState.status = STATUS_NORMAL;
  systemState.countdownStart = 0;
  systemState.countdownRemaining = 0;
  systemState.buzzerOn = false;
  systemState.currentSOSSource = SOS_SOURCE_NONE;
  systemState.currentSOSId = "";
  // Audio state - TẠM THỜI BỎ GHI ÂM
  // systemState.audioFileIndex = 0;
  // systemState.audioFileStartTime = 0;
  // systemState.audioRecording = false;
  
  // Khởi tạo GPIO
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Khởi tạo I2C (QUAN TRỌNG: Phải khởi tạo với chân đúng trước khi dùng RTC)
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  delay(100);  // Đợi I2C ổn định
  Serial.println("I2C initialized (SDA: GPIO 8, SCL: GPIO 9)");
  
  // Khởi tạo cảm biến
  dht.begin();
  Serial.println("DHT11 initialized");
  
  // Khởi tạo DS1307 (sau khi I2C đã khởi tạo)
  if (!rtc.begin()) {
    Serial.println("ERROR: RTC DS1307 not found!");
    Serial.println("Check I2C connections (SDA: GPIO 8, SCL: GPIO 9)");
  } else {
    if (!rtc.isrunning()) {
      Serial.println("RTC is NOT running, setting time...");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    Serial.println("RTC DS1307 initialized");
  }
  
  initMPU9250();
  
  // Khởi tạo GPS
  SerialGPS.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS Serial initialized");
  
  // Khởi tạo SIM 4G
  SerialSIM.begin(115200, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);
  delay(2000);
  SerialSIM.println("AT+CRESET");
  delay(2000);
  SerialSIM.flush();
  SerialSIM.println("ATE0");
  delay(1000);
  
  // Khởi tạo SD Card
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR: SD Card initialization failed!");
  } else {
    Serial.println("SD Card initialized");
  }
  
  // Khởi tạo OLED
  oled.begin();
  oled.setFont(u8g2_font_ncenB08_tr);
  oled.clearBuffer();
  oled.drawStr(0, 20, "HOP DEN V2");
  oled.drawStr(0, 40, "Initializing...");
  oled.sendBuffer();
  
  // Khởi tạo I2S cho audio - TẠM THỜI BỎ GHI ÂM
  // initI2SAudio();
  
  // Tạo tasks
  // xTaskCreatePinnedToCore(taskAudio, "Audio", 4096, NULL, 1, NULL, 0);  // TẠM THỜI BỎ GHI ÂM
  xTaskCreatePinnedToCore(taskSDLog, "SDLog", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskSensors, "Sensors", 8192, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskSIM4G, "SIM4G", 8192, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskOLED, "OLED", 4096, NULL, 1, NULL, 1);
  
  Serial.println("\n=== SYSTEM READY ===");
  delay(2000);
}

// ============================================
// LOOP
// ============================================

void loop() {
  // Webserver đã được xóa hoàn toàn
  delay(10);
}

// ============================================
// WEBSERVER ĐÃ ĐƯỢC XÓA HOÀN TOÀN
// ============================================

// ============================================
// TASK: AUDIO (Core 0) - TẠM THỜI BỎ GHI ÂM
// ============================================

/*
void taskAudio(void* parameter) {
  Serial.println("TaskAudio started on Core 0");
  
  while (true) {
    if (systemState.audioRecording) {
      // Đọc audio từ I2S
      size_t bytesRead;
      i2s_read(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);
      
      if (audioFileOpen && currentAudioFile) {
        currentAudioFile.write((uint8_t*)audioBuffer, bytesRead);
      }
      
      // Kiểm tra thời gian ghi âm
      unsigned long elapsed = (millis() - systemState.audioFileStartTime) / 1000;
      if (elapsed >= AUDIO_FILE_DURATION_SEC) {
        // Kết thúc file hiện tại
        if (audioFileOpen) {
          currentAudioFile.close();
          audioFileOpen = false;
        }
        
        // Tạo tên file mới theo thời gian kết thúc
        DateTime now = rtc.now();
        char fileName[64];
        snprintf(fileName, sizeof(fileName), "/AUDIO_%04d%02d%02d_%02d%02d%02d.wav",
                 now.year(), now.month(), now.day(),
                 now.hour(), now.minute(), now.second());
        
        // Tạo file mới
        currentAudioFile = SD.open(fileName, FILE_WRITE);
        if (currentAudioFile) {
          // Write WAV header
          writeWAVHeader(currentAudioFile, AUDIO_SAMPLE_RATE, AUDIO_BITS_PER_SAMPLE, AUDIO_CHANNELS);
          audioFileOpen = true;
          systemState.audioFileStartTime = millis();
          systemState.audioFileIndex = (systemState.audioFileIndex + 1) % AUDIO_MAX_FILES;
          
          Serial.printf("Started new audio file: %s\n", fileName);
        }
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
*/

// ============================================
// TASK: SD LOG (Core 0)
// ============================================

void taskSDLog(void* parameter) {
  Serial.println("TaskSDLog started on Core 0");
  unsigned long lastLogTime = 0;
  
  while (true) {
    if (millis() - lastLogTime >= LOG_INTERVAL_MS) {
      lastLogTime = millis();
      
      // Lấy dữ liệu sensor
      SensorData data;
      if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        data = currentSensorData;
        xSemaphoreGive(sensorDataMutex);
      }
      
      // Lấy trạng thái SOS từ phần cứng
      String sosStatus = "none";
      if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (systemState.currentSOSSource == SOS_SOURCE_HARDWARE_ACCIDENT) {
          sosStatus = "accident";
        } else if (systemState.currentSOSSource == SOS_SOURCE_HARDWARE_RESCUE) {
          sosStatus = "rescue";
        }
        xSemaphoreGive(systemStateMutex);
      }
      
      // Tạo tên file log theo ngày
      DateTime now = rtc.now();
      char logFileName[32];
      snprintf(logFileName, sizeof(logFileName), "/LOG_%04d%02d%02d.csv",
               now.year(), now.month(), now.day());
      
      // Mở file log
      File logFile = SD.open(logFileName, FILE_APPEND);
      if (logFile) {
        // Ghi dữ liệu CSV
        logFile.printf("%04d-%02d-%02d %02d:%02d:%02d,",
                      now.year(), now.month(), now.day(),
                      now.hour(), now.minute(), now.second());
        logFile.printf("%.6f,%.6f,%.1f,", data.lat, data.lon, data.speed);
        logFile.printf("%.2f,%.2f,%.2f,", data.ax, data.ay, data.az);
        logFile.printf("%.2f,%.2f,%.2f,", data.gx, data.gy, data.gz);
        logFile.printf("%.1f,%.1f,", data.temp, data.humi);
        logFile.printf("%s\n", sosStatus.c_str());
        
        logFile.close();
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// ============================================
// TASK: SENSORS (Core 1)
// ============================================

void taskSensors(void* parameter) {
  Serial.println("TaskSensors started on Core 1");
  
  while (true) {
    SensorData data;
    data.valid = false;
    
    // Đọc cảm biến
    readMPU9250(&data);
    readDHT11(&data);
    readGPS(&data);
    readRTC(&data);
    
    // Cập nhật dữ liệu
    if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      currentSensorData = data;
      xSemaphoreGive(sensorDataMutex);
    }
    
    // Kiểm tra nhiệt độ/độ ẩm
    checkTemperatureHumidity(&data);
    
    // Tính gia tốc tuyến tính (magnitude)
    float accelMagnitude = sqrt(data.ax * data.ax + data.ay * data.ay + data.az * data.az);
    
    // Tính gia tốc góc (angular acceleration) từ gyroscope
    unsigned long currentTime = millis();
    float angularAccelMagnitude = 0.0;
    
    if (hasLastGyro && (currentTime - lastGyroTime) > 0) {
      // Tính sự thay đổi tốc độ góc (deg/s)
      float deltaGyroX = data.gx - lastGyroX;
      float deltaGyroY = data.gy - lastGyroY;
      float deltaGyroZ = data.gz - lastGyroZ;
      
      // Tính gia tốc góc (deg/s²) = thay đổi tốc độ góc / thời gian
      float deltaTime = (currentTime - lastGyroTime) / 1000.0f;  // Chuyển sang giây
      if (deltaTime > 0) {
        float angularAccelX = deltaGyroX / deltaTime;
        float angularAccelY = deltaGyroY / deltaTime;
        float angularAccelZ = deltaGyroZ / deltaTime;
        
        // Magnitude của gia tốc góc
        angularAccelMagnitude = sqrt(angularAccelX * angularAccelX + 
                                     angularAccelY * angularAccelY + 
                                     angularAccelZ * angularAccelZ);
      }
    }
    
    // Cập nhật giá trị gyro để tính lần sau
    lastGyroX = data.gx;
    lastGyroY = data.gy;
    lastGyroZ = data.gz;
    lastGyroTime = currentTime;
    hasLastGyro = true;
    
    if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      if (systemState.status == STATUS_NORMAL) {
        // Phát hiện va chạm - kết hợp:
        // 1. Tốc độ >= ngưỡng tối thiểu (tránh báo giả khi đứng yên)
        // 2. Gia tốc tuyến tính > ngưỡng VÀ/HOẶC gia tốc góc > ngưỡng
        float linearAccelThreshold = deviceConfig.accidentThreshold;
        float angularAccelThreshold = deviceConfig.angularAccelThreshold;
        float minSpeedThreshold = deviceConfig.minSpeedForAccident;
        float currentSpeed = data.speed;
        
        bool linearAccelExceeded = accelMagnitude > linearAccelThreshold;
        bool angularAccelExceeded = angularAccelMagnitude > angularAccelThreshold;
        bool speedThresholdMet = currentSpeed >= minSpeedThreshold;
        
        // Phát hiện va chạm nếu: tốc độ đủ VÀ (gia tốc tuyến tính vượt ngưỡng HOẶC gia tốc góc vượt ngưỡng)
        if (speedThresholdMet && (linearAccelExceeded || angularAccelExceeded)) {
          systemState.status = STATUS_ACCIDENT_COUNTDOWN;
          systemState.countdownStart = millis();
          systemState.countdownRemaining = ACCIDENT_COUNTDOWN_SEC;
          systemState.buzzerOn = true;
          systemState.currentSOSSource = SOS_SOURCE_HARDWARE_ACCIDENT;
          Serial.printf("!!! ACCIDENT DETECTED - Starting countdown !!!\n");
          Serial.printf("    Linear Accel: %.2f g (threshold: %.2f g)\n", accelMagnitude, linearAccelThreshold);
          Serial.printf("    Angular Accel: %.2f deg/s² (threshold: %.2f deg/s²)\n", angularAccelMagnitude, angularAccelThreshold);
          Serial.printf("    Speed: %.2f km/h (min: %.2f km/h)\n", currentSpeed, minSpeedThreshold);
        } else if ((linearAccelExceeded || angularAccelExceeded) && !speedThresholdMet) {
          // Ignore nếu speed < ngưỡng (có thể là rung động khi đứng yên)
          Serial.printf("[ACCIDENT] Ignored - Speed too low (%.2f km/h < %.2f km/h)\n", 
                       currentSpeed, minSpeedThreshold);
        }
      }
      
      // Xử lý đếm ngược tai nạn
      if (systemState.status == STATUS_ACCIDENT_COUNTDOWN) {
        unsigned long elapsed = (millis() - systemState.countdownStart) / 1000;
        systemState.countdownRemaining = ACCIDENT_COUNTDOWN_SEC - elapsed;
        
        if (systemState.countdownRemaining <= 0) {
          sendAccidentSOS();
          systemState.status = STATUS_ACCIDENT_SOS_SENT;
          systemState.buzzerOn = false;
          Serial.println("ACCIDENT SOS SENT!");
        }
      }
      
      // Xử lý đếm ngược cứu hộ
      if (systemState.status == STATUS_RESCUE_COUNTDOWN) {
        unsigned long elapsed = (millis() - systemState.countdownStart) / 1000;
        systemState.countdownRemaining = ACCIDENT_COUNTDOWN_SEC - elapsed;
        
        if (systemState.countdownRemaining <= 0) {
          sendRescueSOS();
          systemState.status = STATUS_RESCUE_SOS_SENT;
          systemState.buzzerOn = false;
          Serial.println("RESCUE SOS SENT!");
        }
      }
      
      xSemaphoreGive(systemStateMutex);
    }
    
    // Cập nhật buzzer trong quá trình đếm ngược
    updateBuzzer();
    
    // Xử lý nút nhấn
    handleButtons();
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ============================================
// TASK: SIM 4G (Core 1)
// ============================================

void taskSIM4G(void* parameter) {
  Serial.println("TaskSIM4G started on Core 1");
  
  // Khởi tạo SIM 4G
  initSIM4G();
  
  unsigned long lastStatusCheck = 0;
  
  while (true) {
    // Kiểm tra trạng thái module định kỳ (mỗi 60 giây)
    if (millis() - lastStatusCheck >= 60000) {
      lastStatusCheck = millis();
      SerialSIM.println("AT+CSQ");
      delay(200);
      String response = "";
      unsigned long startTime = millis();
      while (millis() - startTime < 1000) {
        if (SerialSIM.available()) {
          char c = SerialSIM.read();
          response += c;
        }
      }
      Serial.printf("[SIM] Status check: %s\n", response.substring(0, 50).c_str());
      logSIMToSD("STATUS", "PERIODIC", response.substring(0, 50).c_str());
    }
    
    // Gửi telemetry định kỳ (chỉ khi SIM 4G đã sẵn sàng)
    if (sim4gInitialized && sim4gNetworkOpen && millis() - lastTelemetrySend >= TELEMETRY_INTERVAL_MS) {
      Serial.println("[SIM] Sending telemetry...");
      sendTelemetryData();
      lastTelemetrySend = millis();
    } else if ((!sim4gInitialized || !sim4gNetworkOpen) && millis() - lastTelemetrySend >= TELEMETRY_INTERVAL_MS) {
      // Nếu SIM chưa sẵn sàng, log cảnh báo
      static unsigned long lastWarning = 0;
      if (millis() - lastWarning >= 60000) {  // Cảnh báo mỗi 60 giây
        Serial.println("[SIM] Telemetry delayed - SIM4G not ready yet");
        lastWarning = millis();
      }
    }
    
    // Đồng bộ SOS với server
    if (millis() - lastSOSSync >= SOS_SYNC_INTERVAL_MS) {
      syncSOSWithServer();
      lastSOSSync = millis();
    }
    
    // Xử lý queue SMS, Call, HTTP
    char smsText[160];
    if (xQueueReceive(smsQueue, smsText, 0) == pdTRUE) {
      sendSMS(deviceConfig.sosPhoneNumbers, smsText);
    }
    
    char phoneNumber[20];
    if (xQueueReceive(callQueue, phoneNumber, 0) == pdTRUE) {
      makeCall(phoneNumber);
    }
    
    HTTPRequest httpReq;
    if (xQueueReceive(httpQueue, &httpReq, 0) == pdTRUE) {
      sendHTTPPost(httpReq.endpoint, httpReq.jsonData);
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ============================================
// TASK: OLED (Core 1)
// ============================================

void taskOLED(void* parameter) {
  Serial.println("TaskOLED started on Core 1");
  unsigned long lastUpdate = 0;
  
  while (true) {
    if (millis() - lastUpdate >= OLED_UPDATE_MS) {
      lastUpdate = millis();
      
      oled.clearBuffer();
      oled.setFont(u8g2_font_ncenB08_tr);
      
      // Lấy dữ liệu
      SensorData data;
      SystemStatus status;
      int countdown = 0;
      SOSSource sosSource = SOS_SOURCE_NONE;
      
      if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        data = currentSensorData;
        xSemaphoreGive(sensorDataMutex);
      }
      
      if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        status = systemState.status;
        countdown = systemState.countdownRemaining;
        sosSource = systemState.currentSOSSource;
        xSemaphoreGive(systemStateMutex);
      }
      
      // Hiển thị thông tin
      DateTime now = rtc.now();
      char timeStr[16];
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
      oled.drawStr(0, 12, timeStr);
      
      char gpsStr[32];
      // Hiển thị GPS với debounce để tránh nhấp nháy
      // Chỉ hiển thị "No signal" nếu không có GPS trong > 5 giây
      unsigned long timeSinceLastValidGPS = millis() - lastValidGPSTime;
      bool gpsCurrentlyValid = data.valid && (data.lat != 0 || data.lon != 0);
      
      if (gpsCurrentlyValid) {
        // GPS hiện tại valid - hiển thị ngay
        snprintf(gpsStr, sizeof(gpsStr), "GPS: %.4f,%.4f", data.lat, data.lon);
        // Cập nhật giá trị hiển thị
        displayedGPSLat = data.lat;
        displayedGPSLon = data.lon;
        lastValidGPSTime = millis();
        hasDisplayedGPS = true;
      } else if (hasDisplayedGPS && timeSinceLastValidGPS < 5000) {
        // Chưa có GPS mới nhưng mới mất < 5 giây - hiển thị giá trị cuối cùng
        snprintf(gpsStr, sizeof(gpsStr), "GPS: %.4f,%.4f", displayedGPSLat, displayedGPSLon);
      } else {
        // Mất GPS > 5 giây - hiển thị "No signal"
        snprintf(gpsStr, sizeof(gpsStr), "GPS: No signal");
        hasDisplayedGPS = false;
      }
      oled.drawStr(0, 24, gpsStr);
      
      char tempStr[32];
      snprintf(tempStr, sizeof(tempStr), "T:%.1fC H:%.1f%%", data.temp, data.humi);
      oled.drawStr(0, 36, tempStr);
      
      // Hiển thị trạng thái SOS
      if (sosSource != SOS_SOURCE_NONE) {
        if (status == STATUS_ACCIDENT_COUNTDOWN || status == STATUS_RESCUE_COUNTDOWN) {
          char countdownStr[32];
          snprintf(countdownStr, sizeof(countdownStr), "SOS: %d", countdown);
          oled.drawStr(0, 48, countdownStr);
        } else if (status == STATUS_ACCIDENT_SOS_SENT || status == STATUS_RESCUE_SOS_SENT) {
          oled.drawStr(0, 48, "SOS SENT");
        }
      } else {
        oled.drawStr(0, 48, "NORMAL");
      }
      
      oled.sendBuffer();
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ============================================
// HÀM ĐỌC CẢM BIẾN
// ============================================

void readMPU9250(SensorData* data) {
  // Đọc từ MPU9250 register 0x3B (ACCEL_XOUT_H) - 14 bytes
  // 6 bytes accel + 2 bytes temp + 6 bytes gyro
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(MPU9250_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) {  // false = không gửi STOP
    // Nếu không đọc được, giữ giá trị mặc định
    data->ax = 0; data->ay = 0; data->az = 9.8;
    data->gx = 0; data->gy = 0; data->gz = 0;
    return;
  }
  
  Wire.requestFrom(MPU9250_ADDRESS, 14, true);  // true = gửi STOP sau khi đọc
  if (Wire.available() < 14) {
    data->ax = 0; data->ay = 0; data->az = 9.8;
    data->gx = 0; data->gy = 0; data->gz = 0;
    return;
  }
  
  // Đọc Accelerometer (16-bit, big-endian)
  int16_t accelX = (Wire.read() << 8) | Wire.read();
  int16_t accelY = (Wire.read() << 8) | Wire.read();
  int16_t accelZ = (Wire.read() << 8) | Wire.read();
  
  // Bỏ qua Temperature (2 bytes)
  Wire.read();
  Wire.read();
  
  // Đọc Gyroscope (16-bit, big-endian)
  int16_t gyroX = (Wire.read() << 8) | Wire.read();
  int16_t gyroY = (Wire.read() << 8) | Wire.read();
  int16_t gyroZ = (Wire.read() << 8) | Wire.read();
  
  // Chuyển đổi sang đơn vị thực
  // Accelerometer: ±2g = 16384 LSB/g
  data->ax = accelX / 16384.0f;
  data->ay = accelY / 16384.0f;
  data->az = accelZ / 16384.0f;
  
  // Gyroscope: ±250°/s = 131 LSB/°/s
  data->gx = gyroX / 131.0f;
  data->gy = gyroY / 131.0f;
  data->gz = gyroZ / 131.0f;
}

void readDHT11(SensorData* data) {
  data->temp = dht.readTemperature();
  data->humi = dht.readHumidity();
}

void readGPS(SensorData* data) {
  while (SerialGPS.available() > 0) {
    if (gps.encode(SerialGPS.read())) {
      if (gps.location.isValid()) {
        data->lat = gps.location.lat();
        data->lon = gps.location.lng();
        
        // Ưu tiên dùng speed từ GPS nếu có
        if (gps.speed.isValid() && gps.speed.kmph() > 0) {
          data->speed = gps.speed.kmph();
        } else {
          // Tính vận tốc từ 2 điểm GPS liên tiếp
          unsigned long currentTime = millis();
          if (hasLastGPS && (currentTime - lastGPSTime) > 1000) {  // Ít nhất 1 giây
            float calculatedSpeed = calculateSpeedFromGPS(
              lastGPSLat, lastGPSLon, lastGPSTime,
              data->lat, data->lon, currentTime
            );
            data->speed = calculatedSpeed;
          } else {
            data->speed = 0.0;  // Chưa đủ dữ liệu để tính
          }
        }
        
        // Cập nhật điểm GPS trước để tính vận tốc lần sau
        lastGPSLat = data->lat;
        lastGPSLon = data->lon;
        lastGPSTime = millis();
        hasLastGPS = true;
        
        // Cập nhật GPS để hiển thị trên OLED (tránh nhấp nháy)
        displayedGPSLat = data->lat;
        displayedGPSLon = data->lon;
        lastValidGPSTime = millis();
        hasDisplayedGPS = true;
        
        data->valid = true;
      } else {
        data->valid = false;
      }
    }
  }
}

void readRTC(SensorData* data) {
  data->rtcTime = rtc.now();
}

// ============================================
// HÀM TÍNH VẬN TỐC TỪ GPS
// ============================================

float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  const float R = 6371000.0;  // Bán kính Trái Đất (mét)
  float dLat = (lat2 - lat1) * PI / 180.0;
  float dLon = (lon2 - lon1) * PI / 180.0;
  float a = sin(dLat / 2.0) * sin(dLat / 2.0) +
            cos(lat1 * PI / 180.0) * cos(lat2 * PI / 180.0) *
            sin(dLon / 2.0) * sin(dLon / 2.0);
  float c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
  return R * c;
}

float calculateSpeedFromGPS(float lat1, float lon1, unsigned long time1,
                             float lat2, float lon2, unsigned long time2) {
  if (time2 <= time1) {
    return 0.0;
  }
  float distance = calculateDistance(lat1, lon1, lat2, lon2);
  float timeSeconds = (time2 - time1) / 1000.0;
  if (timeSeconds <= 0) {
    return 0.0;
  }
  float speedMs = distance / timeSeconds;
  return speedMs * 3.6;  // Chuyển m/s sang km/h
}

// ============================================
// HÀM XỬ LÝ NÚT NHẤN (VỚI DEBOUNCE)
// ============================================

void handleButtons() {
  // Đọc trạng thái nút
  bool btn1Reading = digitalRead(BUTTON_1_PIN) == LOW;
  bool btn2Reading = digitalRead(BUTTON_2_PIN) == LOW;
  
  unsigned long currentTime = millis();
  
  // ============================================
  // BUTTON 1: Hủy SOS tai nạn
  // ============================================
  
  // Debounce cho button 1 - reset timer nếu trạng thái thay đổi
  if (btn1Reading != button1LastState) {
    button1LastDebounceTime = currentTime;
    button1LastState = btn1Reading;
  }
  
  // Nếu nút đã giữ ổn định trong khoảng debounce time (50ms)
  if ((currentTime - button1LastDebounceTime) >= BUTTON_DEBOUNCE_MS) {
    if (btn1Reading && !button1Pressed) {
      // Nút vừa được nhấn (debounced) - bắt đầu đếm thời gian giữ
      button1Pressed = true;
      button1PressTime = currentTime;
      Serial.println("[BUTTON1] Pressed (debounced)");
    } else if (btn1Reading && button1Pressed) {
      // Nút đang được giữ - kiểm tra thời gian
      if (currentTime - button1PressTime >= BUTTON_HOLD_MS) {
        // Đã giữ đủ 5 giây - xử lý Hủy SOS tai nạn
        if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
          if (systemState.status == STATUS_ACCIDENT_COUNTDOWN) {
            systemState.status = STATUS_NORMAL;
            systemState.buzzerOn = false;
            systemState.currentSOSSource = SOS_SOURCE_NONE;
            digitalWrite(BUZZER_PIN, LOW);
            Serial.println("[BUTTON1] ACCIDENT SOS CANCELLED");
          }
          xSemaphoreGive(systemStateMutex);
        }
        
        // Đợi nút được nhả ra hoàn toàn (dùng while để chống bounce)
        while (digitalRead(BUTTON_1_PIN) == LOW) {
          delay(10);
        }
        // Đợi thêm một chút để đảm bảo nút đã ổn định
        delay(BUTTON_DEBOUNCE_MS);
        button1Pressed = false;
        Serial.println("[BUTTON1] Released");
      }
    } else if (!btn1Reading && button1Pressed) {
      // Nút vừa được nhả ra (trước khi đủ 5 giây) - reset
      button1Pressed = false;
      Serial.println("[BUTTON1] Released (not held long enough)");
    }
  }
  
  // ============================================
  // BUTTON 2: SOS cứu hộ
  // ============================================
  
  // Debounce cho button 2 - reset timer nếu trạng thái thay đổi
  if (btn2Reading != button2LastState) {
    button2LastDebounceTime = currentTime;
    button2LastState = btn2Reading;
  }
  
  // Nếu nút đã giữ ổn định trong khoảng debounce time (50ms)
  if ((currentTime - button2LastDebounceTime) >= BUTTON_DEBOUNCE_MS) {
    if (btn2Reading && !button2Pressed) {
      // Nút vừa được nhấn (debounced) - bắt đầu đếm thời gian giữ
      button2Pressed = true;
      button2PressTime = currentTime;
      Serial.println("[BUTTON2] Pressed (debounced)");
    } else if (btn2Reading && button2Pressed) {
      // Nút đang được giữ - kiểm tra thời gian
      if (currentTime - button2PressTime >= BUTTON_HOLD_MS) {
        // Đã giữ đủ 5 giây - xử lý SOS cứu hộ
        if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
          if (systemState.status == STATUS_NORMAL) {
            // Kích hoạt SOS cứu hộ
            systemState.status = STATUS_RESCUE_COUNTDOWN;
            systemState.countdownStart = millis();
            systemState.countdownRemaining = ACCIDENT_COUNTDOWN_SEC;
            systemState.buzzerOn = true;
            systemState.currentSOSSource = SOS_SOURCE_HARDWARE_RESCUE;
            Serial.println("[BUTTON2] RESCUE SOS ACTIVATED");
          } else if (systemState.status == STATUS_RESCUE_COUNTDOWN) {
            // Hủy SOS cứu hộ
            systemState.status = STATUS_NORMAL;
            systemState.buzzerOn = false;
            systemState.currentSOSSource = SOS_SOURCE_NONE;
            digitalWrite(BUZZER_PIN, LOW);
            Serial.println("[BUTTON2] RESCUE SOS CANCELLED");
          }
          xSemaphoreGive(systemStateMutex);
        }
        
        // Đợi nút được nhả ra hoàn toàn (dùng while để chống bounce)
        while (digitalRead(BUTTON_2_PIN) == LOW) {
          delay(10);
        }
        // Đợi thêm một chút để đảm bảo nút đã ổn định
        delay(BUTTON_DEBOUNCE_MS);
        button2Pressed = false;
        Serial.println("[BUTTON2] Released");
      }
    } else if (!btn2Reading && button2Pressed) {
      // Nút vừa được nhả ra (trước khi đủ 5 giây) - reset
      button2Pressed = false;
      Serial.println("[BUTTON2] Released (not held long enough)");
    }
  }
}

// ============================================
// HÀM BUZZER
// ============================================

void updateBuzzer() {
  if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(50)) != pdTRUE) return;
  
  bool buzzerOn = systemState.buzzerOn;
  SystemStatus status = systemState.status;
  int countdownRemaining = systemState.countdownRemaining;
  xSemaphoreGive(systemStateMutex);
  
  // Kích hoạt buzzer khi đang đếm ngược
  if (buzzerOn && (status == STATUS_ACCIDENT_COUNTDOWN || status == STATUS_RESCUE_COUNTDOWN)) {
    static unsigned long lastBeep = 0;
    unsigned long beepInterval = 200;  // Beep mỗi 200ms (tần số ~5Hz)
    
    // Tăng tần số beep khi gần hết thời gian (cảnh báo gấp)
    if (countdownRemaining <= 10) {
      beepInterval = 100;  // Beep nhanh hơn khi còn < 10 giây
    } else if (countdownRemaining <= 5) {
      beepInterval = 50;   // Beep rất nhanh khi còn < 5 giây
    }
    
    if (millis() - lastBeep >= beepInterval) {
      digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN));
      lastBeep = millis();
      Serial.printf("[BUZZER] Beep - Countdown: %d sec\n", countdownRemaining);
    }
  } else {
    // Tắt buzzer khi không đếm ngược
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ============================================
// HÀM KIỂM TRA NHIỆT ĐỘ/ĐỘ ẨM
// ============================================

void checkTemperatureHumidity(SensorData* data) {
  // Kiểm tra xem có đang đếm ngược SOS không (ưu tiên SOS)
  if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    SystemStatus status = systemState.status;
    xSemaphoreGive(systemStateMutex);
    
    // Nếu đang đếm ngược SOS, không cảnh báo nhiệt độ/độ ẩm (tránh xung đột buzzer)
    if (status == STATUS_ACCIDENT_COUNTDOWN || status == STATUS_RESCUE_COUNTDOWN) {
      return;
    }
  }
  
  static unsigned long lastAlert = 0;
  static bool beepState = false;
  
  if (data->temp > deviceConfig.tempThreshold || data->humi > deviceConfig.humiThreshold) {
    if (millis() - lastAlert >= 1000) {
      beepState = !beepState;
      digitalWrite(BUZZER_PIN, beepState);
      lastAlert = millis();
      Serial.printf("WARNING: Temp=%.1fC/%.1fC Humi=%.1f%%/%.1f%%\n",
                   data->temp, deviceConfig.tempThreshold, data->humi, deviceConfig.humiThreshold);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ============================================
// HÀM GỬI SOS
// ============================================

void sendSOS(const char* type, const char* severity, const char* notePrefix) {
  unsigned long startTime = millis();
  Serial.println("\n[SOS] ===== Starting SOS send =====");
  Serial.printf("[SOS] Type: %s, Severity: %s\n", type, severity);
  
  SensorData data;
  if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    data = currentSensorData;
    xSemaphoreGive(sensorDataMutex);
  } else {
    Serial.println("[SOS] ERROR: Failed to acquire sensorDataMutex");
    return;
  }
  
  DateTime now = rtc.now();
  float accelMag = sqrt(data.ax*data.ax + data.ay*data.ay + data.az*data.az);
  
  Serial.printf("[SOS] Location: Lat=%.6f, Lon=%.6f\n", data.lat, data.lon);
  Serial.printf("[SOS] Speed: %.2f km/h, Accel: %.2f g\n", data.speed, accelMag);
  Serial.printf("[SOS] Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  
  // Tạo JSON
  StaticJsonDocument<512> doc;
  doc["userId"] = deviceConfig.userId;
  doc["deviceId"] = deviceConfig.deviceId;
  doc["type"] = type;
  doc["severity"] = severity;
  JsonObject loc = doc.createNestedObject("location");
  loc["lat"] = data.lat;
  loc["lon"] = data.lon;
  
  char note[256];
  snprintf(note, sizeof(note), "%s Speed:%.1f km/h Accel:%.2fg Time:%04d-%02d-%02d %02d:%02d:%02d",
           notePrefix, data.speed, accelMag, now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  doc["note"] = note;
  
  String json;
  serializeJson(doc, json);
  
  Serial.printf("[SOS] JSON data (%d bytes): %s\n", json.length(), json.c_str());
  Serial.printf("[SOS] Device: userId=%s, deviceId=%s\n", 
                deviceConfig.userId, deviceConfig.deviceId);
  
  // Gửi HTTP POST /api/sos
  HTTPRequest req;
  strncpy(req.endpoint, "/api/sos", sizeof(req.endpoint) - 1);
  req.endpoint[sizeof(req.endpoint) - 1] = '\0';
  strncpy(req.jsonData, json.c_str(), sizeof(req.jsonData) - 1);
  req.jsonData[sizeof(req.jsonData) - 1] = '\0';
  
  if (xQueueSend(httpQueue, &req, 0) == pdTRUE) {
    Serial.println("[SOS] ✓ HTTP POST request queued successfully");
  } else {
    Serial.println("[SOS] ✗ ERROR: Failed to queue HTTP request (queue full)");
  }
  
  // Gửi SMS
  char sms[160];
  snprintf(sms, sizeof(sms), "SOS %s - Lat:%.6f Lon:%.6f Speed:%.1f Time:%04d-%02d-%02d %02d:%02d:%02d",
           type, data.lat, data.lon, data.speed, now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  if (xQueueSend(smsQueue, sms, 0) == pdTRUE) {
    Serial.printf("[SOS] ✓ SMS queued: %s\n", sms);
  } else {
    Serial.println("[SOS] ✗ ERROR: Failed to queue SMS (queue full)");
  }
  
  // Gọi điện
  if (xQueueSend(callQueue, deviceConfig.sosPhoneNumbers, 0) == pdTRUE) {
    Serial.printf("[SOS] ✓ Call queued: %s\n", deviceConfig.sosPhoneNumbers);
  } else {
    Serial.println("[SOS] ✗ ERROR: Failed to queue call (queue full)");
  }
  
  unsigned long elapsed = millis() - startTime;
  Serial.printf("[SOS] Total time: %lu ms\n", elapsed);
  Serial.printf("[SOS] %s SOS sent to server\n", type);
  Serial.println("[SOS] ===== End SOS send =====\n");
}

void sendAccidentSOS() {
  sendSOS("accident", "critical", "SOS Tai nan tu dong");
}

void sendRescueSOS() {
  sendSOS("breakdown", "high", "SOS Cuu ho thu cong");
}

// ============================================
// HÀM GỬI TELEMETRY
// ============================================

void sendTelemetryData() {
  unsigned long startTime = millis();
  Serial.println("\n[TELEMETRY] ===== Starting telemetry send =====");
  
  SensorData data;
  if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    Serial.println("[TELEMETRY] ERROR: Failed to acquire sensorDataMutex");
    return;
  }
  data = currentSensorData;
  xSemaphoreGive(sensorDataMutex);
  
  // Kiểm tra GPS data hợp lệ
  bool gpsValid = data.valid && (data.lat != 0.0 || data.lon != 0.0);
  
  Serial.printf("[TELEMETRY] Sensor data: Lat=%.6f, Lon=%.6f, Speed=%.2f km/h, Valid=%s\n", 
                data.lat, data.lon, data.speed, gpsValid ? "YES" : "NO");
  
  // Nếu GPS không hợp lệ, sử dụng giá trị GPS cuối cùng đã hiển thị (tránh gửi 0,0)
  float latToSend = data.lat;
  float lonToSend = data.lon;
  
  if (!gpsValid) {
    // Sử dụng GPS đã hiển thị (tránh nhấp nháy) nếu có
    if (hasDisplayedGPS && displayedGPSLat != 0.0 && displayedGPSLon != 0.0) {
      latToSend = displayedGPSLat;
      lonToSend = displayedGPSLon;
      Serial.printf("[TELEMETRY] GPS invalid, using last displayed GPS: Lat=%.6f, Lon=%.6f\n", 
                    latToSend, lonToSend);
    } else {
      Serial.println("[TELEMETRY] ✗ WARNING: GPS invalid and no last GPS available, sending anyway");
    }
  }
  
  String sosStatus = "none";
  if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    if (systemState.currentSOSSource == SOS_SOURCE_HARDWARE_ACCIDENT) sosStatus = "accident";
    else if (systemState.currentSOSSource == SOS_SOURCE_HARDWARE_RESCUE) sosStatus = "rescue";
    xSemaphoreGive(systemStateMutex);
  }
  Serial.printf("[TELEMETRY] SOS Status: %s\n", sosStatus.c_str());
  
  DateTime now = rtc.now();
  char ts[32];
  snprintf(ts, sizeof(ts), "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
           now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  
  StaticJsonDocument<512> doc;
  doc["userId"] = deviceConfig.userId;
  doc["deviceId"] = deviceConfig.deviceId;
  doc["lat"] = latToSend;
  doc["lon"] = lonToSend;
  doc["speed"] = data.speed;
  doc["source"] = "hardware";
  doc["timestamp"] = ts;
  doc["sosStatus"] = sosStatus;
  
  String json;
  serializeJson(doc, json);
  
  Serial.printf("[TELEMETRY] JSON data (%d bytes): %s\n", json.length(), json.c_str());
  Serial.printf("[TELEMETRY] Device: userId=%s, deviceId=%s\n", 
                deviceConfig.userId, deviceConfig.deviceId);
  
  HTTPRequest req;
  strncpy(req.endpoint, "/api/telemetry", sizeof(req.endpoint) - 1);
  req.endpoint[sizeof(req.endpoint) - 1] = '\0';
  strncpy(req.jsonData, json.c_str(), sizeof(req.jsonData) - 1);
  req.jsonData[sizeof(req.jsonData) - 1] = '\0';
  
  if (xQueueSend(httpQueue, &req, 0) == pdTRUE) {
    Serial.println("[TELEMETRY] ✓ Request queued successfully");
  } else {
    Serial.println("[TELEMETRY] ✗ ERROR: Failed to queue request (queue full)");
  }
  
  unsigned long elapsed = millis() - startTime;
  Serial.printf("[TELEMETRY] Total time: %lu ms\n", elapsed);
  Serial.println("[TELEMETRY] ===== End telemetry send =====\n");
}

// ============================================
// HÀM ĐỒNG BỘ SOS VỚI SERVER
// ============================================

void syncSOSWithServer() {
  unsigned long startTime = millis();
  Serial.println("\n[SYNC] ===== Starting SOS sync =====");
  
  // Kiểm tra điều kiện
  String currentSOSId = "";
  SOSSource currentSource = SOS_SOURCE_NONE;
  
  if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    currentSource = systemState.currentSOSSource;
    currentSOSId = systemState.currentSOSId;
    xSemaphoreGive(systemStateMutex);
  }
  
  Serial.printf("[SYNC] Current SOS source: %d\n", currentSource);
  Serial.printf("[SYNC] Current SOS ID: %s\n", currentSOSId.c_str());
  
  if (currentSource == SOS_SOURCE_NONE || currentSOSId.isEmpty()) {
    Serial.println("[SYNC] No active SOS to sync, skipping...");
    return;
  }
  
  // Kiểm tra trạng thái mạng
  Serial.printf("[SYNC] Checking network status...\n");
  Serial.printf("[SYNC] SIM4G initialized: %s\n", sim4gInitialized ? "YES" : "NO");
  Serial.printf("[SYNC] Network open: %s\n", sim4gNetworkOpen ? "YES" : "NO");
  Serial.printf("[SYNC] GPRS connected: %s\n", modem.isGprsConnected() ? "YES" : "NO");
  
  if (!sim4gInitialized || !sim4gNetworkOpen || !modem.isGprsConnected()) {
    Serial.println("[SYNC] ✗ ERROR: Network not connected, skipping sync...");
    return;
  }
  
  // Gửi HTTP GET request để kiểm tra trạng thái SOS
  char endpoint[64];
  snprintf(endpoint, sizeof(endpoint), "/api/sos/%s", currentSOSId.c_str());
  
  Serial.printf("[SYNC] Checking SOS status: %s\n", currentSOSId.c_str());
  Serial.printf("[SYNC] Endpoint: %s\n", endpoint);
  logSIMToSD("SYNC", currentSOSId.c_str(), "Checking status");
  
  // Tạo HttpClient
  Serial.println("[SYNC] Creating HttpClient...");
  HttpClient httpClient(client, server, serverPort);
  
  // Gửi GET request
  Serial.println("[SYNC] Sending GET request...");
  unsigned long getStartTime = millis();
  httpClient.get(endpoint);
  unsigned long getTime = millis() - getStartTime;
  Serial.printf("[SYNC] GET request sent in %lu ms\n", getTime);
  
  // Đọc status code
  Serial.println("[SYNC] Reading response...");
  unsigned long responseStartTime = millis();
  int statusCode = httpClient.responseStatusCode();
  unsigned long statusTime = millis() - responseStartTime;
  Serial.printf("[SYNC] Status code received in %lu ms: %d\n", statusTime, statusCode);
  
  String response = httpClient.responseBody();
  unsigned long responseTime = millis() - responseStartTime;
  Serial.printf("[SYNC] Full response received in %lu ms (%d bytes)\n", responseTime, response.length());
  Serial.printf("[SYNC] Response: %s\n", response.c_str());
  
  // Đóng connection
  httpClient.stop();
  Serial.printf("[SYNC] Connection closed\n");
  
  if (statusCode != 200) {
    Serial.printf("[SYNC] ✗ Failed to get SOS status (status %d)\n", statusCode);
    return;
  }
  
  // Parse JSON response
  Serial.println("[SYNC] Parsing JSON response...");
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) {
    Serial.printf("[SYNC] ✗ JSON parse error: %s\n", error.c_str());
    return;
  }
  
  Serial.println("[SYNC] ✓ JSON parsed successfully");
  
  // Kiểm tra trạng thái SOS
  if (doc.containsKey("status")) {
    String status = doc["status"].as<String>();
    Serial.printf("[SYNC] SOS status from server: %s\n", status.c_str());
    
    // Nếu status="done" hoặc "cancelled" → Hủy SOS
    if (status == "done" || status == "cancelled") {
      String sosIdCopy = currentSOSId;  // Lưu lại SOS ID trước khi clear
      
      Serial.println("[SYNC] SOS status changed to: " + status);
      Serial.println("[SYNC] Cancelling local SOS...");
      
      if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        systemState.status = STATUS_NORMAL;
        systemState.buzzerOn = false;
        systemState.currentSOSSource = SOS_SOURCE_NONE;
        systemState.currentSOSId = "";
        digitalWrite(BUZZER_PIN, LOW);
        xSemaphoreGive(systemStateMutex);
      }
      
      Serial.println("[SYNC] ✓ SOS cancelled by server (status: " + status + ")");
      logSIMToSD("SYNC", sosIdCopy.c_str(), ("CANCELLED: " + status).c_str());
    } else {
      Serial.printf("[SYNC] SOS status unchanged: %s\n", status.c_str());
    }
  } else {
    Serial.println("[SYNC] ✗ Response does not contain 'status' field");
  }
  
  unsigned long totalTime = millis() - startTime;
  Serial.printf("[SYNC] Total time: %lu ms\n", totalTime);
  Serial.println("[SYNC] ===== End SOS sync =====\n");
}

// ============================================
// WEBSERVER ĐÃ ĐƯỢC XÓA HOÀN TOÀN
// ============================================

// ============================================
// HÀM CẤU HÌNH THIẾT BỊ
// ============================================

void loadDeviceConfig() {
  // Đọc từ EEPROM
  EEPROM.get(EEPROM_CONFIG_ADDR, deviceConfig);
  
  // Validate dữ liệu
  bool needReset = false;
  
  if (!deviceConfig.valid) {
    Serial.println("[CONFIG] Invalid config flag, resetting...");
    needReset = true;
  } else {
    // Kiểm tra deviceId và userId có hợp lệ không (không phải ký tự lạ)
    bool deviceIdValid = true;
    bool userIdValid = true;
    
    // Kiểm tra null-terminated và không có ký tự lạ
    for (int i = 0; i < sizeof(deviceConfig.deviceId) - 1; i++) {
      if (deviceConfig.deviceId[i] == '\0') break;
      if (deviceConfig.deviceId[i] < 32 || deviceConfig.deviceId[i] > 126) {
        deviceIdValid = false;
        break;
      }
    }
    
    for (int i = 0; i < sizeof(deviceConfig.userId) - 1; i++) {
      if (deviceConfig.userId[i] == '\0') break;
      if (deviceConfig.userId[i] < 32 || deviceConfig.userId[i] > 126) {
        userIdValid = false;
        break;
      }
    }
    
    // Đảm bảo null-terminated
    deviceConfig.deviceId[sizeof(deviceConfig.deviceId) - 1] = '\0';
    deviceConfig.userId[sizeof(deviceConfig.userId) - 1] = '\0';
    
    if (!deviceIdValid || !userIdValid || strlen(deviceConfig.deviceId) == 0 || strlen(deviceConfig.userId) == 0) {
      Serial.println("[CONFIG] Invalid deviceId/userId, resetting...");
      Serial.printf("[CONFIG] deviceId: %s, userId: %s\n", deviceConfig.deviceId, deviceConfig.userId);
      needReset = true;
    }
  }
  
  if (needReset) {
    setDefaultConfig();
    saveDeviceConfig();
  }
  
  // Đảm bảo userId luôn là "U0001" (theo yêu cầu)
  if (strcmp(deviceConfig.userId, DEFAULT_USER_ID) != 0) {
    Serial.printf("[CONFIG] Updating userId from '%s' to '%s'\n", deviceConfig.userId, DEFAULT_USER_ID);
    strncpy(deviceConfig.userId, DEFAULT_USER_ID, sizeof(deviceConfig.userId) - 1);
    deviceConfig.userId[sizeof(deviceConfig.userId) - 1] = '\0';
    saveDeviceConfig();
  }
  
  // In ra config hiện tại
  Serial.println("[CONFIG] Loaded config:");
  Serial.printf("  deviceId: %s\n", deviceConfig.deviceId);
  Serial.printf("  userId: %s\n", deviceConfig.userId);
  Serial.printf("  accidentThreshold (linear): %.2f g\n", deviceConfig.accidentThreshold);
  Serial.printf("  angularAccelThreshold: %.2f deg/s²\n", deviceConfig.angularAccelThreshold);
  Serial.printf("  minSpeedForAccident: %.2f km/h\n", deviceConfig.minSpeedForAccident);
  Serial.printf("  tempThreshold: %.1f°C\n", deviceConfig.tempThreshold);
  Serial.printf("  humiThreshold: %.1f%%\n", deviceConfig.humiThreshold);
  Serial.printf("  sosPhoneNumbers: %s\n", deviceConfig.sosPhoneNumbers);
}

void saveDeviceConfig() {
  deviceConfig.valid = true;
  EEPROM.put(EEPROM_CONFIG_ADDR, deviceConfig);
  EEPROM.commit();
}

void setDefaultConfig() {
  // Clear toàn bộ struct trước
  memset(&deviceConfig, 0, sizeof(deviceConfig));
  
  // Set giá trị mặc định
  strncpy(deviceConfig.deviceId, DEFAULT_DEVICE_ID, sizeof(deviceConfig.deviceId) - 1);
  deviceConfig.deviceId[sizeof(deviceConfig.deviceId) - 1] = '\0';
  
  strncpy(deviceConfig.userId, DEFAULT_USER_ID, sizeof(deviceConfig.userId) - 1);
  deviceConfig.userId[sizeof(deviceConfig.userId) - 1] = '\0';
  
  deviceConfig.accidentThreshold = DEFAULT_ACCIDENT_THRESHOLD_G;
  deviceConfig.angularAccelThreshold = DEFAULT_ANGULAR_ACCEL_THRESHOLD;
  deviceConfig.minSpeedForAccident = DEFAULT_MIN_SPEED_FOR_ACCIDENT;
  deviceConfig.tempThreshold = DEFAULT_TEMP_THRESHOLD;
  deviceConfig.humiThreshold = DEFAULT_HUMI_THRESHOLD;
  
  strncpy(deviceConfig.sosPhoneNumbers, "0964380284", sizeof(deviceConfig.sosPhoneNumbers) - 1);
  deviceConfig.sosPhoneNumbers[sizeof(deviceConfig.sosPhoneNumbers) - 1] = '\0';
  
  deviceConfig.valid = true;
  
  Serial.println("[CONFIG] Default config set");
  Serial.printf("  - Linear Accel Threshold: %.2f g\n", deviceConfig.accidentThreshold);
  Serial.printf("  - Angular Accel Threshold: %.2f deg/s²\n", deviceConfig.angularAccelThreshold);
  Serial.printf("  - Min Speed for Accident: %.2f km/h\n", deviceConfig.minSpeedForAccident);
}

// ============================================
// HÀM KHỞI TẠO
// ============================================

void initMPU9250() {
  Serial.println("Initializing MPU9250...");
  
  // Kiểm tra kết nối MPU9250
  Wire.beginTransmission(MPU9250_ADDRESS);
  if (Wire.endTransmission() != 0) {
    Serial.println("ERROR: MPU9250 not found at address 0x69!");
    Serial.println("Check I2C connections (SDA: GPIO 8, SCL: GPIO 9)");
    return;
  }
  
  // Kiểm tra WHO_AM_I register
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(MPU9250_WHO_AM_I);
  if (Wire.endTransmission() != 0) {
    Serial.println("ERROR: Cannot read WHO_AM_I from MPU9250!");
    return;
  }
  
  Wire.requestFrom(MPU9250_ADDRESS, 1);
  if (Wire.available()) {
    byte whoAmI = Wire.read();
    Serial.printf("MPU9250 WHO_AM_I: 0x%02X\n", whoAmI);
    // MPU9250: 0x71, MPU6500: 0x70
    if (whoAmI != 0x71 && whoAmI != 0x70) {
      Serial.printf("WARNING: Unexpected WHO_AM_I value: 0x%02X\n", whoAmI);
    }
  }
  
  // Reset MPU9250
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(MPU9250_PWR_MGMT_1);
  Wire.write(0x80);  // Reset bit
  Wire.endTransmission();
  delay(100);
  
  // Wake up MPU9250 (clear sleep mode)
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(MPU9250_PWR_MGMT_1);
  Wire.write(0x00);  // Clear sleep bit
  Wire.endTransmission();
  delay(10);
  
  // Cấu hình Accelerometer: ±2g (độ chính xác cao)
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(MPU9250_ACCEL_CONFIG);
  Wire.write(0x00);  // ±2g (AFS_SEL = 0)
  Wire.endTransmission();
  delay(10);
  
  // Cấu hình Gyroscope: ±250°/s (độ nhạy cao)
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(MPU9250_GYRO_CONFIG);
  Wire.write(0x00);  // ±250°/s (FS_SEL = 0)
  Wire.endTransmission();
  delay(10);
  
  // Cấu hình DLPF (Digital Low Pass Filter)
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(MPU9250_CONFIG);
  Wire.write(0x03);  // DLPF_CFG = 3 (44Hz cho accel, 42Hz cho gyro)
  Wire.endTransmission();
  delay(10);
  
  // Cấu hình sample rate
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(MPU9250_SMPLRT_DIV);
  Wire.write(0x04);  // Sample rate = 1kHz / (1 + 4) = 200Hz
  Wire.endTransmission();
  delay(10);
  
  Serial.println("MPU9250 initialized successfully at address 0x69");
}

// TẠM THỜI BỎ GHI ÂM
/*
void initI2SAudio() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = AUDIO_SAMPLE_RATE,
    .bits_per_sample = (i2s_bits_per_sample_t)AUDIO_BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = AUDIO_BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  
  // Bắt đầu ghi âm
  systemState.audioRecording = true;
  DateTime now = rtc.now();
  char fileName[64];
  snprintf(fileName, sizeof(fileName), "/AUDIO_%04d%02d%02d_%02d%02d%02d.wav",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  currentAudioFile = SD.open(fileName, FILE_WRITE);
  if (currentAudioFile) {
    writeWAVHeader(currentAudioFile, AUDIO_SAMPLE_RATE, AUDIO_BITS_PER_SAMPLE, AUDIO_CHANNELS);
    audioFileOpen = true;
    systemState.audioFileStartTime = millis();
  }
  
  Serial.println("I2S Audio initialized");
}
*/

void initSIM4G() {
  Serial.println("\n=== INITIALIZING SIM 4G MODULE ===");
  
  // Log vào Serial
  Serial.println("[SIM] Starting initialization...");
  Serial.printf("[SIM] APN: %s\n", apn);
  
  // Gửi lệnh AT để kiểm tra module
  SerialSIM.println("AT");
  delay(500);
  String response = "";
  unsigned long startTime = millis();
  while (millis() - startTime < 2000) {
    if (SerialSIM.available()) {
      char c = SerialSIM.read();
      response += c;
      Serial.print(c);  // Echo response
    }
  }
  
  if (response.indexOf("OK") >= 0) {
    Serial.println("[SIM] Module responded to AT");
    logSIMToSD("INIT", "AT_OK", "Module responded");
  } else {
    Serial.println("[SIM] ERROR: No response to AT");
    logSIMToSD("INIT", "AT_ERROR", "No response");
  }
  
  // Kiểm tra SIM card
  SerialSIM.println("AT+CPIN?");
  delay(500);
  response = "";
  startTime = millis();
  while (millis() - startTime < 2000) {
    if (SerialSIM.available()) {
      char c = SerialSIM.read();
      response += c;
      Serial.print(c);
    }
  }
  
  if (response.indexOf("READY") >= 0 || response.indexOf("OK") >= 0) {
    Serial.println("[SIM] SIM card detected");
    logSIMToSD("INIT", "SIM_OK", "SIM card ready");
  } else {
    Serial.println("[SIM] WARNING: SIM card status unclear");
    logSIMToSD("INIT", "SIM_WARN", "Status unclear");
  }
  
  // Kiểm tra tín hiệu
  SerialSIM.println("AT+CSQ");
  delay(500);
  response = "";
  startTime = millis();
  while (millis() - startTime < 2000) {
    if (SerialSIM.available()) {
      char c = SerialSIM.read();
      response += c;
      Serial.print(c);
    }
  }
  
  Serial.println("[SIM] Signal check completed");
  logSIMToSD("INIT", "SIGNAL_CHECK", response.substring(0, 50).c_str());
  
  Serial.println("[SIM] SIM 4G initialization completed");
  logSIMToSD("INIT", "DONE", "Initialization completed");
}

// TẠM THỜI BỎ GHI ÂM - Giữ lại hàm để dùng sau
/*
void writeWAVHeader(File& file, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels) {
  // WAV header (44 bytes)
  file.write((const uint8_t*)"RIFF", 4);
  uint32_t fileSize = 0; // Will be updated later
  file.write((uint8_t*)&fileSize, 4);
  file.write((const uint8_t*)"WAVE", 4);
  file.write((const uint8_t*)"fmt ", 4);
  uint32_t fmtSize = 16;
  file.write((uint8_t*)&fmtSize, 4);
  uint16_t audioFormat = 1; // PCM
  file.write((uint8_t*)&audioFormat, 2);
  file.write((uint8_t*)&channels, 2);
  file.write((uint8_t*)&sampleRate, 4);
  uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
  file.write((uint8_t*)&byteRate, 4);
  uint16_t blockAlign = channels * bitsPerSample / 8;
  file.write((uint8_t*)&blockAlign, 2);
  file.write((uint8_t*)&bitsPerSample, 2);
  file.write((const uint8_t*)"data", 4);
  uint32_t dataSize = 0; // Will be updated later
  file.write((uint8_t*)&dataSize, 4);
}
*/

// ============================================
// HÀM SIM 4G (STUB)
// ============================================

void sendSMS(const char* phoneNumber, const char* message) {
  if (!sim4gInitialized) {
    Serial.println("[SMS] SIM 4G not initialized!");
    return;
  }
  
  Serial.printf("[SMS] Sending SMS to %s: %s\n", phoneNumber, message);
  logSIMToSD("SMS", phoneNumber, message);
  
  // Gửi AT command để gửi SMS
  SerialSIM.print("AT+CMGS=\"");
  SerialSIM.print(phoneNumber);
  SerialSIM.println("\"");
  delay(1500);  // Đợi prompt "> "
  
  // Đọc prompt "> " từ modem (nếu có)
  String prompt = SerialSIM.readString();
  if (prompt.length() > 0) {
    Serial.printf("[SMS] Prompt: %s\n", prompt.c_str());
  }
  
  // Gửi nội dung SMS (giới hạn 160 ký tự cho SMS 7-bit)
  String smsMessage = String(message);
  if (smsMessage.length() > 160) {
    smsMessage = smsMessage.substring(0, 160);
  }
  SerialSIM.print(smsMessage);
  delay(500);
  
  // Gửi Ctrl+Z để kết thúc
  SerialSIM.write(0x1A);
  delay(3000);  // Đợi response
  
  // Đọc phản hồi
  String response = "";
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {  // Timeout 5 giây
    if (SerialSIM.available()) {
      char c = SerialSIM.read();
      response += c;
      if (response.length() > 512) {
        response = response.substring(response.length() - 512);
      }
    }
    delay(10);
  }
  
  Serial.printf("[SMS] Response: %s\n", response.c_str());
  
  if (response.indexOf("+CMGS:") != -1 || response.indexOf("OK") != -1) {
    Serial.println("[SMS] SMS sent successfully!");
    logSIMToSD("SMS", phoneNumber, "SUCCESS");
  } else if (response.indexOf("+CMS ERROR") != -1) {
    Serial.println("[SMS] SMS send failed!");
    logSIMToSD("SMS", phoneNumber, "FAILED");
  }
}

void makeCall(const char* phoneNumber) {
  if (!sim4gInitialized) {
    Serial.println("[CALL] SIM 4G not initialized!");
    return;
  }
  
  Serial.printf("[CALL] Calling %s...\n", phoneNumber);
  logSIMToSD("CALL", phoneNumber, "Initiating");
  
  // Gửi AT command để gọi điện
  SerialSIM.print("ATD");
  SerialSIM.print(phoneNumber);
  SerialSIM.println(";");  // Dấu ; để gọi voice call
  delay(2000);
  
  // Đọc phản hồi
  String response = "";
  unsigned long startTime = millis();
  while (millis() - startTime < 3000) {
    if (SerialSIM.available()) {
      char c = SerialSIM.read();
      response += c;
      if (response.length() > 512) {
        response = response.substring(response.length() - 512);
      }
    }
    delay(10);
  }
  
  Serial.printf("[CALL] Response: %s\n", response.c_str());
  
  if (response.indexOf("OK") != -1 || response.indexOf("CONNECT") != -1) {
    Serial.println("[CALL] Call initiated successfully!");
    logSIMToSD("CALL", phoneNumber, "CONNECTED");
    
    // Gọi điện thường kéo dài, có thể tự ngắt sau một thời gian (tùy chọn)
    // delay(30000);  // Gọi 30 giây
    // SerialSIM.println("ATH");  // Hang up
  } else if (response.indexOf("BUSY") != -1) {
    Serial.println("[CALL] Number is busy!");
    logSIMToSD("CALL", phoneNumber, "BUSY");
  } else if (response.indexOf("NO ANSWER") != -1) {
    Serial.println("[CALL] No answer!");
    logSIMToSD("CALL", phoneNumber, "NO_ANSWER");
  } else if (response.indexOf("NO CARRIER") != -1) {
    Serial.println("[CALL] No carrier!");
    logSIMToSD("CALL", phoneNumber, "NO_CARRIER");
  } else {
    Serial.println("[CALL] Call status unknown");
    logSIMToSD("CALL", phoneNumber, "UNKNOWN");
  }
}

void sendHTTPPost(const char* endpoint, const char* jsonData) {
  unsigned long startTime = millis();
  Serial.println("\n[HTTP_POST] ===== Starting HTTP POST =====");
  
  // Kiểm tra trạng thái mạng
  Serial.printf("[HTTP_POST] Checking network status...\n");
  Serial.printf("[HTTP_POST] SIM4G initialized: %s\n", sim4gInitialized ? "YES" : "NO");
  Serial.printf("[HTTP_POST] Network open: %s\n", sim4gNetworkOpen ? "YES" : "NO");
  
  if (!sim4gInitialized || !sim4gNetworkOpen) {
    Serial.println("[HTTP_POST] ✗ ERROR: SIM4G not initialized or network not open, skipping POST...");
    logSIMToSD("HTTP_POST", endpoint, "NOT_INITIALIZED");
    return;
  }
  
  // Kiểm tra GPRS connection - retry nếu chưa connect
  if (!modem.isGprsConnected()) {
    Serial.println("[HTTP_POST] GPRS not connected, attempting to connect...");
    if (!modem.gprsConnect(apn, user, pass)) {
      Serial.println("[HTTP_POST] ✗ ERROR: Failed to connect GPRS");
      logSIMToSD("HTTP_POST", endpoint, "GPRS_CONNECT_FAILED");
      return;
    }
    delay(2000);  // Đợi connection ổn định
    Serial.println("[HTTP_POST] ✓ GPRS connected");
  }
  
  Serial.printf("[HTTP_POST] GPRS connected: %s\n", modem.isGprsConnected() ? "YES" : "NO");
  Serial.printf("[HTTP_POST] Server: %s:%d\n", server, serverPort);
  Serial.printf("[HTTP_POST] Endpoint: %s\n", endpoint);
  Serial.printf("[HTTP_POST] Full URL: http://%s:%d%s\n", server, serverPort, endpoint);
  Serial.printf("[HTTP_POST] JSON size: %d bytes\n", strlen(jsonData));
  Serial.printf("[HTTP_POST] JSON data: %s\n", jsonData);
  logSIMToSD("HTTP_POST", endpoint, "Sending");
  
  // Tạo HttpClient với timeout
  Serial.println("[HTTP_POST] Creating HttpClient...");
  HttpClient httpClient(client, server, serverPort);
  httpClient.setTimeout(10000);  // Timeout 10 giây
  
  // Gửi POST request với retry
  int maxRetries = 3;
  int statusCode = 0;
  String response = "";
  bool success = false;
  
  for (int retry = 0; retry < maxRetries; retry++) {
    if (retry > 0) {
      Serial.printf("[HTTP_POST] Retry %d/%d...\n", retry, maxRetries - 1);
      delay(1000 * retry);  // Exponential backoff
    }
    
    Serial.println("[HTTP_POST] Sending POST request...");
    unsigned long postStartTime = millis();
    
    // Gửi POST request
    int postResult = httpClient.post(endpoint, "application/json", jsonData);
    unsigned long postTime = millis() - postStartTime;
    Serial.printf("[HTTP_POST] POST request sent in %lu ms (result: %d)\n", postTime, postResult);
    
    if (postResult < 0) {
      Serial.printf("[HTTP_POST] ✗ POST request failed (result: %d)\n", postResult);
      continue;
    }
    
    // Đọc status code với timeout
    Serial.println("[HTTP_POST] Reading response status code...");
    unsigned long responseStartTime = millis();
    statusCode = httpClient.responseStatusCode();
    unsigned long statusTime = millis() - responseStartTime;
    Serial.printf("[HTTP_POST] Status code received in %lu ms: %d\n", statusTime, statusCode);
    
    if (statusCode <= 0) {
      Serial.printf("[HTTP_POST] ✗ Invalid status code: %d\n", statusCode);
      continue;
    }
    
    // Đọc response body
    Serial.println("[HTTP_POST] Reading response body...");
    response = httpClient.responseBody();
    unsigned long responseTime = millis() - responseStartTime;
    Serial.printf("[HTTP_POST] Full response received in %lu ms (%d bytes)\n", responseTime, response.length());
    Serial.printf("[HTTP_POST] Response: %s\n", response.c_str());
    logSIMToSD("HTTP_POST", endpoint, response.substring(0, 50).c_str());
    
    if (statusCode == 200 || statusCode == 201 || statusCode == 204) {
      success = true;
      break;
    } else {
      Serial.printf("[HTTP_POST] ✗ POST failed with status %d (retry %d/%d)\n", statusCode, retry + 1, maxRetries);
    }
  }
  
  // Parse JSON response để lưu SOS ID nếu là POST /api/sos
  if (success) {
    Serial.println("[HTTP_POST] ✓ POST successful!");
    
    if (strstr(endpoint, "/api/sos") != NULL && response.length() > 0) {
      Serial.println("[HTTP_POST] Parsing SOS response...");
      // Parse JSON response để lấy SOS ID
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, response);
      
      if (error) {
        Serial.printf("[HTTP_POST] ✗ JSON parse error: %s\n", error.c_str());
        Serial.printf("[HTTP_POST] Raw response: %s\n", response.c_str());
      } else {
        if (doc.containsKey("id")) {
          String sosId = doc["id"].as<String>();
          
          // Lưu SOS ID vào systemState
          if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            systemState.currentSOSId = sosId;
            xSemaphoreGive(systemStateMutex);
          }
          
          Serial.printf("[HTTP_POST] ✓ SOS ID saved: %s\n", sosId.c_str());
        } else {
          Serial.println("[HTTP_POST] ✗ Response does not contain 'id' field");
          Serial.printf("[HTTP_POST] Response keys: ");
          for (JsonPair kv : doc.as<JsonObject>()) {
            Serial.printf("%s ", kv.key().c_str());
          }
          Serial.println();
        }
      }
    }
  } else {
    Serial.printf("[HTTP_POST] ✗ POST failed after %d retries (final status: %d)\n", maxRetries, statusCode);
  }
  
  // Đóng connection
  httpClient.stop();
  Serial.printf("[HTTP_POST] Connection closed\n");
  
  unsigned long totalTime = millis() - startTime;
  Serial.printf("[HTTP_POST] Total time: %lu ms\n", totalTime);
  Serial.println("[HTTP_POST] ===== End HTTP POST =====\n");
}

// Gửi HTTP GET request
void sendHTTPGet(const char* endpoint) {
  unsigned long startTime = millis();
  Serial.println("\n[HTTP_GET] ===== Starting HTTP GET =====");
  
  // Kiểm tra trạng thái mạng
  Serial.printf("[HTTP_GET] Checking network status...\n");
  Serial.printf("[HTTP_GET] SIM4G initialized: %s\n", sim4gInitialized ? "YES" : "NO");
  Serial.printf("[HTTP_GET] Network open: %s\n", sim4gNetworkOpen ? "YES" : "NO");
  Serial.printf("[HTTP_GET] GPRS connected: %s\n", modem.isGprsConnected() ? "YES" : "NO");
  
  if (!sim4gInitialized || !sim4gNetworkOpen || !modem.isGprsConnected()) {
    Serial.println("[HTTP_GET] ✗ ERROR: Network not connected, skipping GET...");
    return;
  }
  
  Serial.printf("[HTTP_GET] Server: %s:%d\n", server, serverPort);
  Serial.printf("[HTTP_GET] Endpoint: %s\n", endpoint);
  
  // Tạo HttpClient
  Serial.println("[HTTP_GET] Creating HttpClient...");
  HttpClient httpClient(client, server, serverPort);
  
  // Gửi GET request
  Serial.println("[HTTP_GET] Sending GET request...");
  unsigned long getStartTime = millis();
  httpClient.get(endpoint);
  unsigned long getTime = millis() - getStartTime;
  Serial.printf("[HTTP_GET] GET request sent in %lu ms\n", getTime);
  
  // Đọc status code
  Serial.println("[HTTP_GET] Reading response...");
  unsigned long responseStartTime = millis();
  int statusCode = httpClient.responseStatusCode();
  unsigned long statusTime = millis() - responseStartTime;
  Serial.printf("[HTTP_GET] Status code received in %lu ms: %d\n", statusTime, statusCode);
  
  String response = httpClient.responseBody();
  unsigned long responseTime = millis() - responseStartTime;
  Serial.printf("[HTTP_GET] Full response received in %lu ms (%d bytes)\n", responseTime, response.length());
  Serial.printf("[HTTP_GET] Response: %s\n", response.c_str());
  
  // Parse JSON response
  Serial.println("[HTTP_GET] Parsing JSON response...");
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) {
    Serial.printf("[HTTP_GET] ✗ JSON parse error: %s\n", error.c_str());
    httpClient.stop();
    return;
  } else {
    Serial.println("[HTTP_GET] ✓ JSON parsed successfully");
  }
  
  // Đóng connection
  httpClient.stop();
  Serial.printf("[HTTP_GET] Connection closed\n");
  
  unsigned long totalTime = millis() - startTime;
  Serial.printf("[HTTP_GET] Total time: %lu ms\n", totalTime);
  Serial.println("[HTTP_GET] ===== End HTTP GET =====\n");
  
  // Trả về JSON document qua tham chiếu (hoặc return String)
  // Hiện tại chỉ parse và log, caller sẽ tự xử lý
  return;
}

// ============================================
// HÀM LOG MODULE SIM VÀO SD CARD
// ============================================

void logSIMToSD(const char* action, const char* param1, const char* param2) {
  DateTime now = rtc.now();
  char logFileName[32];
  snprintf(logFileName, sizeof(logFileName), "/SIM_LOG_%04d%02d%02d.csv",
           now.year(), now.month(), now.day());
  
  File logFile = SD.open(logFileName, FILE_APPEND);
  if (logFile) {
    logFile.printf("%04d-%02d-%02d %02d:%02d:%02d,",
                  now.year(), now.month(), now.day(),
                  now.hour(), now.minute(), now.second());
    logFile.printf("%s,%s,%s\n", action,                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                