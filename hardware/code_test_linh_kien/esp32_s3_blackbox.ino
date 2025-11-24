/*
 * HỘP ĐEN Ô TÔ / XE MÁY - ESP32-S3
 * 
 * MCU: ESP32-S3 (dual-core)
 * Framework: Arduino + FreeRTOS
 * 
 * PHÂN CHIA CORE:
 * - Core 0: Audio (INMP441) + SD Card (ghi âm, ghi log)
 * - Core 1: Sensors + Logic (MPU, DHT11, GPS, DS1307, SIM 4G, nút, buzzer, OLED)
 * 
 * CHỨC NĂNG:
 * 1. Ghi nhật ký liên tục (GPS, MPU, DHT11, RTC) vào SD card
 * 2. Ghi âm vòng tròn 10 file, mỗi file 3 phút
 * 3. Phát hiện va chạm/tai nạn từ MPU9250
 * 4. Cảnh báo SOS tai nạn (tự động) và cứu hộ (thủ công)
 * 5. Gửi SMS + gọi điện qua SIM 4G
 * 6. Hiển thị trạng thái trên OLED 0.96"
 */

#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <DHT.h>
#include <RTClib.h>
#include <TinyGPS++.h>
#include <U8g2lib.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <ArduinoJson.h>
// TinyGSM for SIM 4G
#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

// ============================================
// ĐỊNH NGHĨA CHÂN GPIO
// ============================================

// I2S Audio (INMP441) - ESP32-S3
#define I2S_WS   8   // LRCL / WS
#define I2S_SD   7   // DOUT from INMP441
#define I2S_SCK  9   // BCLK / SCK

// SD Card (SPI)
#define SD_MOSI  11
#define SD_MISO  13
#define SD_SCK   12
#define SD_CS    10

// I2C (MPU9250, DS1307, OLED, DHT11 có thể dùng I2C hoặc GPIO)
#define I2C_SDA_PIN     21
#define I2C_SCL_PIN     22

// MPU9250 (I2C, address 0x69) - AD0 nối lên 3.3V (HIGH) để đổi địa chỉ từ 0x68 sang 0x69
// DS1307 (I2C, address 0x68) - Không xung đột với MPU9250 nữa
// OLED SSD1306 (I2C, address 0x3C)

// DHT11
#define DHT_PIN         2
#define DHT_TYPE        DHT11

// GPS NEO-8M (UART)
#define GPS_RX_PIN      16
#define GPS_TX_PIN      20  // Đổi sang GPIO 20 để nhường GPIO 17 cho SIM

// SIM 4G (UART) - TinyGSM - Dùng GPIO 17, 18
#define SIM_RX_PIN      18  // Nhận từ SIM TX
#define SIM_TX_PIN      17  // Gửi đến SIM RX
#define SerialSIM       Serial2

// Nút nhấn
#define BUTTON_1_PIN    0   // Nút SOS tai nạn
#define BUTTON_2_PIN    35  // Nút SOS cứu hộ

// Buzzer
#define BUZZER_PIN      25

// ============================================
// THAM SỐ HỆ THỐNG
// ============================================

#define AUDIO_SAMPLE_RATE   16000
#define AUDIO_BITS_PER_SAMPLE 16
#define AUDIO_CHANNELS      1
#define AUDIO_FILE_DURATION_SEC 180  // 3 phút
#define AUDIO_MAX_FILES     10
#define AUDIO_BUFFER_SIZE   1024

#define LOG_INTERVAL_MS     1000  // Ghi log mỗi 1 giây
#define OLED_UPDATE_MS      100   // Cập nhật OLED mỗi 100ms
#define TELEMETRY_INTERVAL_MS 30000  // Gửi telemetry mỗi 30 giây

#define ACCIDENT_THRESHOLD_G    2.5f  // Ngưỡng gia tốc phát hiện tai nạn (g)
#define ACCIDENT_COUNTDOWN_SEC  30    // Đếm ngược 30 giây
#define BUTTON_HOLD_CANCEL_MS   5000  // Nhấn giữ 5 giây để hủy

// Cấu hình Endpoints
#define TELEMETRY_ENDPOINT  "/api/telemetry"
#define SOS_ENDPOINT        "/api/sos"
#define USER_ID             "U0001"  // ID user trong hệ thống
#define DEVICE_ID           "DHW001" // ID thiết bị
#define SOS_PHONE_NUMBER    "0335587155"

// ============================================
// BIẾN TOÀN CỤC & STRUCT
// ============================================

// Cấu trúc dữ liệu sensor
struct SensorData {
  float lat, lon, speed;      // GPS
  float ax, ay, az;           // Gia tốc MPU
  float gx, gy, gz;           // Góc quay MPU
  float temp, humi;           // DHT11
  DateTime rtcTime;           // DS1307
  bool valid;
};

// Trạng thái hệ thống
enum SystemStatus {
  STATUS_NORMAL,
  STATUS_ACCIDENT_COUNTDOWN,
  STATUS_ACCIDENT_SOS_SENT,
  STATUS_RESCUE_COUNTDOWN,
  STATUS_RESCUE_SOS_SENT,
  STATUS_SOS_CANCELED
};

// Cấu trúc trạng thái hệ thống
struct SystemState {
  SystemStatus status;
  unsigned long countdownStart;
  int countdownRemaining;
  bool buzzerOn;
  SensorData lastSensorData;
  char logFileName[32];
  int audioFileIndex;
  unsigned long audioFileStartTime;
  bool audioRecording;
};

// Khởi tạo đối tượng
DHT dht(DHT_PIN, DHT_TYPE);
RTC_DS1307 rtc;
TinyGPSPlus gps;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8G2_I2C_OPT_NONE);

// Serial cho GPS và SIM
HardwareSerial SerialGPS(1);
HardwareSerial SerialSIM(2);

// TinyGSM objects
TinyGsm modem(SerialSIM);
TinyGsmClient client(modem);

// APN configuration
const char apn[] = "m3-world";  // Đổi theo nhà mạng (CMNET, m3-world, v.v.)
const char user[] = "";
const char pass[] = "";

// Backend server
const char server[] = "api.hopdenthongminh.cloud";  // API subdomain
const int serverPort = 80;  // HTTP port (Cloudflare tự động redirect sang HTTPS)

// Biến toàn cục
SystemState systemState;
SensorData currentSensorData;
SemaphoreHandle_t sensorDataMutex;
SemaphoreHandle_t systemStateMutex;
QueueHandle_t smsQueue;
QueueHandle_t callQueue;
QueueHandle_t httpQueue;  // Queue cho HTTP requests

// Cấu trúc HTTP request
struct HTTPRequest {
  char endpoint[64];
  char jsonData[512];
};

// Biến SIM 4G
bool sim4gInitialized = false;
bool sim4gNetworkOpen = false;
String authToken = "";  // Token sau khi login (nếu cần)
unsigned long lastTelemetrySend = 0;

// Biến để tính vận tốc offline từ GPS
float lastGPSLat = 0;
float lastGPSLon = 0;
unsigned long lastGPSTime = 0;
bool hasLastGPS = false;

// Buffer cho audio
int16_t audioBuffer[AUDIO_BUFFER_SIZE];
File currentAudioFile;
bool audioFileOpen = false;

// ============================================
// HÀM KHỞI TẠO
// ============================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== HOP DEN O TO / XE MAY - ESP32-S3 ===");
  
  // Khởi tạo mutex và queue
  sensorDataMutex = xSemaphoreCreateMutex();
  systemStateMutex = xSemaphoreCreateMutex();
  smsQueue = xQueueCreate(5, sizeof(char[160]));
  callQueue = xQueueCreate(5, sizeof(char[20]));
  httpQueue = xQueueCreate(10, sizeof(HTTPRequest));  // Queue cho HTTP POST data
  
  // Khởi tạo trạng thái hệ thống
  systemState.status = STATUS_NORMAL;
  systemState.countdownStart = 0;
  systemState.countdownRemaining = 0;
  systemState.buzzerOn = false;
  systemState.audioFileIndex = 0;
  systemState.audioFileStartTime = 0;
  systemState.audioRecording = false;
  
  // Khởi tạo GPIO
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Khởi tạo I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  
  // Khởi tạo DHT11
  dht.begin();
  Serial.println("DHT11 initialized");
  
  // Khởi tạo RTC DS1307
  if (!rtc.begin()) {
    Serial.println("ERROR: RTC DS1307 not found!");
  } else {
    if (!rtc.isrunning()) {
      Serial.println("RTC is NOT running, setting time...");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    Serial.println("RTC DS1307 initialized");
  }
  
  // Khởi tạo MPU9250
  initMPU9250();
  
  // Khởi tạo GPS
  SerialGPS.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS Serial initialized");
  
  // Khởi tạo SIM 4G với TinyGSM
  SerialSIM.begin(115200, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);
  delay(2000);
  
  // Reset modem
  Serial.println("Modem Reset, Please Wait");
  SerialSIM.println("AT+CRESET");
  delay(2000);
  SerialSIM.flush();
  
  // Echo off
  Serial.println("Echo Off");
  SerialSIM.println("ATE0");
  delay(1000);
  
  initSIM4G();
  
  // Khởi tạo OLED
  oled.begin();
  oled.setFont(u8g2_font_ncenB08_tr);
  oled.clearBuffer();
  oled.drawStr(0, 20, "HOP DEN");
  oled.drawStr(0, 40, "Initializing...");
  oled.sendBuffer();
  Serial.println("OLED initialized");
  
  // Khởi tạo SD Card
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR: SD Card initialization failed!");
  } else {
    Serial.println("SD Card initialized");
    createLogFile();
  }
  
  // Khởi tạo I2S cho audio
  initI2S();
  Serial.println("I2S initialized");
  
  // Tạo các task FreeRTOS
  // Core 0: Audio + Logger
  xTaskCreatePinnedToCore(taskAudio, "TaskAudio", 8192, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(taskLogger, "TaskLogger", 4096, NULL, 2, NULL, 0);
  
  // Core 1: Sensors + Logic + SIM + OLED
  xTaskCreatePinnedToCore(taskSensors, "TaskSensors", 8192, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(taskSIM4G, "TaskSIM4G", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskOLED, "TaskOLED", 4096, NULL, 1, NULL, 1);
  
  Serial.println("All tasks created. System ready!");
}

void loop() {
  // Loop trống, tất cả logic chạy trong FreeRTOS tasks
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// ============================================
// KHỞI TẠO MPU9250
// ============================================

void initMPU9250() {
  // Kiểm tra kết nối MPU9250
  // Địa chỉ 0x69 vì AD0 nối lên 3.3V (HIGH)
  Wire.beginTransmission(0x69); // MPU9250 I2C address (0x69 - AD0 = HIGH)
  if (Wire.endTransmission() != 0) {
    Serial.println("ERROR: MPU9250 not found at address 0x69!");
    Serial.println("Check AD0 pin connection (should be HIGH/3.3V)");
    return;
  }
  
  // Wake up MPU9250 (PWR_MGMT_1 register)
  Wire.beginTransmission(0x69);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0x00); // Wake up (clear sleep bit)
  Wire.endTransmission();
  delay(10);
  
  // Configure accelerometer range (±2g) - Độ chính xác cao hơn
  Wire.beginTransmission(0x69);
  Wire.write(0x1C); // ACCEL_CONFIG register
  Wire.write(0x00); // ±2g (AFS_SEL = 0)
  Wire.endTransmission();
  delay(10);
  
  // Configure gyroscope range (±250°/s) - Độ nhạy cao
  Wire.beginTransmission(0x69);
  Wire.write(0x1B); // GYRO_CONFIG register
  Wire.write(0x00); // ±250°/s (FS_SEL = 0)
  Wire.endTransmission();
  delay(10);
  
  // Configure DLPF (Digital Low Pass Filter) cho độ chính xác tốt hơn
  Wire.beginTransmission(0x69);
  Wire.write(0x1A); // CONFIG register
  Wire.write(0x03); // DLPF_CFG = 3 (44Hz cho accel, 42Hz cho gyro)
  Wire.endTransmission();
  delay(10);
  
  // Configure sample rate (1kHz)
  Wire.beginTransmission(0x69);
  Wire.write(0x19); // SMPLRT_DIV register
  Wire.write(0x04); // Sample rate = 1kHz / (1 + 4) = 200Hz
  Wire.endTransmission();
  delay(10);
  
  Serial.println("MPU9250 initialized at address 0x69 (AD0 = HIGH)");
}

// ============================================
// ĐỌC DỮ LIỆU TỪ CẢM BIẾN
// ============================================

void readMPU9250(SensorData* data) {
  Wire.beginTransmission(0x69); // MPU9250 address 0x69 (AD0 = HIGH)
  Wire.write(0x3B); // ACCEL_XOUT_H register (đọc từ đây)
  Wire.endTransmission(false);
  Wire.requestFrom(0x69, 14, true); // Đọc 14 bytes (6 accel + 2 temp + 6 gyro)
  
  // Đọc dữ liệu accelerometer (16-bit, big-endian)
  int16_t accelX = (Wire.read() << 8 | Wire.read());
  int16_t accelY = (Wire.read() << 8 | Wire.read());
  int16_t accelZ = (Wire.read() << 8 | Wire.read());
  
  // Bỏ qua temperature (2 bytes)
  Wire.read();
  Wire.read();
  
  // Đọc dữ liệu gyroscope (16-bit, big-endian)
  int16_t gyroX = (Wire.read() << 8 | Wire.read());
  int16_t gyroY = (Wire.read() << 8 | Wire.read());
  int16_t gyroZ = (Wire.read() << 8 | Wire.read());
  
  // Convert to g (accel) and °/s (gyro)
  // MPU9250 scale factors (giống MPU6500):
  // Accelerometer: ±2g = 16384 LSB/g
  // Gyroscope: ±250°/s = 131 LSB/°/s
  data->ax = accelX / 16384.0f;
  data->ay = accelY / 16384.0f;
  data->az = accelZ / 16384.0f;
  data->gx = gyroX / 131.0f;
  data->gy = gyroY / 131.0f;
  data->gz = gyroZ / 131.0f;
  
  // MPU9250 có độ chính xác cao hơn MPU6500, đặc biệt ở gia tốc thấp
}

void readDHT11(SensorData* data) {
  data->temp = dht.readTemperature();
  data->humi = dht.readHumidity();
  if (isnan(data->temp) || isnan(data->humi)) {
    data->temp = -999;
    data->humi = -999;
  }
}

// Hàm tính khoảng cách giữa 2 điểm GPS (Haversine formula)
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

// Hàm tính vận tốc từ 2 điểm GPS và thời gian
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
          // Tính vận tốc offline từ 2 điểm GPS liên tiếp
          unsigned long currentTime = millis();
          if (hasLastGPS && (currentTime - lastGPSTime) > 1000) {  // Ít nhất 1 giây
            float calculatedSpeed = calculateSpeedFromGPS(
              lastGPSLat, lastGPSLon, lastGPSTime,
              data->lat, data->lon, currentTime
            );
            data->speed = calculatedSpeed;
            Serial.printf("[GPS] Calculated speed: %.2f km/h (from GPS points)\n", calculatedSpeed);
          } else {
            data->speed = 0.0;  // Chưa đủ dữ liệu để tính
          }
        }
        
        // Cập nhật điểm GPS trước để tính vận tốc lần sau
        lastGPSLat = data->lat;
        lastGPSLon = data->lon;
        lastGPSTime = millis();
        hasLastGPS = true;
        
        data->valid = true;
      } else {
        data->valid = false;
      }
    }
  }
}

void readRTC(SensorData* data) {
  DateTime now = rtc.now();
  data->rtcTime = now;
}

// ============================================
// TASK: AUDIO (Core 0)
// ============================================

void taskAudio(void* parameter) {
  Serial.println("TaskAudio started on Core 0");
  
  while (true) {
    if (!audioFileOpen) {
      openNextAudioFile();
    }
    
    // Đọc dữ liệu từ I2S
    size_t bytesRead;
    i2s_read(I2S_NUM_0, audioBuffer, AUDIO_BUFFER_SIZE * sizeof(int16_t), &bytesRead, portMAX_DELAY);
    
    if (bytesRead > 0 && audioFileOpen) {
      currentAudioFile.write((uint8_t*)audioBuffer, bytesRead);
      currentAudioFile.flush();
      
      // Kiểm tra thời gian ghi file
      unsigned long currentTime = millis();
      if (currentTime - systemState.audioFileStartTime >= (AUDIO_FILE_DURATION_SEC * 1000)) {
        closeAudioFile();
        systemState.audioFileIndex = (systemState.audioFileIndex + 1) % AUDIO_MAX_FILES;
        systemState.audioFileStartTime = currentTime;
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void initI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = AUDIO_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = AUDIO_BUFFER_SIZE,
    .use_apll = false
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

void openNextAudioFile() {
  char fileName[32];
  snprintf(fileName, sizeof(fileName), "/AUDIO_%02d.WAV", systemState.audioFileIndex);
  
  // Xóa file cũ nếu tồn tại
  if (SD.exists(fileName)) {
    SD.remove(fileName);
  }
  
  currentAudioFile = SD.open(fileName, FILE_WRITE);
  if (currentAudioFile) {
    writeWAVHeader(&currentAudioFile, AUDIO_SAMPLE_RATE, AUDIO_BITS_PER_SAMPLE, AUDIO_CHANNELS);
    audioFileOpen = true;
    systemState.audioFileStartTime = millis();
    Serial.printf("Opened audio file: %s\n", fileName);
  } else {
    Serial.printf("ERROR: Cannot open audio file: %s\n", fileName);
    audioFileOpen = false;
  }
}

void closeAudioFile() {
  if (audioFileOpen && currentAudioFile) {
    // Cập nhật file size trong WAV header
    uint32_t fileSize = currentAudioFile.size();
    currentAudioFile.seek(4);
    currentAudioFile.write((uint8_t*)&fileSize, 4);
    currentAudioFile.seek(40);
    fileSize -= 36;
    currentAudioFile.write((uint8_t*)&fileSize, 4);
    
    currentAudioFile.close();
    audioFileOpen = false;
    Serial.printf("Closed audio file, index: %d\n", systemState.audioFileIndex);
  }
}

void writeWAVHeader(File* file, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels) {
  uint32_t chunkSize = 0; // Sẽ cập nhật sau
  uint32_t subChunk2Size = 0; // Sẽ cập nhật sau
  uint16_t audioFormat = 1; // PCM
  uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
  uint16_t blockAlign = channels * bitsPerSample / 8;
  
  // RIFF header
  file->write("RIFF", 4);
  file->write((uint8_t*)&chunkSize, 4);
  file->write("WAVE", 4);
  
  // fmt subchunk
  file->write("fmt ", 4);
  uint32_t subChunk1Size = 16;
  file->write((uint8_t*)&subChunk1Size, 4);
  file->write((uint8_t*)&audioFormat, 2);
  file->write((uint8_t*)&channels, 2);
  file->write((uint8_t*)&sampleRate, 4);
  file->write((uint8_t*)&byteRate, 4);
  file->write((uint8_t*)&blockAlign, 2);
  file->write((uint8_t*)&bitsPerSample, 2);
  
  // data subchunk
  file->write("data", 4);
  file->write((uint8_t*)&subChunk2Size, 4);
}

// ============================================
// TASK: LOGGER (Core 0)
// ============================================

void taskLogger(void* parameter) {
  Serial.println("TaskLogger started on Core 0");
  
  while (true) {
    if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      SensorData data = currentSensorData;
      xSemaphoreGive(sensorDataMutex);
      
      // Ghi log vào file
      File logFile = SD.open(systemState.logFileName, FILE_APPEND);
      if (logFile) {
        DateTime now = data.rtcTime;
        logFile.printf("%04d-%02d-%02d %02d:%02d:%02d,",
                      now.year(), now.month(), now.day(),
                      now.hour(), now.minute(), now.second());
        logFile.printf("%.6f,%.6f,%.2f,", data.lat, data.lon, data.speed);
        logFile.printf("%.3f,%.3f,%.3f,", data.ax, data.ay, data.az);
        logFile.printf("%.3f,%.3f,%.3f,", data.gx, data.gy, data.gz);
        logFile.printf("%.1f,%.1f,", data.temp, data.humi);
        
        // Trạng thái
        const char* statusStr = "NORMAL";
        if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
          switch (systemState.status) {
            case STATUS_ACCIDENT_COUNTDOWN: statusStr = "ACCIDENT_COUNTDOWN"; break;
            case STATUS_ACCIDENT_SOS_SENT: statusStr = "ACCIDENT_SOS_SENT"; break;
            case STATUS_RESCUE_COUNTDOWN: statusStr = "RESCUE_COUNTDOWN"; break;
            case STATUS_RESCUE_SOS_SENT: statusStr = "RESCUE_SOS_SENT"; break;
            case STATUS_SOS_CANCELED: statusStr = "SOS_CANCELED"; break;
            default: statusStr = "NORMAL"; break;
          }
          xSemaphoreGive(systemStateMutex);
        }
        logFile.println(statusStr);
        logFile.close();
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(LOG_INTERVAL_MS));
  }
}

void createLogFile() {
  DateTime now = rtc.now();
  snprintf(systemState.logFileName, sizeof(systemState.logFileName),
           "/LOG_%04d%02d%02d_%02d%02d%02d.TXT",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  
  File logFile = SD.open(systemState.logFileName, FILE_WRITE);
  if (logFile) {
    logFile.println("TIME,LAT,LON,SPEED,AX,AY,AZ,GX,GY,GZ,TEMP,HUMI,STATUS");
    logFile.close();
    Serial.printf("Created log file: %s\n", systemState.logFileName);
  }
}

// ============================================
// TASK: SENSORS (Core 1)
// ============================================

void taskSensors(void* parameter) {
  Serial.println("TaskSensors started on Core 1");
  
  unsigned long lastButton1Check = 0;
  unsigned long lastButton2Check = 0;
  bool button1Pressed = false;
  bool button2Pressed = false;
  unsigned long button1PressTime = 0;
  unsigned long button2PressTime = 0;
  
  while (true) {
    // Đọc tất cả sensors
    SensorData data;
    data.valid = false;
    
    readMPU9250(&data);
    readDHT11(&data);
    readGPS(&data);
    readRTC(&data);
    
    // Cập nhật dữ liệu sensor
    if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      currentSensorData = data;
      xSemaphoreGive(sensorDataMutex);
    }
    
    // Phát hiện va chạm từ MPU
    float accelMagnitude = sqrt(data.ax * data.ax + data.ay * data.ay + data.az * data.az);
    
    if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      // Kiểm tra trạng thái hiện tại
      if (systemState.status == STATUS_NORMAL) {
        // Phát hiện va chạm
        if (accelMagnitude > ACCIDENT_THRESHOLD_G) {
          systemState.status = STATUS_ACCIDENT_COUNTDOWN;
          systemState.countdownStart = millis();
          systemState.countdownRemaining = ACCIDENT_COUNTDOWN_SEC;
          systemState.buzzerOn = true;
          Serial.println("!!! ACCIDENT DETECTED - Starting countdown !!!");
        }
      }
      
      // Xử lý đếm ngược tai nạn
      if (systemState.status == STATUS_ACCIDENT_COUNTDOWN) {
        unsigned long elapsed = (millis() - systemState.countdownStart) / 1000;
        systemState.countdownRemaining = ACCIDENT_COUNTDOWN_SEC - elapsed;
        
        if (systemState.countdownRemaining <= 0) {
          // Hết thời gian đếm ngược, gửi SOS
          sendAccidentSOS();
          systemState.status = STATUS_ACCIDENT_SOS_SENT;
          systemState.buzzerOn = false;
          Serial.println("ACCIDENT SOS SENT!");
        }
      }
      
      // Xử lý đếm ngược cứu hộ
      if (systemState.status == STATUS_RESCUE_COUNTDOWN) {
        unsigned long elapsed = (millis() - systemState.countdownStart) / 1000;
        systemState.countdownRemaining = 10 - elapsed; // 10 giây cho cứu hộ
        
        if (systemState.countdownRemaining <= 0) {
          sendRescueSOS();
          systemState.status = STATUS_RESCUE_SOS_SENT;
          systemState.buzzerOn = false;
          Serial.println("RESCUE SOS SENT!");
        }
      }
      
      xSemaphoreGive(systemStateMutex);
    }
    
    // Xử lý nút 1 (SOS tai nạn - hủy)
    if (millis() - lastButton1Check > 50) { // Debounce 50ms
      bool currentState = !digitalRead(BUTTON_1_PIN);
      
      if (currentState && !button1Pressed) {
        button1Pressed = true;
        button1PressTime = millis();
      } else if (currentState && button1Pressed) {
        unsigned long holdTime = millis() - button1PressTime;
        if (holdTime >= BUTTON_HOLD_CANCEL_MS) {
          // Nhấn giữ 5 giây - hủy SOS
          if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (systemState.status == STATUS_ACCIDENT_COUNTDOWN || 
                systemState.status == STATUS_RESCUE_COUNTDOWN) {
              systemState.status = STATUS_SOS_CANCELED;
              systemState.buzzerOn = false;
              Serial.println("SOS CANCELED by button 1");
            }
            xSemaphoreGive(systemStateMutex);
          }
          button1Pressed = false;
        }
      } else if (!currentState && button1Pressed) {
        button1Pressed = false;
      }
      
      lastButton1Check = millis();
    }
    
    // Xử lý nút 2 (SOS cứu hộ - kích hoạt)
    if (millis() - lastButton2Check > 50) { // Debounce 50ms
      bool currentState = !digitalRead(BUTTON_2_PIN);
      
      if (currentState && !button2Pressed) {
        button2Pressed = true;
        button2PressTime = millis();
      } else if (currentState && button2Pressed) {
        unsigned long holdTime = millis() - button2PressTime;
        if (holdTime >= BUTTON_HOLD_CANCEL_MS) {
          // Nhấn giữ 5 giây - hủy SOS
          if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (systemState.status == STATUS_RESCUE_COUNTDOWN) {
              systemState.status = STATUS_SOS_CANCELED;
              systemState.buzzerOn = false;
              Serial.println("SOS CANCELED by button 2");
            }
            xSemaphoreGive(systemStateMutex);
          }
          button2Pressed = false;
        }
      } else if (!currentState && button2Pressed) {
        // Nhấn ngắn - kích hoạt SOS cứu hộ
        if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
          if (systemState.status == STATUS_NORMAL || systemState.status == STATUS_SOS_CANCELED) {
            systemState.status = STATUS_RESCUE_COUNTDOWN;
            systemState.countdownStart = millis();
            systemState.countdownRemaining = 10;
            systemState.buzzerOn = true;
            Serial.println("RESCUE SOS activated by button 2");
          }
          xSemaphoreGive(systemStateMutex);
        }
        button2Pressed = false;
      }
      
      lastButton2Check = millis();
    }
    
    // Điều khiển buzzer
    if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      bool buzzerState = systemState.buzzerOn;
      xSemaphoreGive(systemStateMutex);
      
      if (buzzerState) {
        // Beep theo nhịp (500ms on, 500ms off)
        static unsigned long lastBuzzerToggle = 0;
        static bool buzzerToggle = false;
        if (millis() - lastBuzzerToggle > 500) {
          buzzerToggle = !buzzerToggle;
          digitalWrite(BUZZER_PIN, buzzerToggle ? HIGH : LOW);
          lastBuzzerToggle = millis();
        }
      } else {
        digitalWrite(BUZZER_PIN, LOW);
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void sendAccidentSOS() {
  SensorData data;
  if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    data = currentSensorData;
    xSemaphoreGive(sensorDataMutex);
  }
  
  DateTime now = data.rtcTime;
  char smsText[160];
  snprintf(smsText, sizeof(smsText),
           "SOS TAI NAN\nLat:%.6f\nLon:%.6f\nSpeed:%.1f km/h\nTime:%04d-%02d-%02d %02d:%02d:%02d",
           data.lat, data.lon, data.speed,
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  
  // Gửi SMS và gọi điện
  xQueueSend(smsQueue, smsText, 0);
  xQueueSend(callQueue, SOS_PHONE_NUMBER, 0);
  
  // Gửi SOS qua HTTP lên server
  sendSOSViaHTTP("accident", data);
  
  Serial.println("Accident SOS queued for sending");
}

void sendRescueSOS() {
  SensorData data;
  if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    data = currentSensorData;
    xSemaphoreGive(sensorDataMutex);
  }
  
  DateTime now = data.rtcTime;
  char smsText[160];
  snprintf(smsText, sizeof(smsText),
           "SOS CUU HO - Xe gap su co, can ho tro\nLat:%.6f\nLon:%.6f\nSpeed:%.1f km/h\nTime:%04d-%02d-%02d %02d:%02d:%02d",
           data.lat, data.lon, data.speed,
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  
  // Gửi SMS và gọi điện
  xQueueSend(smsQueue, smsText, 0);
  xQueueSend(callQueue, SOS_PHONE_NUMBER, 0);
  
  // Gửi SOS qua HTTP lên server
  sendSOSViaHTTP("breakdown", data);
  
  Serial.println("Rescue SOS queued for sending");
}

// ============================================
// TASK: SIM 4G (Core 1)
// ============================================

void taskSIM4G(void* parameter) {
  Serial.println("TaskSIM4G started on Core 1");
  
  while (true) {
    char smsText[160];
    char phoneNumber[20];
    char httpData[512];
    
    // Kiểm tra queue SMS
    if (xQueueReceive(smsQueue, smsText, pdMS_TO_TICKS(100)) == pdTRUE) {
      sendSMS(SOS_PHONE_NUMBER, smsText);
    }
    
    // Kiểm tra queue Call
    if (xQueueReceive(callQueue, phoneNumber, pdMS_TO_TICKS(100)) == pdTRUE) {
      makeCall(phoneNumber);
    }
    
    // Kiểm tra queue HTTP (gửi telemetry hoặc SOS)
    HTTPRequest httpReq;
    if (xQueueReceive(httpQueue, &httpReq, pdMS_TO_TICKS(100)) == pdTRUE) {
      sendHTTPPost(httpReq.endpoint, httpReq.jsonData);
    }
    
    // Gửi telemetry định kỳ
    if (millis() - lastTelemetrySend >= TELEMETRY_INTERVAL_MS) {
      sendTelemetryData();
      lastTelemetrySend = millis();
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void initSIM4G() {
  Serial.println("Initializing SIM 4G with TinyGSM...");
  
  // Kiểm tra SIM card
  Serial.println("SIM card check");
  SerialSIM.println("AT+CPIN?");
  delay(1000);
  String rxString = SerialSIM.readString();
  Serial.print("Got: ");
  Serial.println(rxString);
  
  // Lấy tên modem
  String name = modem.getModemName();
  delay(500);
  Serial.println("Modem Name: " + name);
  
  // Chờ mạng
  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    delay(1000);
    return;
  }
  Serial.println(" success");
  
  if (modem.isNetworkConnected()) {
    Serial.println("Network connected");
  }
  
  // Kết nối GPRS
  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    delay(1000);
    return;
  }
  Serial.println(" success");
  
  if (modem.isGprsConnected()) {
    Serial.println("GPRS connected");
    sim4gNetworkOpen = true;
  }
  
  // Cấu hình SMS text mode (cho SMS/call)
  SerialSIM.println("AT+CMGF=1");
  delay(1000);
  
  sim4gInitialized = true;
  Serial.println("SIM 4G initialized successfully");
}

bool sendATCommand(const char* cmd, const char* expectedResponse, unsigned long timeout) {
  SerialSIM.println(cmd);
  unsigned long start = millis();
  String response = "";
  
  while (millis() - start < timeout) {
    while (SerialSIM.available()) {
      char c = SerialSIM.read();
      response += c;
      if (response.length() > 512) {
        response = response.substring(response.length() - 512); // Giới hạn độ dài
      }
    }
    
    if (response.indexOf(expectedResponse) != -1) {
      if (Serial) {
        Serial.print("[SIM] ");
        Serial.print(cmd);
        Serial.print(" -> OK");
        Serial.println();
      }
      return true;
    }
  }
  
  if (Serial) {
    Serial.print("[SIM] ");
    Serial.print(cmd);
    Serial.print(" -> TIMEOUT/ERROR");
    Serial.println();
    Serial.println(response);
  }
  return false;
}

void readSIMResponse() {
  while (SerialSIM.available()) {
    Serial.write(SerialSIM.read());
  }
}

void sendSMS(const char* phoneNumber, const char* message) {
  Serial.printf("Sending SMS to %s: %s\n", phoneNumber, message);
  
  // Gửi AT command để gửi SMS
  SerialSIM.print("AT+CMGS=\"");
  SerialSIM.print(phoneNumber);
  SerialSIM.println("\"");
  delay(1000);
  
  // Gửi nội dung SMS
  SerialSIM.print(message);
  SerialSIM.write(0x1A); // Ctrl+Z để kết thúc
  delay(2000);
  
  readSIMResponse();
  Serial.println("SMS sent");
}

void makeCall(const char* phoneNumber) {
  Serial.printf("Calling %s...\n", phoneNumber);
  
  // Gửi AT command để gọi điện
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "ATD%s;", phoneNumber);
  sendATCommand(cmd, "OK", 5000);
  
  Serial.println("Call initiated");
  
  // TODO: Có thể thêm logic để tự động ngắt cuộc gọi sau một thời gian
}

// ============================================
// GỬI DỮ LIỆU QUA HTTP (SIM 4G)
// ============================================

void sendHTTPPost(const char* endpoint, const char* jsonData) {
  if (!sim4gNetworkOpen || !modem.isGprsConnected()) {
    Serial.println("[HTTP] Network not open, skipping...");
    return;
  }
  
  Serial.printf("[HTTP] Connecting to %s:%d%s\n", server, serverPort, endpoint);
  
  // Tạo HttpClient
  HttpClient httpClient(client, server, serverPort);
  
  // Gửi POST request
  httpClient.post(endpoint, "application/json", jsonData);
  
  // Đọc status code
  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();
  
  Serial.print("[HTTP] Status Code: ");
  Serial.println(statusCode);
  Serial.print("[HTTP] Response: ");
  Serial.println(response);
  
  // Đóng connection
  httpClient.stop();
  
  if (statusCode == 200 || statusCode == 201) {
    Serial.println("[HTTP] Data sent successfully!");
  } else {
    Serial.println("[HTTP] Request failed");
  }
}

void sendTelemetryData() {
  SensorData data;
  if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    data = currentSensorData;
    xSemaphoreGive(sensorDataMutex);
  }
  
  // Tạo JSON data
  StaticJsonDocument<512> doc;
  doc["userId"] = USER_ID;
  doc["deviceId"] = DEVICE_ID;
  doc["lat"] = data.lat;
  doc["lon"] = data.lon;
  doc["speed"] = data.speed;
  doc["source"] = "hardware";
  
  DateTime now = data.rtcTime;
  char timestamp[32];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  doc["timestamp"] = timestamp;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Gửi vào queue
  HTTPRequest httpReq;
  strncpy(httpReq.endpoint, TELEMETRY_ENDPOINT, sizeof(httpReq.endpoint) - 1);
  strncpy(httpReq.jsonData, jsonString.c_str(), sizeof(httpReq.jsonData) - 1);
  httpReq.endpoint[sizeof(httpReq.endpoint) - 1] = '\0';
  httpReq.jsonData[sizeof(httpReq.jsonData) - 1] = '\0';
  xQueueSend(httpQueue, &httpReq, 0);
  
  Serial.println("[TELEMETRY] Data queued for sending");
}

void sendSOSViaHTTP(const char* sosType, const SensorData& data) {
  StaticJsonDocument<512> doc;
  doc["userId"] = USER_ID;
  doc["type"] = sosType;  // "accident" hoặc "breakdown"
  doc["severity"] = "critical";
  
  JsonObject location = doc.createNestedObject("location");
  location["lat"] = data.lat;
  location["lon"] = data.lon;
  
  DateTime now = data.rtcTime;
  char timestamp[32];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  
  char note[256];
  snprintf(note, sizeof(note),
           "SOS %s - Speed: %.1f km/h, Time: %s",
           sosType, data.speed, timestamp);
  doc["note"] = note;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Gửi vào queue
  HTTPRequest httpReq;
  strncpy(httpReq.endpoint, SOS_ENDPOINT, sizeof(httpReq.endpoint) - 1);
  strncpy(httpReq.jsonData, jsonString.c_str(), sizeof(httpReq.jsonData) - 1);
  httpReq.endpoint[sizeof(httpReq.endpoint) - 1] = '\0';
  httpReq.jsonData[sizeof(httpReq.jsonData) - 1] = '\0';
  xQueueSend(httpQueue, &httpReq, 0);
  
  Serial.printf("[SOS HTTP] %s SOS queued for sending\n", sosType);
}

// ============================================
// TASK: OLED (Core 1)
// ============================================

void taskOLED(void* parameter) {
  Serial.println("TaskOLED started on Core 1");
  
  while (true) {
    oled.clearBuffer();
    
    SensorData data;
    SystemState state;
    
    if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      data = currentSensorData;
      xSemaphoreGive(sensorDataMutex);
    }
    
    if (xSemaphoreTake(systemStateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      state = systemState;
      xSemaphoreGive(systemStateMutex);
    }
    
    // Dòng 1: Thời gian
    char timeStr[32];
    DateTime now = data.rtcTime;
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d %02d/%02d",
             now.hour(), now.minute(), now.second(),
             now.day(), now.month());
    oled.drawStr(0, 10, timeStr);
    
    // Dòng 2: Tốc độ
    char speedStr[32];
    snprintf(speedStr, sizeof(speedStr), "Speed: %.1f km/h", data.speed);
    oled.drawStr(0, 22, speedStr);
    
    // Dòng 3: Nhiệt độ / Độ ẩm
    char tempHumiStr[32];
    snprintf(tempHumiStr, sizeof(tempHumiStr), "T:%.1fC H:%.1f%%", data.temp, data.humi);
    oled.drawStr(0, 34, tempHumiStr);
    
    // Dòng 4: Trạng thái
    char statusStr[32];
    switch (state.status) {
      case STATUS_NORMAL:
        strcpy(statusStr, "NORMAL");
        break;
      case STATUS_ACCIDENT_COUNTDOWN:
        snprintf(statusStr, sizeof(statusStr), "ACC COUNT: %ds", state.countdownRemaining);
        break;
      case STATUS_ACCIDENT_SOS_SENT:
        strcpy(statusStr, "ACC SOS SENT");
        break;
      case STATUS_RESCUE_COUNTDOWN:
        snprintf(statusStr, sizeof(statusStr), "RESCUE: %ds", state.countdownRemaining);
        break;
      case STATUS_RESCUE_SOS_SENT:
        strcpy(statusStr, "RESCUE SOS SENT");
        break;
      case STATUS_SOS_CANCELED:
        strcpy(statusStr, "SOS CANCELED");
        break;
      default:
        strcpy(statusStr, "UNKNOWN");
        break;
    }
    oled.drawStr(0, 46, statusStr);
    
    oled.sendBuffer();
    
    vTaskDelay(pdMS_TO_TICKS(OLED_UPDATE_MS));
  }
}

