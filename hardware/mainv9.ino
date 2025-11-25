// ============================================
// CRASH DETECTION SYSTEM - MPU9250
// Phát hiện tai nạn với lọc nhiễu đầy đủ
// Chỉ báo CRASH hoặc NO_CRASH
// ============================================

#include <Wire.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>
#include <DHT.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

// Select your modem
#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

// ============================================
// CẤU HÌNH PHẦN CỨNG
// ============================================
#define MPU9250_ADDR_1 0x68  // Địa chỉ I2C khi AD0 = LOW
#define MPU9250_ADDR_2 0x69  // Địa chỉ I2C khi AD0 = HIGH
uint8_t MPU9250_ADDR = MPU9250_ADDR_2;  // Mặc định, sẽ tự động phát hiện
#define SAMPLE_RATE_HZ 200
#define SAMPLE_INTERVAL_MS (1000 / SAMPLE_RATE_HZ)  // 5ms

// GPS NEO-8M - Dùng HardwareSerial
// TX của NEO-8M nối vào chân 6 (RX của ESP32)
// RX của NEO-8M nối vào chân 7 (TX của ESP32)
#define GPS_RX_PIN 7   // RX của ESP32 (nhận từ TX của GPS)
#define GPS_TX_PIN 6   // TX của ESP32 (gửi đến RX của GPS)
#define GPS_BAUD 9600  // Baud rate của GPS

// SD Card Module - SPI
#define SD_MOSI_PIN 11  // GPIO 11
#define SD_MISO_PIN 13  // GPIO 13
#define SD_SCK_PIN  12  // GPIO 12
#define SD_CS_PIN   10  // GPIO 10

// SIM 4G Module - UART
#define SIM_RX_PIN 18   // RX của ESP32 (nhận từ TX của SIM)
#define SIM_TX_PIN 17   // TX của ESP32 (gửi đến RX của SIM)
#define SIM_BAUD 115200 // Baud rate của SIM

// APN (tùy nhà mạng)
const char apn[] = "m3-world";  // Đổi theo nhà mạng (CMNET, m3-world, v.v.)
const char user[] = "";
const char pass[] = "";

// Backend server config
const char SERVER_HOST[] = "hopdenthongminh.cloud";
const int SERVER_PORT = 80;
const char TELEMETRY_ENDPOINT[] = "/api/telemetry";
const char SOS_STATUS_ENDPOINT_PREFIX[] = "/api/sos/check/";
const unsigned long TELEMETRY_INTERVAL_MS = 30000;      // 30 giây gửi telemetry
const unsigned long SOS_STATUS_CHECK_INTERVAL_MS = 10000; // 10 giây kiểm tra SOS

// Số điện thoại nhận SMS và gọi điện (có thể thay đổi qua Web Interface)
char PHONE_NUMBER[20] = "0798169921";  // Số điện thoại (tối đa 19 ký tự)

// ============================================
// THÔNG TIN THIẾT BỊ VÀ TÀI KHOẢN
// ============================================
char DEVICE_ID[32] = "DEVICE_001";         // ID thiết bị (có thể thay đổi)
char GMAIL_ACCOUNT[64] = "";                // Gmail (ví dụ: example@gmail.com)
char GMAIL_PASSWORD[64] = "";               // Mật khẩu Gmail

// ============================================
// NGƯỠNG PHÁT HIỆN TAI NẠN (Mặc định)
// ============================================
const float G_CRASH = 3.5;        // G-force va chạm mạnh
const float ANGLE_CRASH = 35.0;    // Độ đổi góc lớn (degrees)

// ============================================
// WEB INTERFACE SERVER
// ============================================
#define WEB_SETTING_BUTTON_HOLD_MS 3000     // Giữ nút 36 trong 3 giây để thoát Web Setting
#define WEB_SETTING_TIMEOUT_MS 180000       // 3 phút timeout - tự động thoát Web Setting
#define EEPROM_SIZE 512                     // Kích thước EEPROM
#define EEPROM_START_ADDR 0                // Địa chỉ bắt đầu EEPROM

// WiFi Access Point
const char* AP_SSID = "CRASH_DETECTION_SETUP";
const char* AP_PASSWORD = "12345678";       // Mật khẩu WiFi (có thể thay đổi)

WebServer server(80);

// Biến ngưỡng cảnh báo (có thể thay đổi qua web)
float gCrashThreshold = G_CRASH;
float angleCrashThreshold = ANGLE_CRASH;
bool allowSOSSent = true;                  // Cho phép gửi SOS hay không
int countdownTime = 30;                     // Thời gian đếm ngược (giây)

// Biến quản lý Web Setting
bool webSettingActive = false;
unsigned long webSettingStartTime = 0;
unsigned long webSettingExitTime = 0;  // Thời gian thoát Web Setting
bool justExitedWebSetting = false;     // Flag để bỏ qua xử lý nút sau khi thoát

// Cooldown sau khi hủy SOS để tránh trigger lại ngay
unsigned long sosCancelTime = 0;       // Thời gian hủy SOS
#define SOS_CANCEL_COOLDOWN_MS 3000    // 3 giây cooldown sau khi hủy SOS

// DHT11 Temperature & Humidity Sensor
#define DHT_PIN 35      // GPIO 35
#define DHT_TYPE DHT11  // Loại cảm biến

// DS1307 RTC (Real Time Clock)
#define DS1307_ADDR_1 0x50  // Địa chỉ I2C có thể
#define DS1307_ADDR_2 0x68  // Địa chỉ I2C mặc định của DS1307
uint8_t DS1307_ADDR = DS1307_ADDR_2;  // Sẽ tự động phát hiện
#define DS1307_TIME_REG 0x00  // Register bắt đầu đọc thời gian

// ============================================
// MPU9250 REGISTERS
// ============================================
#define MPU9250_WHO_AM_I    0x75
#define MPU9250_PWR_MGMT_1  0x6B
#define MPU9250_ACCEL_XOUT_H 0x3B
#define MPU9250_GYRO_XOUT_H  0x43
#define MPU9250_ACCEL_CONFIG 0x1C
#define MPU9250_GYRO_CONFIG  0x1B
#define MPU9250_CONFIG       0x1A

// ============================================
// NGƯỠNG PHÁT HIỆN (điều chỉnh được)
// ============================================
// G_CRASH và ANGLE_CRASH đã được định nghĩa ở trên
const int WINDOW_MS = 500;         // Cửa sổ tính deltaAngle (ms)

// ============================================
// BỘ LỌC EMA
// ============================================
const float EMA_ALPHA = 0.2;       // Hệ số EMA cho accel

// ============================================
// COMPLEMENTARY FILTER
// ============================================
const float COMPL_FILTER_ALPHA = 0.98;  // Trọng số cho gyro

// ============================================
// ENUM & STRUCT
// ============================================
enum CrashState {
  NO_CRASH,
  CRASH
};

enum SystemState {
  STATE_NORMAL,        // Bình thường
  STATE_CRASH_DETECTED, // Vừa phát hiện CRASH
  STATE_COUNTDOWN,     // Đang đếm ngược 30 giây
  STATE_SOS_SENT,      // Đã gửi SOS
  STATE_WEB_SETTING    // Chế độ Web Setting
};

struct ImuData {
  float ax, ay, az;        // Gia tốc (g)
  float gx, gy, gz;        // Gyro (deg/s)
  float g_total;           // G-force tổng
  float roll, pitch;       // Góc nghiêng (degrees)
  float deltaAngle;        // Độ đổi góc trong cửa sổ
};

// ============================================
// BIẾN TOÀN CỤC
// ============================================
ImuData imuData;
ImuData imuDataFiltered;   // Sau EMA filter

// EMA filtered values
float ax_filtered = 0, ay_filtered = 0, az_filtered = 0;

// Complementary filter values
float roll_accel = 0, pitch_accel = 0;
float roll_gyro = 0, pitch_gyro = 0;
float roll = 0, pitch = 0;

// Lịch sử góc để tính deltaAngle
struct AngleHistory {
  float roll;
  float pitch;
  unsigned long timestamp;
};

#define MAX_HISTORY 100
AngleHistory angleHistory[MAX_HISTORY];
int historyIndex = 0;
int historyCount = 0;

// Timing
unsigned long lastSampleTime = 0;
unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL_MS = 100;  // In mỗi 100ms

// SD Card Logging
bool sdCardInitialized = false;
unsigned long lastLogTime = 0;
const unsigned long LOG_INTERVAL_MS = 60000;  // Ghi log mỗi 1 phút (60000ms)
const char* LOG_FILE = "/log.txt";
const char* CRASH_LOG_FILE = "/crash_log.txt";

// ============================================
// SIM 4G Module
// ============================================
HardwareSerial SerialSIM(2);  // Serial2 cho SIM 4G
TinyGsm modem(SerialSIM);
TinyGsmClient gsmClient(modem);

bool sim4gInitialized = false;
bool sim4gNetworkOpen = false;
bool telemetryPendingImmediateSend = false;
unsigned long lastTelemetrySendTime = 0;
unsigned long lastSOSStatusCheckTime = 0;
String currentEventType = "NORMAL";
String currentSeverity = "LOW";
bool hasActiveSOSOnServer = false;
String currentSOSId = "";
String currentSOSStatus = "";
unsigned long ignoreServerCancelUntil = 0;
bool awaitingServerSOSConfirmation = false;

// ============================================
// BUTTON HANDLING - Nút 36 (Hủy SOS/Chuyển màn), Nút 37 (SOS thủ công)
// ============================================
#define BUTTON_CANCEL_PIN 36
#define BUTTON_MANUAL_SOS_PIN 37
#define PRESS_SHORT_MS 3000      // < 3s = ấn nhanh (chuyển màn hình)
#define PRESS_MEDIUM_MS 3000     // >= 3s = ấn chậm (hủy SOS hoặc thoát Web Setting)

// Các biến static cho nút 36 đã được tích hợp vào hàm readButton36()

// ============================================
// OLED SSD1306 0.96 inch Display
// ============================================
// SDA và SCL dùng chung với I2C (thường là GPIO 21, 22 cho ESP32)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Quản lý màn hình
enum DisplayScreen {
  SCREEN_INFO = 0,      // Màn 1: Thông tin
  SCREEN_SENSORS = 1,   // Màn 2: Cảm biến MPU
  SCREEN_STATUS = 2,    // Màn 3: Trạng thái kết nối
  SCREEN_GPS = 3        // Màn 4: GPS chi tiết
};

DisplayScreen currentScreen = SCREEN_INFO;
unsigned long lastScreenUpdate = 0;
const unsigned long SCREEN_UPDATE_INTERVAL = 500;  // Cập nhật màn hình mỗi 500ms

static bool btnManualSOSLast = HIGH;
static uint32_t btnManualSOSPressTime = 0;
static bool btnManualSOSPressed = false;

// ============================================
// BUZZER HANDLING
// ============================================
#define BUZZER_PIN 45  // Pin buzzer
#define BUZZER_BEEP_DURATION 200  // Thời gian kêu mỗi lần bíp (ms)
#define BUZZER_BEEP_INTERVAL 1000  // Chu kỳ bíp bíp (1 giây)

static bool buzzerState = false;
static unsigned long lastBeepTime = 0;
static unsigned long beepStartTime = 0;

// ============================================
// SOS COUNTDOWN SYSTEM
// ============================================
const unsigned long SOS_COUNTDOWN_MS = 30000;  // 30 giây
SystemState systemState = STATE_NORMAL;
unsigned long crashDetectedTime = 0;
unsigned long countdownStartTime = 0;
int remainingSeconds = 30;
bool isManualSOS = false;  // Phân biệt SOS tự động hay thủ công

// ============================================
// GPS NEO-8M - HardwareSerial với TinyGPS++
// ============================================
// Dùng Serial1 cho ESP32 (hoặc Serial1/Serial2 tùy board)
HardwareSerial gpsSerial(1);  // Serial1 cho ESP32

// TinyGPS++ object
TinyGPSPlus gps;

struct GPSData {
  float latitude;      // Vĩ độ
  float longitude;     // Kinh độ
  float altitude;      // Độ cao (m)
  float speed;         // Tốc độ (km/h)
  int satellites;     // Số vệ tinh
  float hdop;          // Độ chính xác (HDOP)
  bool valid;          // Dữ liệu GPS hợp lệ
  String time;         // Thời gian (HH:MM:SS)
  String date;         // Ngày (DD/MM/YY)
  float course;        // Hướng di chuyển (degrees)
};

GPSData gpsData;
unsigned long lastGPSUpdate = 0;
const unsigned long GPS_UPDATE_INTERVAL = 1000;  // Cập nhật GPS mỗi 1 giây

// Biến để tính vận tốc từ 2 điểm GPS
float lastGPSLat = 0;
float lastGPSLon = 0;
unsigned long lastGPSTime = 0;
bool hasLastGPSPoint = false;

// ============================================
// DS1307 RTC (Real Time Clock)
// ============================================
struct RTCData {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t dayOfWeek;  // 1=Sunday, 2=Monday, ...
  uint8_t day;
  uint8_t month;
  uint8_t year;       // 00-99 (2000-2099)
  bool valid;         // Dữ liệu RTC hợp lệ
};

RTCData rtcData;
bool ds1307Initialized = false;

// ============================================
// DHT11 Temperature & Humidity Sensor
// ============================================
DHT dht(DHT_PIN, DHT_TYPE);

struct DHTData {
  float temperature;  // Nhiệt độ (°C)
  float humidity;     // Độ ẩm (%)
  bool valid;         // Dữ liệu hợp lệ
};

DHTData dhtData;
unsigned long lastDHTRead = 0;
const unsigned long DHT_READ_INTERVAL = 2000;  // Đọc DHT mỗi 2 giây (DHT11 cần thời gian)

// ============================================
// HÀM ĐỌC MPU9250
// ============================================
void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t readRegister(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  uint8_t error = Wire.endTransmission(false);
  if (error != 0) {
    return 0xFF;  // Lỗi
  }
  uint8_t bytesRead = Wire.requestFrom(addr, (uint8_t)1);
  if (bytesRead != 1) {
    return 0xFF;  // Không đọc được
  }
  return Wire.read();
}

uint8_t readRegister(uint8_t reg) {
  return readRegister(MPU9250_ADDR, reg);
}

bool readAccelGyro(float &ax, float &ay, float &az, float &gx, float &gy, float &gz) {
  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(MPU9250_ACCEL_XOUT_H);
  uint8_t error = Wire.endTransmission(false);
  if (error != 0) {
    return false;  // Lỗi truyền
  }
  
  uint8_t bytesRead = Wire.requestFrom(MPU9250_ADDR, (uint8_t)14);
  if (bytesRead != 14) {
    return false;  // Không đọc đủ dữ liệu
  }
  
  // Đọc dữ liệu với kiểm tra
  int16_t accelX = (Wire.read() << 8) | Wire.read();
  int16_t accelY = (Wire.read() << 8) | Wire.read();
  int16_t accelZ = (Wire.read() << 8) | Wire.read();
  Wire.read(); Wire.read();  // Skip temperature
  int16_t gyroX = (Wire.read() << 8) | Wire.read();
  int16_t gyroY = (Wire.read() << 8) | Wire.read();
  int16_t gyroZ = (Wire.read() << 8) | Wire.read();
  
  // MPU9250: Accel ±2g = 16384 LSB/g, Gyro ±250deg/s = 131 LSB/deg/s
  ax = accelX / 16384.0;
  ay = accelY / 16384.0;
  az = accelZ / 16384.0;
  gx = gyroX / 131.0;
  gy = gyroY / 131.0;
  gz = gyroZ / 131.0;
  
  return true;
}

bool initMPU9250() {
  Wire.begin();
  delay(100);
  
  // Tự động phát hiện địa chỉ I2C (thử cả 0x68 và 0x69)
  Serial.println("Đang tìm MPU9250...");
  uint8_t whoami1 = readRegister(MPU9250_ADDR_1, MPU9250_WHO_AM_I);
  uint8_t whoami2 = readRegister(MPU9250_ADDR_2, MPU9250_WHO_AM_I);
  
  Serial.print("WHO_AM_I tại 0x68: 0x");
  Serial.println(whoami1, HEX);
  Serial.print("WHO_AM_I tại 0x69: 0x");
  Serial.println(whoami2, HEX);
  
  // MPU9250 có thể trả về 0x71 hoặc 0x70 (tùy version)
  if (whoami1 == 0x71 || whoami1 == 0x70) {
    MPU9250_ADDR = MPU9250_ADDR_1;
    Serial.println("✓ Tìm thấy MPU9250 tại địa chỉ 0x68");
  } else if (whoami2 == 0x71 || whoami2 == 0x70) {
    MPU9250_ADDR = MPU9250_ADDR_2;
    Serial.println("✓ Tìm thấy MPU9250 tại địa chỉ 0x69");
  } else {
    Serial.println("✗ Không tìm thấy MPU9250!");
    Serial.println("Kiểm tra kết nối I2C (SDA, SCL)");
    return false;
  }
  
  // Reset
  writeRegister(MPU9250_PWR_MGMT_1, 0x80);
  delay(100);
  
  // Wake up, không dùng sleep mode, chọn clock source (PLL với X axis gyro)
  writeRegister(MPU9250_PWR_MGMT_1, 0x01);
  delay(10);
  
  // Cấu hình Accel: ±2g (AFS_SEL = 0)
  writeRegister(MPU9250_ACCEL_CONFIG, 0x00);
  delay(10);
  
  // Cấu hình Gyro: ±250deg/s (FS_SEL = 0)
  writeRegister(MPU9250_GYRO_CONFIG, 0x00);
  delay(10);
  
  // Low pass filter: 44Hz (DLPF_CFG = 3)
  writeRegister(MPU9250_CONFIG, 0x03);
  delay(10);
  
  // Kiểm tra lại sau khi cấu hình
  uint8_t whoami_final = readRegister(MPU9250_WHO_AM_I);
  Serial.print("WHO_AM_I sau cấu hình: 0x");
  Serial.println(whoami_final, HEX);
  
  Serial.println("✓ MPU9250 đã được khởi tạo thành công!");
  return true;
}

// ============================================
// EMA FILTER (lọc nhiễu gia tốc)
// ============================================
void applyEMAFilter(float ax, float ay, float az, float &ax_f, float &ay_f, float &az_f) {
  if (ax_filtered == 0 && ay_filtered == 0 && az_filtered == 0) {
    // Khởi tạo lần đầu
    ax_filtered = ax;
    ay_filtered = ay;
    az_filtered = az;
  } else {
    // EMA filter
    ax_filtered = EMA_ALPHA * ax + (1 - EMA_ALPHA) * ax_filtered;
    ay_filtered = EMA_ALPHA * ay + (1 - EMA_ALPHA) * ay_filtered;
    az_filtered = EMA_ALPHA * az + (1 - EMA_ALPHA) * az_filtered;
  }
  ax_f = ax_filtered;
  ay_f = ay_filtered;
  az_f = az_filtered;
}

// ============================================
// TÍNH ROLL/PITCH TỪ ACCEL
// ============================================
void calculateRollPitchFromAccel(float ax, float ay, float az, float &roll_a, float &pitch_a) {
  roll_a = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
  pitch_a = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
}

// ============================================
// COMPLEMENTARY FILTER
// ============================================
void updateComplementaryFilter(float ax, float ay, float az, float gx, float gy, float gz, float dt) {
  // Tính roll/pitch từ accel
  calculateRollPitchFromAccel(ax, ay, az, roll_accel, pitch_accel);
  
  // Khởi tạo lần đầu nếu chưa có
  static bool firstRun = true;
  if (firstRun) {
    roll = roll_accel;
    pitch = pitch_accel;
    roll_gyro = 0;
    pitch_gyro = 0;
    firstRun = false;
  }
  
  // Tích phân gyro
  roll_gyro += gx * dt;
  pitch_gyro += gy * dt;
  
  // Complementary filter: kết hợp accel và gyro
  // Gyro có trọng số cao (0.98), accel có trọng số thấp (0.02) để lọc nhiễu
  roll = COMPL_FILTER_ALPHA * (roll + gx * dt) + (1 - COMPL_FILTER_ALPHA) * roll_accel;
  pitch = COMPL_FILTER_ALPHA * (pitch + gy * dt) + (1 - COMPL_FILTER_ALPHA) * pitch_accel;
}

// ============================================
// TÍNH DELTA ANGLE TRONG CỬA SỔ THỜI GIAN
// ============================================
float calculateDeltaAngle() {
  if (historyCount < 2) return 0.0;
  
  unsigned long now = millis();
  float maxDelta = 0.0;
  
  // Tìm góc trong cửa sổ WINDOW_MS trước
  for (int i = 0; i < historyCount; i++) {
    unsigned long age = now - angleHistory[i].timestamp;
    if (age <= WINDOW_MS) {
      // Tính độ lệch góc
      float deltaRoll = abs(roll - angleHistory[i].roll);
      float deltaPitch = abs(pitch - angleHistory[i].pitch);
      float delta = max(deltaRoll, deltaPitch);
      
      if (delta > maxDelta) {
        maxDelta = delta;
      }
    }
  }
  
  return maxDelta;
}

// ============================================
// LƯU LỊCH SỬ GÓC
// ============================================
void saveAngleHistory() {
  angleHistory[historyIndex].roll = roll;
  angleHistory[historyIndex].pitch = pitch;
  angleHistory[historyIndex].timestamp = millis();
  
  historyIndex = (historyIndex + 1) % MAX_HISTORY;
  if (historyCount < MAX_HISTORY) {
    historyCount++;
  }
}

// ============================================
// HÀM ĐIỀU KHIỂN BUZZER
// ============================================
void updateBuzzer() {
  unsigned long now = millis();
  
  if (systemState == STATE_COUNTDOWN) {
    // Đang đếm ngược - kêu bíp bíp mỗi giây
    if (!buzzerState) {
      // Chưa kêu, kiểm tra xem đã đến lúc kêu chưa
      unsigned long timeSinceLastBeep = now - lastBeepTime;
      if (timeSinceLastBeep >= BUZZER_BEEP_INTERVAL) {
        // Bắt đầu kêu
        buzzerState = true;
        beepStartTime = now;
        digitalWrite(BUZZER_PIN, HIGH);
      }
    } else {
      // Đang kêu, kiểm tra xem đã hết thời gian kêu chưa
      unsigned long beepDuration = now - beepStartTime;
      if (beepDuration >= BUZZER_BEEP_DURATION) {
        // Dừng kêu
        buzzerState = false;
        lastBeepTime = now;
        digitalWrite(BUZZER_PIN, LOW);
      }
    }
  } else {
    // Không trong trạng thái đếm ngược - tắt buzzer
    if (buzzerState) {
      buzzerState = false;
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
}

// ============================================
// HÀM ĐỌC DS1307 RTC
// ============================================
uint8_t bcdToDec(uint8_t bcd) {
  return ((bcd / 16) * 10) + (bcd % 16);
}

uint8_t decToBcd(uint8_t dec) {
  return ((dec / 10) * 16) + (dec % 10);
}

bool readDS1307Register(uint8_t addr, uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  uint8_t error = Wire.endTransmission(false);
  if (error != 0) {
    return false;
  }
  uint8_t bytesRead = Wire.requestFrom(addr, (uint8_t)1);
  if (bytesRead != 1) {
    return false;
  }
  value = Wire.read();
  return true;
}

bool initDS1307() {
  // Thử cả hai địa chỉ
  uint8_t testAddr1 = DS1307_ADDR_1;
  uint8_t testAddr2 = DS1307_ADDR_2;
  
  // Kiểm tra địa chỉ 0x68 trước (địa chỉ mặc định)
  uint8_t testValue;
  if (readDS1307Register(testAddr2, 0x00, testValue)) {
    DS1307_ADDR = testAddr2;
    Serial.print("✓ DS1307 tìm thấy tại địa chỉ 0x");
    Serial.println(testAddr2, HEX);
    return true;
  }
  
  // Thử địa chỉ 0x50
  if (readDS1307Register(testAddr1, 0x00, testValue)) {
    DS1307_ADDR = testAddr1;
    Serial.print("✓ DS1307 tìm thấy tại địa chỉ 0x");
    Serial.println(testAddr1, HEX);
    return true;
  }
  
  Serial.println("✗ Không tìm thấy DS1307!");
  return false;
}

bool readDS1307Time() {
  Wire.beginTransmission(DS1307_ADDR);
  Wire.write(DS1307_TIME_REG);
  uint8_t error = Wire.endTransmission(false);
  if (error != 0) {
    rtcData.valid = false;
    return false;
  }
  
  uint8_t bytesRead = Wire.requestFrom(DS1307_ADDR, (uint8_t)7);
  if (bytesRead != 7) {
    rtcData.valid = false;
    return false;
  }
  
  // Đọc 7 bytes: second, minute, hour, dayOfWeek, day, month, year
  uint8_t second = Wire.read();
  uint8_t minute = Wire.read();
  uint8_t hour = Wire.read();
  uint8_t dayOfWeek = Wire.read();
  uint8_t day = Wire.read();
  uint8_t month = Wire.read();
  uint8_t year = Wire.read();
  
  // Chuyển từ BCD sang decimal
  rtcData.second = bcdToDec(second & 0x7F);  // Bit 7 là CH (Clock Halt)
  rtcData.minute = bcdToDec(minute & 0x7F);
  rtcData.hour = bcdToDec(hour & 0x3F);       // Bit 6 là 12/24 hour mode
  rtcData.dayOfWeek = bcdToDec(dayOfWeek & 0x07);
  rtcData.day = bcdToDec(day & 0x3F);
  rtcData.month = bcdToDec(month & 0x1F);
  rtcData.year = bcdToDec(year);
  
  rtcData.valid = true;
  return true;
}

String getRTCTimeString() {
  if (!rtcData.valid) {
    return "00:00:00";
  }
  char timeStr[10];
  sprintf(timeStr, "%02d:%02d:%02d", rtcData.hour, rtcData.minute, rtcData.second);
  return String(timeStr);
}

String getRTCDateString() {
  if (!rtcData.valid) {
    return "01/01/00";
  }
  char dateStr[9];
  sprintf(dateStr, "%02d/%02d/%02d", rtcData.day, rtcData.month, rtcData.year);
  return String(dateStr);
}

String getRTCDateTimeString() {
  if (!rtcData.valid) {
    return "00/00/00 00:00:00";
  }
  char datetimeStr[20];
  sprintf(datetimeStr, "%02d/%02d/%02d %02d:%02d:%02d", 
          rtcData.day, rtcData.month, rtcData.year,
          rtcData.hour, rtcData.minute, rtcData.second);
  return String(datetimeStr);
}

// ============================================
// HÀM ĐỌC BUTTON (Ấn chậm = >= 1000ms, ấn nhanh = < 1000ms)
// ============================================
// ============================================
// HÀM PHÁT HIỆN 3 TRẠNG THÁI NÚT 36
// ============================================
// Trả về:
//   0 = Chưa có tác động
//   1 = Ấn nhanh (< 3s) - Chuyển màn hình
//   2 = Ấn chậm (3-10s) - Hủy SOS
//   3 = Ấn lâu (>= 10s) - Web Setting
int readButton36() {
  static bool btnLast = HIGH;
  static uint32_t btnPressTime = 0;
  static bool btnPressed = false;
  static bool longPressTriggered = false;  // Đã kích hoạt long press
  
  bool btn = digitalRead(BUTTON_CANCEL_PIN);
  uint32_t now = millis();
  
  if (btn == LOW && btnLast == HIGH) {
    // Nút vừa được nhấn xuống
    btnPressTime = now;
    btnPressed = true;
    longPressTriggered = false;  // Reset flag
  }
  else if (btn == HIGH && btnLast == LOW && btnPressed) {
    // Nút vừa được thả ra
    btnPressed = false;
    uint32_t duration = now - btnPressTime;
    btnLast = btn;
    
    // Phân loại theo thời gian giữ
    if (duration >= PRESS_MEDIUM_MS) {
      longPressTriggered = false;  // Reset flag
      return 2;  // Ấn chậm (>= 3s) - Hủy SOS hoặc thoát Web Setting
    } else if (duration > 50) {  // Loại bỏ nhiễu (tối thiểu 50ms)
      return 1;  // Ấn nhanh (< 3s) - Chuyển màn hình
    }
  }
  else if (btn == LOW && btnPressed) {
    // Đang giữ nút
    uint32_t duration = now - btnPressTime;
    
    // Kiểm tra nếu đã giữ đủ 3 giây (chỉ trigger một lần)
    if (duration >= PRESS_MEDIUM_MS && !longPressTriggered) {
      longPressTriggered = true;  // Đánh dấu đã trigger
      return 2;  // Ấn chậm (>= 3s) - Hủy SOS hoặc thoát Web Setting
    }
  }
  
  btnLast = btn;
  return 0;  // Chưa có tác động
}

// ============================================
// HÀM HIỂN THỊ OLED
// ============================================
void nextScreen() {
  currentScreen = (DisplayScreen)((currentScreen + 1) % 4);
}

void displayScreen1_Info() {
  u8g2.clearBuffer();
  
  // Tiêu đề
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "=== THONG TIN ===");
  
  // Định vị GPS
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 22, "GPS:");
  if (gpsData.valid) {
    char gpsStr[20];
    snprintf(gpsStr, sizeof(gpsStr), "%.4f,%.4f", gpsData.latitude, gpsData.longitude);
    u8g2.drawStr(30, 22, gpsStr);
  } else {
    u8g2.drawStr(30, 22, "noGPS");
  }
  
  // Trạng thái cảnh báo
  u8g2.drawStr(0, 34, "Trang thai:");
  if (systemState == STATE_COUNTDOWN) {
    char countdownStr[15];
    snprintf(countdownStr, sizeof(countdownStr), "SOS %ds", remainingSeconds);
    u8g2.drawStr(70, 34, countdownStr);
  } else if (systemState == STATE_SOS_SENT) {
    u8g2.drawStr(70, 34, "SOS SENT");
  } else {
    u8g2.drawStr(70, 34, "NORMAL");
  }
  
  // Thời gian
  u8g2.drawStr(0, 46, "Time:");
  if (rtcData.valid) {
    char timeStr[12];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", rtcData.hour, rtcData.minute, rtcData.second);
    u8g2.drawStr(35, 46, timeStr);
  } else {
    u8g2.drawStr(35, 46, "N/A");
  }
  
  // Nhiệt độ và độ ẩm
  u8g2.drawStr(0, 58, "T/H:");
  if (dhtData.valid) {
    char tempHumStr[20];
    snprintf(tempHumStr, sizeof(tempHumStr), "%.1fC %.1f%%", dhtData.temperature, dhtData.humidity);
    u8g2.drawStr(30, 58, tempHumStr);
  } else {
    u8g2.drawStr(30, 58, "N/A");
  }
  
  u8g2.sendBuffer();
}

void displayScreen2_Sensors() {
  u8g2.clearBuffer();
  
  // Tiêu đề
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "=== CAM BIEN ===");
  
  // Ax, Ay, Az
  u8g2.setFont(u8g2_font_6x10_tr);
  char accelStr[30];
  snprintf(accelStr, sizeof(accelStr), "A:%.2f %.2f %.2f", imuData.ax, imuData.ay, imuData.az);
  u8g2.drawStr(0, 24, accelStr);
  
  // Roll, Pitch
  char angleStr[25];
  snprintf(angleStr, sizeof(angleStr), "R:%.1f P:%.1f", imuData.roll, imuData.pitch);
  u8g2.drawStr(0, 38, angleStr);
  
  // G-force
  char gforceStr[20];
  snprintf(gforceStr, sizeof(gforceStr), "G:%.2f g", imuData.g_total);
  u8g2.drawStr(0, 52, gforceStr);
  
  // Delta Angle
  char deltaStr[20];
  snprintf(deltaStr, sizeof(deltaStr), "D:%.1f deg", imuData.deltaAngle);
  u8g2.drawStr(0, 64, deltaStr);
  
  u8g2.sendBuffer();
}

void displayScreen3_Status() {
  u8g2.clearBuffer();
  
  // Tiêu đề
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "=== TRANG THAI ===");
  
  u8g2.setFont(u8g2_font_6x10_tr);
  
  // Thẻ SD
  u8g2.drawStr(0, 24, "SD Card:");
  if (sdCardInitialized) {
    u8g2.drawStr(60, 24, "OK");
  } else {
    u8g2.drawStr(60, 24, "NO");
  }
  
  // SIM 4G
  u8g2.drawStr(0, 38, "SIM 4G:");
  if (sim4gInitialized) {
    if (modem.isNetworkConnected()) {
      u8g2.drawStr(60, 38, "OK");
    } else {
      u8g2.drawStr(60, 38, "NO NET");
    }
  } else {
    u8g2.drawStr(60, 38, "NO");
  }
  
  // Ngưỡng cảnh báo (dùng biến động)
  u8g2.drawStr(0, 52, "G_CRASH:");
  char thresholdStr[15];
  snprintf(thresholdStr, sizeof(thresholdStr), "%.1fg", gCrashThreshold);
  u8g2.drawStr(60, 52, thresholdStr);
  
  u8g2.drawStr(0, 64, "ANGLE:");
  char angleThresholdStr[15];
  snprintf(angleThresholdStr, sizeof(angleThresholdStr), "%.0f deg", angleCrashThreshold);
  u8g2.drawStr(60, 64, angleThresholdStr);
  
  u8g2.sendBuffer();
}

void displayScreen4_GPS() {
  u8g2.clearBuffer();
  
  // Tiêu đề
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "=== GPS CHI TIET ===");
  
  u8g2.setFont(u8g2_font_6x10_tr);
  
  if (gpsData.valid) {
    // Vận tốc
    char speedStr[20];
    snprintf(speedStr, sizeof(speedStr), "Speed: %.1f km/h", gpsData.speed);
    u8g2.drawStr(0, 24, speedStr);
    
    // Số vệ tinh
    char satStr[20];
    snprintf(satStr, sizeof(satStr), "Sat: %d", gpsData.satellites);
    u8g2.drawStr(0, 38, satStr);
    
    // HDOP (Độ chính xác)
    char hdopStr[25];
    if (gpsData.hdop < 99.0) {
      snprintf(hdopStr, sizeof(hdopStr), "HDOP: %.2f", gpsData.hdop);
    } else {
      snprintf(hdopStr, sizeof(hdopStr), "HDOP: N/A");
    }
    u8g2.drawStr(0, 52, hdopStr);
    
    // Hướng di chuyển
    if (gpsData.course > 0) {
      char courseStr[20];
      snprintf(courseStr, sizeof(courseStr), "Course: %.0f deg", gpsData.course);
      u8g2.drawStr(0, 64, courseStr);
    } else {
      u8g2.drawStr(0, 64, "Course: N/A");
    }
  } else {
    u8g2.drawStr(0, 30, "NO GPS SIGNAL");
    u8g2.drawStr(0, 44, "Waiting for");
    u8g2.drawStr(0, 58, "satellites...");
  }
  
  u8g2.sendBuffer();
}

// Hàm displayScreen5_WebSetting() đã bị xóa - không còn màn hình 5

void updateDisplay() {
  unsigned long now = millis();
  if (now - lastScreenUpdate >= SCREEN_UPDATE_INTERVAL) {
    lastScreenUpdate = now;
    
    // Nếu đang ở chế độ Web Setting, hiển thị màn hình Web Setting
    if (systemState == STATE_WEB_SETTING) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(0, 10, "=== WEB SETTING ===");
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(0, 24, "SSID:");
      u8g2.drawStr(40, 24, AP_SSID);
      u8g2.drawStr(0, 38, "IP: 192.168.4.1");
      
      // Hiển thị thời gian còn lại
      unsigned long elapsed = millis() - webSettingStartTime;
      unsigned long remaining = (WEB_SETTING_TIMEOUT_MS - elapsed) / 1000;
      if (remaining > 0 && remaining <= 180) {
        char timeStr[20];
        snprintf(timeStr, sizeof(timeStr), "Time: %lu s", remaining);
        u8g2.drawStr(0, 52, timeStr);
      } else {
        u8g2.drawStr(0, 52, "Time: 0 s");
      }
      
      u8g2.drawStr(0, 64, "Gi nut 3s de thoat");
      u8g2.sendBuffer();
      return;
    }
    
    switch (currentScreen) {
      case SCREEN_INFO:
        displayScreen1_Info();
        break;
      case SCREEN_SENSORS:
        displayScreen2_Sensors();
        break;
      case SCREEN_STATUS:
        displayScreen3_Status();
        break;
      case SCREEN_GPS:
        displayScreen4_GPS();
        break;
    }
  }
}

int readButtonManualSOS() {
  bool btn = digitalRead(BUTTON_MANUAL_SOS_PIN);
  uint32_t now = millis();
  
  if (btn == LOW && btnManualSOSLast == HIGH) {
    // Nút vừa được nhấn xuống
    btnManualSOSPressTime = now;
    btnManualSOSPressed = true;
  }
  else if (btn == HIGH && btnManualSOSLast == LOW && btnManualSOSPressed) {
    // Nút vừa được thả ra
    btnManualSOSPressed = false;
    uint32_t duration = now - btnManualSOSPressTime;
    btnManualSOSLast = btn;
    
    // Trả về 1 nếu ấn chậm (>= 1 giây), 0 nếu ấn nhanh
    if (duration >= 1000) {  // Nút 37: ấn >= 1 giây để gửi SOS thủ công
      return 1;  // Ấn chậm - Gửi SOS thủ công
    }
  }
  btnManualSOSLast = btn;
  return 0;  // Chưa có tác động hoặc ấn nhanh
}

// ============================================
// HÀM ĐỌC GPS NEO-8M VỚI TinyGPS++
// ============================================
void updateGPS() {
  // Đọc dữ liệu từ GPS Serial và feed vào TinyGPS++
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      // Có dữ liệu GPS mới được decode
      updateGPSData();
    }
  }
}

void updateGPSData() {
  // Cập nhật struct GPSData từ TinyGPS++
  if (gps.location.isValid()) {
    gpsData.latitude = gps.location.lat();
    gpsData.longitude = gps.location.lng();
    gpsData.valid = true;
  } else {
    gpsData.valid = false;
  }
  
  if (gps.altitude.isValid()) {
    gpsData.altitude = gps.altitude.meters();
  }
  
  // Tính vận tốc từ GPS
  if (gps.speed.isValid() && gps.speed.kmph() > 0) {
    // Ưu tiên dùng speed từ GPS nếu có
    gpsData.speed = gps.speed.kmph();
  } else if (gps.location.isValid()) {
    // Nếu GPS không có speed, tính từ 2 điểm GPS
    unsigned long currentTime = millis();
    if (hasLastGPSPoint && (currentTime - lastGPSTime) > 1000) {
      // Tính vận tốc từ 2 điểm GPS (chỉ khi cách nhau > 1 giây)
      gpsData.speed = calculateSpeedFromGPS(lastGPSLat, lastGPSLon, lastGPSTime, 
                                            gpsData.latitude, gpsData.longitude, currentTime);
    } else {
      gpsData.speed = 0.0;
    }
    
    // Cập nhật điểm GPS trước
    lastGPSLat = gpsData.latitude;
    lastGPSLon = gpsData.longitude;
    lastGPSTime = currentTime;
    hasLastGPSPoint = true;
  } else {
    gpsData.speed = 0.0;
  }
  
  if (gps.satellites.isValid()) {
    gpsData.satellites = gps.satellites.value();
  } else {
    gpsData.satellites = 0;
  }
  
  if (gps.hdop.isValid()) {
    gpsData.hdop = gps.hdop.hdop();
  } else {
    gpsData.hdop = 99.9;  // Invalid
  }
  
  if (gps.time.isValid()) {
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
    gpsData.time = String(timeStr);
  } else {
    gpsData.time = "00:00:00";
  }
  
  if (gps.date.isValid()) {
    char dateStr[9];
    sprintf(dateStr, "%02d/%02d/%02d", gps.date.day(), gps.date.month(), gps.date.year() % 100);
    gpsData.date = String(dateStr);
  } else {
    gpsData.date = "01/01/00";
  }
  
  // Tính Course (hướng di chuyển)
  if (gps.course.isValid()) {
    // Ưu tiên dùng course từ GPS nếu có
    gpsData.course = gps.course.deg();
  } else if (gps.location.isValid() && hasLastGPSPoint) {
    // Nếu GPS không có course, tính từ 2 điểm GPS
    gpsData.course = calculateCourseFromGPS(lastGPSLat, lastGPSLon, 
                                            gpsData.latitude, gpsData.longitude);
  } else {
    gpsData.course = 0.0;
  }
}

// ============================================
// HÀM TÍNH VẬN TỐC TỪ 2 ĐIỂM GPS
// ============================================
float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  const float R = 6371000.0;  // Bán kính Trái Đất (mét)
  float dLat = (lat2 - lat1) * PI / 180.0;
  float dLon = (lon2 - lon1) * PI / 180.0;
  float a = sin(dLat / 2.0) * sin(dLat / 2.0) +
            cos(lat1 * PI / 180.0) * cos(lat2 * PI / 180.0) *
            sin(dLon / 2.0) * sin(dLon / 2.0);
  float c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
  return R * c;  // Khoảng cách tính bằng mét
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

float calculateCourseFromGPS(float lat1, float lon1, float lat2, float lon2) {
  // Tính hướng di chuyển (bearing) từ điểm 1 đến điểm 2
  float dLon = (lon2 - lon1) * PI / 180.0;
  float lat1Rad = lat1 * PI / 180.0;
  float lat2Rad = lat2 * PI / 180.0;
  
  float y = sin(dLon) * cos(lat2Rad);
  float x = cos(lat1Rad) * sin(lat2Rad) - sin(lat1Rad) * cos(lat2Rad) * cos(dLon);
  
  float bearing = atan2(y, x) * 180.0 / PI;
  
  // Chuyển từ -180..180 sang 0..360
  if (bearing < 0) {
    bearing += 360.0;
  }
  
  return bearing;
}

// ============================================
// HÀM ĐỌC DHT11
// ============================================
void updateDHT() {
  unsigned long now = millis();
  
  // DHT11 cần thời gian giữa các lần đọc (tối thiểu 2 giây)
  if (now - lastDHTRead >= DHT_READ_INTERVAL) {
    lastDHTRead = now;
    
    // Đọc nhiệt độ và độ ẩm
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    
    // Kiểm tra dữ liệu hợp lệ
    if (!isnan(temp) && !isnan(hum)) {
      dhtData.temperature = temp;
      dhtData.humidity = hum;
      dhtData.valid = true;
    } else {
      dhtData.valid = false;
      // Giữ giá trị cũ nếu đọc lỗi
    }
  }
}

// ============================================
// HÀM KHỞI TẠO SIM 4G
// ============================================
void initSIMModule() {
  Serial.println("\n[INIT] Initializing SIM 4G module with TinyGSM...");
  
  // Khởi tạo Serial cho SIM 4G
  SerialSIM.begin(SIM_BAUD, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);
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
  String rxString = SerialSIM.readString();
  Serial.print("Got: ");
  Serial.println(rxString);
  
  // Kiểm tra SIM card
  Serial.println("[INFO] SIM card check");
  SerialSIM.println("AT+CPIN?");
  delay(1000);
  rxString = SerialSIM.readString();
  Serial.print("Got: ");
  Serial.println(rxString);
  
  // Lấy tên modem
  String name = modem.getModemName();
  delay(500);
  Serial.println("Modem Name: " + name);
  
  // Chờ mạng
  Serial.print("[INFO] Waiting for network...");
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    delay(1000);
    return;
  }
  Serial.println(" success");
  
  if (modem.isNetworkConnected()) {
    Serial.println("[OK] Network connected");
  }
  
  // Kết nối GPRS (cần cho một số chức năng)
  Serial.print(F("[INFO] Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    delay(1000);
    // Vẫn tiếp tục dù GPRS fail (SMS và Call không cần GPRS)
  } else {
    Serial.println(" success");
    if (modem.isGprsConnected()) {
      Serial.println("[OK] GPRS connected");
      sim4gNetworkOpen = true;
    }
  }
  
  // Cấu hình SMS text mode
  Serial.println("[INFO] Configuring SMS text mode...");
  SerialSIM.println("AT+CMGF=1");
  delay(1000);
  rxString = SerialSIM.readString();
  Serial.print("Got: ");
  Serial.println(rxString);
  
  // Kiểm tra tín hiệu mạng
  Serial.println("[INFO] Checking signal strength...");
  SerialSIM.println("AT+CSQ");
  delay(1000);
  rxString = SerialSIM.readString();
  Serial.print("Signal: ");
  Serial.println(rxString);
  
  sim4gInitialized = true;
  Serial.println("[OK] SIM 4G initialized successfully");
}

// ============================================
// HÀM GỬI SMS THẬT
// ============================================
bool sendRealSMS(String phoneNumber, String message) {
  if (!sim4gInitialized) {
    Serial.println("[ERROR] SIM 4G not initialized!");
    return false;
  }
  
  Serial.println("\n[SMS] Preparing to send SMS...");
  Serial.printf("[SMS] To: %s\n", phoneNumber.c_str());
  Serial.println("[SMS] Message: " + message);
  
  if (message.length() > 160) {
    message = message.substring(0, 160);
  }
  
  Serial.println("[SMS] Sending...");
  
  // Gửi lệnh AT để gửi SMS
  SerialSIM.print("AT+CMGS=\"");
  SerialSIM.print(phoneNumber);
  SerialSIM.println("\"");
  delay(1500);
  
  // Đọc prompt "> " từ modem
  String prompt = SerialSIM.readString();
  
  // Gửi nội dung SMS
  SerialSIM.print(message);
  delay(500);
  SerialSIM.write(0x1A);  // Ctrl+Z để kết thúc
  delay(3000);
  
  // Đọc phản hồi với timeout
  String response = "";
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    if (SerialSIM.available()) {
      char c = SerialSIM.read();
      response += c;
      if (response.length() > 512) {
        response = response.substring(response.length() - 512);
      }
    }
    delay(10);
  }
  
  Serial.print("[SMS] Response: ");
  Serial.println(response);
  
  // Kiểm tra response
  if (response.indexOf("+CMS ERROR") != -1) {
    Serial.println("[ERROR] SMS sending failed!");
    return false;
  } else if (response.indexOf("+CMGS:") != -1 && response.indexOf("OK") != -1) {
    Serial.println("[SUCCESS] SMS sent successfully!");
    return true;
  } else {
    Serial.println("[ERROR] SMS sending failed! No valid response.");
    return false;
  }
}

// ============================================
// HÀM GỌI ĐIỆN THẬT
// ============================================
bool makeRealCall(String phoneNumber) {
  if (!sim4gInitialized) {
    Serial.println("[ERROR] SIM 4G not initialized!");
    return false;
  }
  
  Serial.println("\n[CALL] Preparing to make a call...");
  Serial.printf("[CALL] To: %s\n", phoneNumber.c_str());
  Serial.println("[CALL] Dialing...");
  
  // Gửi lệnh AT để gọi điện
  SerialSIM.print("ATD");
  SerialSIM.print(phoneNumber);
  SerialSIM.println(";");  // Dấu ; để gọi voice call
  delay(2000);
  
  // Đọc phản hồi
  String response = SerialSIM.readString();
  Serial.print("[CALL] Response: ");
  Serial.println(response);
  
  if (response.indexOf("OK") != -1 || response.indexOf("CONNECT") != -1) {
    Serial.println("[SUCCESS] Call initiated!");
    return true;
  } else if (response.indexOf("BUSY") != -1) {
    Serial.println("[WARNING] Number is busy!");
    return false;
  } else if (response.indexOf("NO ANSWER") != -1) {
    Serial.println("[WARNING] No answer!");
    return false;
  } else {
    Serial.println("[INFO] Call status unknown.");
    return false;
  }
}

// ============================================
// BACKEND TELEMETRY HELPERS
// ============================================
void markTelemetryForImmediateSend() {
  telemetryPendingImmediateSend = true;
}

String determineCrashSeverity(bool isManual, float gForce) {
  if (isManual) {
    return "HIGH";
  }
  if (gForce >= 6.0) {
    return "CRITICAL";
  }
  if (gForce >= gCrashThreshold) {
    return "HIGH";
  }
  return "NORMAL";
}

String buildTelemetryPayload(const String& eventType, const String& severity) {
  String payload = "{";
  payload += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  payload += "\"gmail\":\"" + String(GMAIL_ACCOUNT) + "\",";
  payload += "\"pass\":\"" + String(GMAIL_PASSWORD) + "\",";
  payload += "\"timestamp\":\"" + getRTCDateTimeString() + "\",";
  payload += "\"event_type\":\"" + eventType + "\",";
  payload += "\"severity\":\"" + severity + "\",";
  payload += "\"location\":{";
  if (gpsData.valid) {
    payload += "\"latitude\":" + String(gpsData.latitude, 6) + ",";
    payload += "\"longitude\":" + String(gpsData.longitude, 6);
  } else {
    payload += "\"latitude\":0,";
    payload += "\"longitude\":0";
  }
  payload += "}";
  payload += "}";
  return payload;
}

bool ensureNetworkReady(bool verbose = true) {
  if (!sim4gInitialized) {
    if (verbose) {
      Serial.println("[NET] ⚠️ SIM module chưa khởi tạo, thử khởi tạo lại...");
    }
    initSIMModule();
  }

  bool networkConnected = modem.isNetworkConnected();
  if (!networkConnected) {
    if (verbose) Serial.println("[NET] 📶 Đang đợi mạng...");
    networkConnected = modem.waitForNetwork(60000);
  }

  bool gprsConnected = modem.isGprsConnected();
  if (networkConnected && !gprsConnected) {
    if (verbose) Serial.println("[NET] 🌐 Đang kết nối GPRS...");
    modem.gprsDisconnect();
    gprsConnected = modem.gprsConnect(apn, user, pass);
  }

  sim4gNetworkOpen = networkConnected && gprsConnected;

  if (!sim4gNetworkOpen && verbose) {
    Serial.println("[NET] ❌ Không thể mở kết nối GPRS");
  }

  return sim4gNetworkOpen;
}

bool sendTelemetryToServer(const String& eventType, const String& severity, bool verbose = true) {
  if (!ensureNetworkReady(verbose)) {
    if (verbose) Serial.println("[HTTP] ⚠️ Network not available, skip telemetry");
    return false;
  }

  String payload = buildTelemetryPayload(eventType, severity);
  HttpClient httpClient(gsmClient, SERVER_HOST, SERVER_PORT);

  if (verbose) {
    Serial.println("[HTTP] 🚀 Sending telemetry to server...");
    Serial.println("[HTTP] Payload: " + payload);
  }

  httpClient.post(TELEMETRY_ENDPOINT, "application/json", payload);
  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();
  httpClient.stop();

  if (verbose) {
    Serial.print("[HTTP] Status: ");
    Serial.println(statusCode);
    Serial.print("[HTTP] Response: ");
    Serial.println(response);
  }

  if (statusCode >= 200 && statusCode < 300) {
    lastTelemetrySendTime = millis();
    telemetryPendingImmediateSend = false;
    return true;
  }

  return false;
}

void updateEventState(const String& newEvent, const String& newSeverity, bool sendNow = true) {
  if (currentEventType == newEvent && currentSeverity == newSeverity && !sendNow) {
    return;
  }

  currentEventType = newEvent;
  currentSeverity = newSeverity;

  if (newEvent == "CRASH") {
    hasActiveSOSOnServer = true;
    ignoreServerCancelUntil = millis() + 15000; // chờ server tạo SOS
    awaitingServerSOSConfirmation = true;
  } else if (newEvent == "NORMAL") {
    hasActiveSOSOnServer = false;
    currentSOSId = "";
    currentSOSStatus = "";
    ignoreServerCancelUntil = 0;
    awaitingServerSOSConfirmation = false;
  }

  if (sendNow) {
    markTelemetryForImmediateSend();
  }

  Serial.print("[EVENT] Updated event_type=");
  Serial.print(currentEventType);
  Serial.print(", severity=");
  Serial.println(currentSeverity);
}

void handleSOSCancelledByServer() {
  Serial.println("[SOS] ⚠️ SOS bị hủy từ server - đưa thiết bị về trạng thái NORMAL");
  systemState = STATE_NORMAL;
  isManualSOS = false;
  sosCancelTime = millis();
  buzzerState = false;
  digitalWrite(BUZZER_PIN, LOW);
  updateEventState("NORMAL", "LOW", true);
}

void checkSOSStatusFromServer() {
  if (!ensureNetworkReady(false)) {
    return;
  }

  HttpClient httpClient(gsmClient, SERVER_HOST, SERVER_PORT);
  String endpoint = String(SOS_STATUS_ENDPOINT_PREFIX) + String(DEVICE_ID);
  httpClient.get(endpoint.c_str());

  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();
  httpClient.stop();

  if (statusCode != 200) {
    Serial.print("[SOS] ❌ Check failed, status: ");
    Serial.println(statusCode);
    return;
  }

  if (response.indexOf("\"hasActiveSOS\":true") >= 0) {
    hasActiveSOSOnServer = true;
    awaitingServerSOSConfirmation = false;

    int sosIdStart = response.indexOf("\"sosId\":\"");
    if (sosIdStart >= 0) {
      sosIdStart += 9;
      int sosIdEnd = response.indexOf("\"", sosIdStart);
      if (sosIdEnd > sosIdStart) {
        currentSOSId = response.substring(sosIdStart, sosIdEnd);
      }
    }

    int statusStart = response.indexOf("\"status\":\"");
    if (statusStart >= 0) {
      statusStart += 10;
      int statusEnd = response.indexOf("\"", statusStart);
      if (statusEnd > statusStart) {
        currentSOSStatus = response.substring(statusStart, statusEnd);
      }
    }

    Serial.print("[SOS] ✅ Active SOS on server: ");
    Serial.print(currentSOSId);
    Serial.print(" (status: ");
    Serial.print(currentSOSStatus);
    Serial.println(")");
  } else {
    unsigned long nowMs = millis();
    bool deferServerCancel = (nowMs < ignoreServerCancelUntil) ||
      awaitingServerSOSConfirmation ||
      (systemState == STATE_CRASH_DETECTED) ||
      (systemState == STATE_COUNTDOWN);

    if (deferServerCancel) {
      Serial.println("[SOS] ⏳ Bỏ qua thông báo hủy từ server (đang đợi xác nhận SOS)");
      return;
    }

    if (hasActiveSOSOnServer) {
      hasActiveSOSOnServer = false;
      handleSOSCancelledByServer();
    } else {
      Serial.println("[SOS] ℹ️ No active SOS on server");
    }
  }
}


// ============================================
// HÀM GỬI TIN NHẮN VÀ GỌI ĐIỆN (THẬT)
// ============================================
void simulateSendSMSAndCall(bool isManual = false) {
  Serial.println("\n============================================");
  if (isManual) {
    Serial.println("*** GỬI TIN NHẮN & GỌI ĐIỆN (THỦ CÔNG) ***");
    Serial.println("*** Mức độ: TRUNG BÌNH (Thấp hơn tự động) ***");
  } else {
    Serial.println("*** GỬI TIN NHẮN & GỌI ĐIỆN (TỰ ĐỘNG) ***");
    Serial.println("*** Mức độ: NGHIÊM TRỌNG ***");
  }
  Serial.println("============================================");
  
  // Đọc thời gian từ DS1307 mỗi lần log
  if (ds1307Initialized) {
    readDS1307Time();
    if (rtcData.valid) {
      Serial.print("🕐 Thời gian log (RTC): ");
      Serial.println(getRTCDateTimeString());
    }
  }
  
  // Cập nhật GPS trước khi gửi
  updateGPS();
  
  // In thông tin định vị GPS
  Serial.println("\n📍 THÔNG TIN ĐỊNH VỊ GPS:");
  if (gpsData.valid) {
    Serial.print("  Vĩ độ: ");
    Serial.print(gpsData.latitude, 6);
    Serial.print("° (Lat: ");
    Serial.print(gpsData.latitude, 6);
    Serial.println("°)");
    Serial.print("  Kinh độ: ");
    Serial.print(gpsData.longitude, 6);
    Serial.print("° (Lon: ");
    Serial.print(gpsData.longitude, 6);
    Serial.println("°)");
    
    if (gpsData.altitude > 0) {
      Serial.print("  Độ cao: ");
      Serial.print(gpsData.altitude, 1);
      Serial.println(" m");
    }
    
    if (gpsData.speed > 0) {
      Serial.print("  Tốc độ: ");
      Serial.print(gpsData.speed, 2);
      Serial.println(" km/h");
    }
    
    Serial.print("  Số vệ tinh: ");
    Serial.println(gpsData.satellites);
    
    if (gpsData.hdop < 99.0) {
      Serial.print("  Độ chính xác (HDOP): ");
      Serial.print(gpsData.hdop, 2);
      if (gpsData.hdop < 1.0) {
        Serial.println(" (Excellent)");
      } else if (gpsData.hdop < 2.0) {
        Serial.println(" (Good)");
      } else if (gpsData.hdop < 5.0) {
        Serial.println(" (Moderate)");
      } else {
        Serial.println(" (Poor)");
      }
    }
    
    if (gpsData.course > 0) {
      Serial.print("  Hướng di chuyển: ");
      Serial.print(gpsData.course, 1);
      Serial.println("°");
    }
    
    Serial.print("  Thời gian GPS (UTC): ");
    Serial.print(gpsData.time);
    Serial.print(" - Ngày: ");
    Serial.println(gpsData.date);
    
    // Hiển thị thời gian từ RTC
    if (rtcData.valid) {
      Serial.print("  Thời gian RTC (Local): ");
      Serial.println(getRTCDateTimeString());
    }
    
    // Tạo link Google Maps
    Serial.print("  Link Google Maps: ");
    Serial.print("https://www.google.com/maps?q=");
    Serial.print(gpsData.latitude, 6);
    Serial.print(",");
    Serial.println(gpsData.longitude, 6);
  } else {
    Serial.println("  ⚠️ Không có tín hiệu GPS!");
    Serial.println("  Vị trí: Không xác định");
    Serial.println("  (GPS đang chờ lock vệ tinh)");
  }
  
  // In trạng thái cảnh báo
  Serial.println("\n🚨 TRẠNG THÁI CẢNH BÁO:");
  if (isManual) {
    Serial.println("  Loại: CẢNH BÁO THỦ CÔNG");
    Serial.println("  Mức độ: TRUNG BÌNH");
    Serial.println("  (Người dùng nhấn nút SOS thủ công)");
  } else {
    Serial.println("  Loại: TAI NẠN XE");
    Serial.println("  Mức độ: NGHIÊM TRỌNG");
    Serial.println("  (Hệ thống tự động phát hiện)");
  }
  Serial.print("  G-force: ");
  Serial.print(imuData.g_total, 2);
  Serial.println(" g");
  Serial.print("  Góc nghiêng: Roll=");
  Serial.print(imuData.roll, 1);
  Serial.print("°, Pitch=");
  Serial.print(imuData.pitch, 1);
  Serial.println("°");
  Serial.print("  Delta Angle: ");
  Serial.print(imuData.deltaAngle, 1);
  Serial.println("°");
  
  // Gửi SMS thật
  Serial.println("\n📱 GỬI TIN NHẮN SMS:");
  String smsMessage = "";
  if (isManual) {
    smsMessage = "[SOS MANUAL] CANH BAO THU CONG!";
  } else {
    smsMessage = "[SOS] TAI NAN XE!";
  }
  smsMessage += "\n";
  if (gpsData.valid) {
    smsMessage += "Vi tri: ";
    smsMessage += String(gpsData.latitude, 6);
    smsMessage += ", ";
    smsMessage += String(gpsData.longitude, 6);
    smsMessage += "\n";
    smsMessage += "Link: https://www.google.com/maps?q=";
    smsMessage += String(gpsData.latitude, 6);
    smsMessage += ",";
    smsMessage += String(gpsData.longitude, 6);
  } else {
    smsMessage += "Vi tri: Khong xac dinh";
  }
  smsMessage += "\n";
  if (rtcData.valid) {
    smsMessage += "Thoi gian: ";
    smsMessage += getRTCDateTimeString();
  }
  
  bool smsResult = sendRealSMS(String(PHONE_NUMBER), smsMessage);
  if (smsResult) {
    Serial.println("  ✓ Tin nhắn đã được gửi thành công!");
  } else {
    Serial.println("  ✗ Gửi tin nhắn thất bại!");
  }
  
  // Gọi điện thật
  Serial.println("\n📞 GỌI ĐIỆN:");
  bool callResult = makeRealCall(String(PHONE_NUMBER));
  if (callResult) {
    Serial.println("  ✓ Cuộc gọi đã được thực hiện");
    Serial.println("  (Cuộc gọi sẽ tự động kết thúc sau một khoảng thời gian)");
  } else {
    Serial.println("  ✗ Gọi điện thất bại!");
  }
  
  // Mô phỏng gửi đến server
  Serial.println("\n🌐 MÔ PHỎNG GỬI ĐẾN SERVER:");
  Serial.println("  Đang kết nối WiFi/Internet...");
  Serial.println("  ✓ Đã kết nối");
  Serial.println("  Đang gửi HTTP POST request...");
  Serial.println("  Endpoint: https://api.example.com/sos");
  Serial.println("  Payload:");
  Serial.println("    {");
  if (isManual) {
    Serial.println("      \"type\": \"MANUAL_SOS\",");
    Serial.println("      \"severity\": \"MEDIUM\",");
  } else {
    Serial.println("      \"type\": \"CRASH\",");
    Serial.println("      \"severity\": \"HIGH\",");
  }
  if (gpsData.valid) {
    Serial.print("      \"latitude\": ");
    Serial.print(gpsData.latitude, 6);
    Serial.println(",");
    Serial.print("      \"longitude\": ");
    Serial.print(gpsData.longitude, 6);
    Serial.println(",");
  }
  Serial.print("      \"g_force\": ");
  Serial.print(imuData.g_total, 2);
  Serial.println(",");
  Serial.print("      \"roll\": ");
  Serial.print(imuData.roll, 1);
  Serial.println(",");
  Serial.print("      \"pitch\": ");
  Serial.print(imuData.pitch, 1);
  Serial.println(",");
  Serial.print("      \"delta_angle\": ");
  Serial.print(imuData.deltaAngle, 1);
  Serial.println(",");
  if (rtcData.valid) {
    Serial.print("      \"timestamp\": \"");
    Serial.print(getRTCDateTimeString());
    Serial.println("\"");
  } else {
    Serial.println("      \"timestamp\": null");
  }
  Serial.println("    }");
  Serial.println("  ✓ Dữ liệu đã được gửi đến server thành công!");
  
  Serial.println("\n============================================");
  Serial.println("✓ TẤT CẢ THÔNG TIN ĐÃ ĐƯỢC GỬI!");
  Serial.println("============================================\n");
}

// ============================================
// HÀM GHI LOG VÀO SD CARD
// ============================================
bool initSDCard() {
  // Cấu hình SPI với các chân đã định nghĩa
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  
  if (!SD.begin(SD_CS_PIN)) {
    return false;  // Không có SD card, nhưng không báo lỗi ở đây
  }
  
  // Kiểm tra dung lượng
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.print("  Card size: ");
  Serial.print(cardSize);
  Serial.println(" MB");
  
  return true;
}

String formatLogEntry() {
  String logEntry = "";
  
  // Thời gian RTC
  if (rtcData.valid) {
    logEntry += getRTCDateTimeString();
  } else {
    logEntry += "N/A";
  }
  logEntry += ",";
  
  // IMU Data
  logEntry += String(imuData.ax, 3) + ",";
  logEntry += String(imuData.ay, 3) + ",";
  logEntry += String(imuData.az, 3) + ",";
  logEntry += String(imuData.g_total, 3) + ",";
  logEntry += String(imuData.roll, 2) + ",";
  logEntry += String(imuData.pitch, 2) + ",";
  logEntry += String(imuData.deltaAngle, 2) + ",";
  
  // GPS Data
  if (gpsData.valid) {
    logEntry += String(gpsData.latitude, 6) + ",";
    logEntry += String(gpsData.longitude, 6) + ",";
    logEntry += String(gpsData.altitude, 2) + ",";
    logEntry += String(gpsData.speed, 2) + ",";
    logEntry += String(gpsData.satellites) + ",";
  } else {
    logEntry += "N/A,N/A,N/A,N/A,N/A,";
  }
  
  // DHT11 Data
  if (dhtData.valid) {
    logEntry += String(dhtData.temperature, 1) + ",";
    logEntry += String(dhtData.humidity, 1);
  } else {
    logEntry += "N/A,N/A";
  }
  
  // Trạng thái
  logEntry += ",NORMAL";
  
  return logEntry;
}

String formatCrashLogEntry(bool isManual) {
  String logEntry = "";
  
  // Thời gian RTC
  if (rtcData.valid) {
    logEntry += getRTCDateTimeString();
  } else {
    logEntry += "N/A";
  }
  logEntry += ",";
  
  // Loại va chạm
  if (isManual) {
    logEntry += "MANUAL_SOS,";
  } else {
    logEntry += "AUTO_CRASH,";
  }
  
  // IMU Data
  logEntry += String(imuData.ax, 3) + ",";
  logEntry += String(imuData.ay, 3) + ",";
  logEntry += String(imuData.az, 3) + ",";
  logEntry += String(imuData.g_total, 3) + ",";
  logEntry += String(imuData.roll, 2) + ",";
  logEntry += String(imuData.pitch, 2) + ",";
  logEntry += String(imuData.deltaAngle, 2) + ",";
  
  // GPS Data
  if (gpsData.valid) {
    logEntry += String(gpsData.latitude, 6) + ",";
    logEntry += String(gpsData.longitude, 6) + ",";
    logEntry += String(gpsData.altitude, 2) + ",";
    logEntry += String(gpsData.speed, 2) + ",";
    logEntry += String(gpsData.satellites) + ",";
    logEntry += "https://www.google.com/maps?q=";
    logEntry += String(gpsData.latitude, 6) + ",";
    logEntry += String(gpsData.longitude, 6) + ",";
  } else {
    logEntry += "N/A,N/A,N/A,N/A,N/A,N/A,";
  }
  
  // DHT11 Data
  if (dhtData.valid) {
    logEntry += String(dhtData.temperature, 1) + ",";
    logEntry += String(dhtData.humidity, 1);
  } else {
    logEntry += "N/A,N/A";
  }
  
  return logEntry;
}

void writeLogToSD() {
  if (!sdCardInitialized) return;
  
  File logFile = SD.open(LOG_FILE, FILE_WRITE);
  if (logFile) {
    String logEntry = formatLogEntry();
    logFile.println(logEntry);
    logFile.close();
    Serial.println("✓ Log written to SD card");
  } else {
    Serial.println("✗ Error writing to log file");
  }
}

void writeCrashLogToSD(bool isManual) {
  if (!sdCardInitialized) {
    // Không in gì, chỉ bỏ qua việc ghi log
    return;
  }
  
  File crashFile = SD.open(CRASH_LOG_FILE, FILE_WRITE);
  if (crashFile) {
    String logEntry = formatCrashLogEntry(isManual);
    crashFile.println(logEntry);
    crashFile.close();
    Serial.println("✓ CRASH LOG written to SD card immediately!");
  } else {
    Serial.println("✗ Error writing to crash log file");
  }
}

// ============================================
// HÀM EEPROM - Lưu/Đọc dữ liệu
// ============================================
void saveToEEPROM() {
  int addr = EEPROM_START_ADDR;
  
  // Lưu DEVICE_ID (32 bytes)
  for (int i = 0; i < 32; i++) {
    EEPROM.write(addr++, DEVICE_ID[i]);
  }
  
  // Lưu GMAIL_ACCOUNT (64 bytes)
  for (int i = 0; i < 64; i++) {
    EEPROM.write(addr++, GMAIL_ACCOUNT[i]);
  }
  
  // Lưu GMAIL_PASSWORD (64 bytes)
  for (int i = 0; i < 64; i++) {
    EEPROM.write(addr++, GMAIL_PASSWORD[i]);
  }
  
  // Lưu ngưỡng cảnh báo (float = 4 bytes)
  uint8_t* gPtr = (uint8_t*)&gCrashThreshold;
  for (int i = 0; i < 4; i++) {
    EEPROM.write(addr++, gPtr[i]);
  }
  uint8_t* anglePtr = (uint8_t*)&angleCrashThreshold;
  for (int i = 0; i < 4; i++) {
    EEPROM.write(addr++, anglePtr[i]);
  }
  
  // Lưu allowSOSSent
  EEPROM.write(addr++, allowSOSSent ? 1 : 0);
  
  // Lưu countdownTime
  EEPROM.write(addr++, countdownTime);
  
  // Lưu PHONE_NUMBER (20 bytes)
  for (int i = 0; i < 20; i++) {
    EEPROM.write(addr++, PHONE_NUMBER[i]);
  }
  
  EEPROM.commit();
  Serial.println("✓ Đã lưu dữ liệu vào EEPROM");
}

void loadFromEEPROM() {
  int addr = EEPROM_START_ADDR;
  
  // Đọc DEVICE_ID (32 bytes)
  for (int i = 0; i < 32; i++) {
    DEVICE_ID[i] = EEPROM.read(addr++);
  }
  DEVICE_ID[31] = '\0';  // Đảm bảo kết thúc chuỗi
  
  // Đọc GMAIL_ACCOUNT (64 bytes)
  for (int i = 0; i < 64; i++) {
    GMAIL_ACCOUNT[i] = EEPROM.read(addr++);
  }
  GMAIL_ACCOUNT[63] = '\0';
  
  // Đọc GMAIL_PASSWORD (64 bytes)
  for (int i = 0; i < 64; i++) {
    GMAIL_PASSWORD[i] = EEPROM.read(addr++);
  }
  GMAIL_PASSWORD[63] = '\0';
  
  // Đọc ngưỡng cảnh báo (float = 4 bytes)
  uint8_t gBytes[4];
  for (int i = 0; i < 4; i++) {
    gBytes[i] = EEPROM.read(addr++);
  }
  memcpy(&gCrashThreshold, gBytes, 4);
  
  uint8_t angleBytes[4];
  for (int i = 0; i < 4; i++) {
    angleBytes[i] = EEPROM.read(addr++);
  }
  memcpy(&angleCrashThreshold, angleBytes, 4);
  
  // Đọc allowSOSSent
  allowSOSSent = (EEPROM.read(addr++) == 1);
  
  // Đọc countdownTime
  countdownTime = EEPROM.read(addr++);
  
  // Đọc PHONE_NUMBER (20 bytes)
  for (int i = 0; i < 20; i++) {
    PHONE_NUMBER[i] = EEPROM.read(addr++);
  }
  PHONE_NUMBER[19] = '\0';  // Đảm bảo kết thúc chuỗi
  
  Serial.println("✓ Đã đọc dữ liệu từ EEPROM");
  Serial.printf("  DEVICE_ID: %s\n", DEVICE_ID);
  Serial.printf("  GMAIL: %s\n", GMAIL_ACCOUNT);
  Serial.printf("  PHONE: %s\n", PHONE_NUMBER);
  Serial.printf("  G_CRASH: %.2f\n", gCrashThreshold);
  Serial.printf("  ANGLE_CRASH: %.2f\n", angleCrashThreshold);
  Serial.printf("  Allow SOS: %s\n", allowSOSSent ? "YES" : "NO");
  Serial.printf("  Countdown: %d s\n", countdownTime);
}

// ============================================
// HÀM WEB SERVER - HTML Interface
// ============================================
String getHTMLPage() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Hộp Đen Thông Minh</title>";
  html += "<style>";
  html += "* { box-sizing: border-box; }";
  html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }";
  html += ".container { background: white; padding: 30px; border-radius: 20px; max-width: 550px; margin: 0 auto; box-shadow: 0 10px 40px rgba(0,0,0,0.2); }";
  html += "h1 { color: #2c3e50; margin: 0 0 10px 0; font-size: 28px; text-align: center; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); -webkit-background-clip: text; -webkit-text-fill-color: transparent; background-clip: text; }";
  html += ".subtitle { text-align: center; color: #7f8c8d; margin-bottom: 25px; font-size: 14px; }";
  html += "label { display: block; margin-top: 20px; margin-bottom: 8px; font-weight: 600; color: #34495e; font-size: 14px; }";
  html += "input[type='text'], input[type='email'], input[type='password'], input[type='tel'], input[type='number'], select { width: 100%; padding: 12px 15px; margin-top: 5px; border: 2px solid #e0e0e0; border-radius: 8px; font-size: 14px; transition: all 0.3s; background: #f8f9fa; }";
  html += "input:focus, select:focus { outline: none; border-color: #667eea; background: white; box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1); }";
  html += "small { display: block; margin-top: 5px; color: #95a5a6; font-size: 12px; }";
  html += "button { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 15px 30px; border: none; border-radius: 10px; cursor: pointer; margin-top: 25px; width: 100%; font-size: 16px; font-weight: 600; transition: all 0.3s; box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4); }";
  html += "button:hover { transform: translateY(-2px); box-shadow: 0 6px 20px rgba(102, 126, 234, 0.6); }";
  html += "button:active { transform: translateY(0); }";
  html += ".status { margin-top: 20px; padding: 15px; background: linear-gradient(135deg, #e8f5e9 0%, #c8e6c9 100%); border-radius: 10px; border-left: 4px solid #4caf50; }";
  html += ".status strong { color: #2e7d32; }";
  html += ".form-group { margin-bottom: 5px; }";
  html += ".icon { margin-right: 8px; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>🚗 Hộp Đen Thông Minh</h1>";
  html += "<p class='subtitle'>Cấu hình hệ thống phát hiện va chạm</p>";
  html += "<form action='/save' method='POST'>";
  
  // Device ID
  html += "<div class='form-group'>";
  html += "<label><span class='icon'>🆔</span>ID Thiết Bị:</label>";
  html += "<input type='text' name='device_id' value='" + String(DEVICE_ID) + "' maxlength='31' placeholder='Nhập ID thiết bị'>";
  html += "</div>";
  
  // Gmail Account
  html += "<div class='form-group'>";
  html += "<label><span class='icon'>📧</span>Gmail:</label>";
  html += "<input type='email' name='gmail' value='" + String(GMAIL_ACCOUNT) + "' maxlength='63' placeholder='example@gmail.com'>";
  html += "</div>";
  
  // Gmail Password
  html += "<div class='form-group'>";
  html += "<label><span class='icon'>🔒</span>Mật Khẩu Gmail:</label>";
  html += "<input type='password' name='gmail_pass' value='" + String(GMAIL_PASSWORD) + "' maxlength='63' placeholder='Nhập mật khẩu'>";
  html += "</div>";
  
  // Phone Number
  html += "<div class='form-group'>";
  html += "<label><span class='icon'>📱</span>Số Điện Thoại (Nhận SMS & Gọi Điện):</label>";
  html += "<input type='tel' name='phone_number' value='" + String(PHONE_NUMBER) + "' maxlength='19' placeholder='0798169921'>";
  html += "<small>Ví dụ: 0798169921 hoặc +84798169921</small>";
  html += "</div>";
  
  // G Crash Threshold
  html += "<div class='form-group'>";
  html += "<label><span class='icon'>⚡</span>Ngưỡng G-Force (G_CRASH):</label>";
  html += "<input type='number' name='g_crash' value='" + String(gCrashThreshold, 2) + "' step='0.1' min='1.0' max='10.0'>";
  html += "<small>Giá trị G-force để phát hiện va chạm (mặc định: 3.5g)</small>";
  html += "</div>";
  
  // Angle Crash Threshold
  html += "<div class='form-group'>";
  html += "<label><span class='icon'>📐</span>Ngưỡng Góc (ANGLE_CRASH):</label>";
  html += "<input type='number' name='angle_crash' value='" + String(angleCrashThreshold, 2) + "' step='1.0' min='10.0' max='90.0'>";
  html += "<small>Góc nghiêng để phát hiện lật xe (mặc định: 35 độ)</small>";
  html += "</div>";
  
  // Allow SOS Sent
  html += "<div class='form-group'>";
  html += "<label><span class='icon'>🚨</span>Cho Phép Cảnh Báo (Tự Động & Thủ Công):</label>";
  html += "<select name='allow_sos'>";
  html += "<option value='1'" + String(allowSOSSent ? " selected" : "") + ">✅ BẬT - Cho phép phát hiện va chạm tự động và SOS thủ công</option>";
  html += "<option value='0'" + String(!allowSOSSent ? " selected" : "") + ">❌ TẮT - Tắt tất cả cảnh báo (tự động và thủ công)</option>";
  html += "</select>";
  html += "</div>";
  
  // Countdown Time
  html += "<div class='form-group'>";
  html += "<label><span class='icon'>⏱️</span>Thời Gian Đếm Ngược (giây):</label>";
  html += "<input type='number' name='countdown' value='" + String(countdownTime) + "' min='10' max='60'>";
  html += "<small>Thời gian đếm ngược trước khi gửi SOS (10-60 giây)</small>";
  html += "</div>";
  
  html += "<button type='submit'>💾 LƯU VÀO EEPROM</button>";
  html += "</form>";
  
  html += "<div class='status'>";
  html += "<strong>📡 Thông Tin Kết Nối:</strong><br>";
  html += "🌐 WiFi: <strong>" + String(AP_SSID) + "</strong><br>";
  html += "📍 IP: <strong>" + WiFi.softAPIP().toString() + "</strong><br>";
  html += "🔑 Mật khẩu: <strong>" + String(AP_PASSWORD) + "</strong><br>";
  html += "<br><small>💡 Giữ nút 36 trong 10 giây để thoát Web Setting</small>";
  html += "</div>";
  
  html += "</div></body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", getHTMLPage());
}

void handleSave() {
  if (server.hasArg("device_id")) {
    server.arg("device_id").toCharArray(DEVICE_ID, 32);
  }
  if (server.hasArg("gmail")) {
    server.arg("gmail").toCharArray(GMAIL_ACCOUNT, 64);
  }
  if (server.hasArg("gmail_pass")) {
    server.arg("gmail_pass").toCharArray(GMAIL_PASSWORD, 64);
  }
  if (server.hasArg("g_crash")) {
    gCrashThreshold = server.arg("g_crash").toFloat();
  }
  if (server.hasArg("angle_crash")) {
    angleCrashThreshold = server.arg("angle_crash").toFloat();
  }
  if (server.hasArg("allow_sos")) {
    allowSOSSent = (server.arg("allow_sos").toInt() == 1);
  }
  if (server.hasArg("countdown")) {
    countdownTime = server.arg("countdown").toInt();
    if (countdownTime < 10) countdownTime = 10;
    if (countdownTime > 60) countdownTime = 60;
  }
  if (server.hasArg("phone_number")) {
    server.arg("phone_number").toCharArray(PHONE_NUMBER, 20);
    PHONE_NUMBER[19] = '\0';  // Đảm bảo kết thúc chuỗi
  }
  
  // Lưu vào EEPROM
  saveToEEPROM();
  
  // Trả về trang thành công
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='3;url=/'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; padding: 50px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; margin: 0; display: flex; align-items: center; justify-content: center; }";
  html += ".success-box { background: white; padding: 40px; border-radius: 20px; box-shadow: 0 10px 40px rgba(0,0,0,0.2); max-width: 400px; }";
  html += "h1 { color: #4caf50; font-size: 32px; margin: 20px 0; }";
  html += "p { color: #7f8c8d; font-size: 16px; }";
  html += ".spinner { border: 4px solid #f3f3f3; border-top: 4px solid #667eea; border-radius: 50%; width: 40px; height: 40px; animation: spin 1s linear infinite; margin: 20px auto; }";
  html += "@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }";
  html += "</style></head><body>";
  html += "<div class='success-box'>";
  html += "<h1>✅ ĐÃ LƯU THÀNH CÔNG!</h1>";
  html += "<p>Đang chuyển về trang chủ...</p>";
  html += "<div class='spinner'></div>";
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void initWebServer() {
  Serial.println("\n============================================");
  Serial.println("🔧 ĐANG KHỞI TẠO WiFi Access Point...");
  Serial.println("============================================");
  
  // Tắt WiFi cũ nếu có (đảm bảo clean state)
  WiFi.disconnect(true);
  delay(200);
  
  // Đặt mode AP trước
  WiFi.mode(WIFI_AP);
  delay(200);
  
  // Cấu hình IP cố định cho AP
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  // Thử cấu hình IP nhiều lần nếu cần
  bool configResult = false;
  for (int retry = 0; retry < 3; retry++) {
    configResult = WiFi.softAPConfig(local_IP, gateway, subnet);
    if (configResult) break;
    delay(100);
  }
  Serial.printf("WiFi Config: %s\n", configResult ? "OK" : "FAILED");
  
  // Khởi tạo AP với retry logic
  bool apResult = false;
  for (int retry = 0; retry < 3; retry++) {
    apResult = WiFi.softAP(AP_SSID, AP_PASSWORD);
    if (apResult) break;
    delay(200);
  }
  Serial.printf("WiFi AP Start: %s\n", apResult ? "OK" : "FAILED");
  
  // Đợi để AP khởi động hoàn toàn - Kiểm tra kỹ hơn
  int waitCount = 0;
  while (waitCount < 30) {  // Tối đa 3 giây
    delay(100);
    IPAddress testIP = WiFi.softAPIP();
    if (testIP.toString() != "0.0.0.0" && testIP == local_IP) {
      Serial.println("✓ AP đã sẵn sàng với IP đúng");
      break;
    }
    waitCount++;
  }
  
  // Kiểm tra lại và khởi động lại nếu cần
  IPAddress finalIP = WiFi.softAPIP();
  if (finalIP.toString() == "0.0.0.0") {
    Serial.println("⚠️ AP chưa có IP - Thử khởi động lại...");
    WiFi.softAPdisconnect(false);
    delay(200);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    delay(500);
    finalIP = WiFi.softAPIP();
  }
  
  // Kiểm tra trạng thái WiFi
  uint8_t stationCount = WiFi.softAPgetStationNum();
  
  // Khởi tạo web server TRƯỚC KHI in thông tin
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  
  Serial.println("============================================");
  Serial.println("🌐 WEB INTERFACE SERVER STARTED");
  Serial.println("============================================");
  Serial.printf("SSID: %s\n", AP_SSID);
  Serial.printf("Password: %s\n", AP_PASSWORD);
  Serial.printf("IP Address: %s\n", finalIP.toString().c_str());
  Serial.printf("WiFi Status: %s\n", WiFi.status() == WL_CONNECTED ? "CONNECTED" : "AP_MODE");
  Serial.printf("AP Status: %s\n", WiFi.softAPgetStationNum() >= 0 ? "ACTIVE" : "INACTIVE");
  Serial.printf("Connected Stations: %d\n", stationCount);
  Serial.println("Kết nối điện thoại vào WiFi này và mở trình duyệt:");
  Serial.printf("http://%s hoặc http://192.168.4.1\n", finalIP.toString().c_str());
  Serial.println("============================================");
  Serial.println("✓ Web Server đã sẵn sàng và đang chạy!");
  Serial.println("============================================\n");
}

void stopWebServer() {
  server.stop();
  WiFi.softAPdisconnect(true);
  Serial.println("✓ Đã tắt Web Interface Server");
}

// ============================================
// HÀM SOS - Gửi tín hiệu cứu hộ
// ============================================
void sendSOS(bool isManual = false) {
  // Kiểm tra xem có cho phép gửi SOS không
  if (!allowSOSSent) {
    Serial.println("============================================");
    Serial.println("⚠️ SOS bị TẮT - Không gửi tín hiệu");
    Serial.println("============================================");
    return;
  }
  
  Serial.println("============================================");
  if (isManual) {
    Serial.println("*** SOS ACTIVATED (THỦ CÔNG) ***");
  } else {
    Serial.println("*** SOS ACTIVATED (TỰ ĐỘNG) ***");
  }
  Serial.println("============================================");
  
  // Gửi telemetry ngay để server nhận event CRASH
  markTelemetryForImmediateSend();
  sendTelemetryToServer(currentEventType, currentSeverity, false);
  Serial.println("[SOS] ⏳ Chờ 5 giây trước khi gửi SMS/gọi điện để đảm bảo gói web đã lên server...");
  delay(5000);

  // Ghi log va chạm vào SD card NGAY LẬP TỨC
  writeCrashLogToSD(isManual);
  
  // Gọi hàm mô phỏng gửi tin nhắn và gọi điện
  simulateSendSMSAndCall(isManual);
  
  Serial.println("✓ Đã gửi tín hiệu SOS đến server");
  Serial.println("✓ Đã gửi tin nhắn SOS");
  Serial.println("✓ Đã gọi điện đến người thân");
  Serial.println("============================================");
  
  // TODO: Thêm code thực tế để:
  // - Gửi HTTP request đến server
  // - Gửi SMS qua GSM module
  // - Gọi điện qua GSM module
}

// ============================================
// HÀM CHÍNH: PHÁT HIỆN TAI NẠN
// ============================================
CrashState updateCrashDetection(ImuData &out) {
  unsigned long now = millis();
  
  // Kiểm tra thời gian lấy mẫu (200Hz = 5ms)
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) {
    return NO_CRASH;  // Chưa đến lúc lấy mẫu
  }
  
  float dt = (now - lastSampleTime) / 1000.0;  // Đổi sang giây
  lastSampleTime = now;
  
  // 1. Đọc IMU
  float ax_raw, ay_raw, az_raw;
  float gx, gy, gz;
  if (!readAccelGyro(ax_raw, ay_raw, az_raw, gx, gy, gz)) {
    // Lỗi đọc IMU, trả về NO_CRASH để tránh báo sai
    return NO_CRASH;
  }
  
  // 2. Lọc EMA cho accel
  float ax_f, ay_f, az_f;
  applyEMAFilter(ax_raw, ay_raw, az_raw, ax_f, ay_f, az_f);
  
  // 3. Tính G-force tổng từ giá trị đã lọc
  float g_total = sqrt(ax_f * ax_f + ay_f * ay_f + az_f * az_f);
  
  // 4. Cập nhật complementary filter cho roll/pitch
  updateComplementaryFilter(ax_f, ay_f, az_f, gx, gy, gz, dt);
  
  // 5. Tính deltaAngle
  float deltaAngle = calculateDeltaAngle();
  
  // 6. Lưu lịch sử góc
  saveAngleHistory();
  
  // 7. Cập nhật output struct
  out.ax = ax_f;
  out.ay = ay_f;
  out.az = az_f;
  out.gx = gx;
  out.gy = gy;
  out.gz = gz;
  out.g_total = g_total;
  out.roll = roll;
  out.pitch = pitch;
  out.deltaAngle = deltaAngle;
  
  // 8. LOGIC PHÁT HIỆN TAI NẠN
  // Chỉ cần 1 trong 2 điều kiện TRUE → CRASH
  
  // Điều kiện 1: Va chạm mạnh vật lý (dùng biến động từ EEPROM)
  bool crashByGForce = (g_total > gCrashThreshold);
  
  // Điều kiện 2: Ngã / lật / xoay nhanh (dùng biến động từ EEPROM)
  bool crashByAngle = (deltaAngle > angleCrashThreshold);
  
  if (crashByGForce || crashByAngle) {
    return CRASH;
  }
  
  return NO_CRASH;
}

// ============================================
// HÀM CHỈ CẬP NHẬT DỮ LIỆU MPU (KHÔNG PHÁT HIỆN CRASH)
// Dùng khi ở STATE_SOS_SENT, STATE_COUNTDOWN để vẫn cập nhật dữ liệu mới
// Tránh hiện tượng lưu giá trị cũ vượt ngưỡng gây cảnh báo liên tục
// ============================================
void updateMPUDataOnly(ImuData &out) {
  unsigned long now = millis();
  
  // Kiểm tra thời gian lấy mẫu (200Hz = 5ms)
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) {
    return;  // Chưa đến lúc lấy mẫu
  }
  
  float dt = (now - lastSampleTime) / 1000.0;  // Đổi sang giây
  lastSampleTime = now;
  
  // 1. Đọc IMU
  float ax_raw, ay_raw, az_raw;
  float gx, gy, gz;
  if (!readAccelGyro(ax_raw, ay_raw, az_raw, gx, gy, gz)) {
    // Lỗi đọc IMU, không cập nhật
    return;
  }
  
  // 2. Lọc EMA cho accel
  float ax_f, ay_f, az_f;
  applyEMAFilter(ax_raw, ay_raw, az_raw, ax_f, ay_f, az_f);
  
  // 3. Tính G-force tổng từ giá trị đã lọc
  float g_total = sqrt(ax_f * ax_f + ay_f * ay_f + az_f * az_f);
  
  // 4. Cập nhật complementary filter cho roll/pitch
  updateComplementaryFilter(ax_f, ay_f, az_f, gx, gy, gz, dt);
  
  // 5. Tính deltaAngle
  float deltaAngle = calculateDeltaAngle();
  
  // 6. Lưu lịch sử góc
  saveAngleHistory();
  
  // 7. Cập nhật output struct (KHÔNG kiểm tra crash)
  out.ax = ax_f;
  out.ay = ay_f;
  out.az = az_f;
  out.gx = gx;
  out.gy = gy;
  out.gz = gz;
  out.g_total = g_total;
  out.roll = roll;
  out.pitch = pitch;
  out.deltaAngle = deltaAngle;
}

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("============================================");
  Serial.println("CRASH DETECTION SYSTEM - MPU9250");
  Serial.println("============================================");
  
  // Khởi tạo EEPROM và đọc dữ liệu
  EEPROM.begin(EEPROM_SIZE);
  loadFromEEPROM();
  
  // Đảm bảo systemState luôn là STATE_NORMAL khi khởi động
  systemState = STATE_NORMAL;
  
  // Khởi tạo OLED SSD1306
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 30, "Initializing...");
  u8g2.sendBuffer();
  delay(500);
  
  if (!initMPU9250()) {
    Serial.println("ERROR: Cannot initialize MPU9250!");
    while(1) delay(1000);
  }
  
  // Khởi tạo lịch sử góc
  memset(angleHistory, 0, sizeof(angleHistory));
  historyIndex = 0;
  historyCount = 0;
  
  // Khởi tạo nút hủy và nút SOS thủ công
  pinMode(BUTTON_CANCEL_PIN, INPUT_PULLUP);
  pinMode(BUTTON_MANUAL_SOS_PIN, INPUT_PULLUP);
  
  // ============================================
  // KHỞI TẠO WiFi WEB SERVER (SAU NÚT NHẤN)
  // ============================================
  Serial.println("\n============================================");
  Serial.println("🌐 TỰ ĐỘNG MỞ WEB SETTING MODE");
  Serial.println("============================================");
  Serial.println("Web Setting sẽ tự động mở khi khởi động");
  Serial.println("Giữ nút 36 trong 3 giây để xác nhận và thoát");
  Serial.println("Hoặc đợi 3 phút để tự động vào phần chính");
  Serial.println("============================================\n");
  
  // Tự động kích hoạt Web Setting
  webSettingActive = true;
  systemState = STATE_WEB_SETTING;
  webSettingStartTime = millis();
  initWebServer();
  
  // Khởi tạo buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  buzzerState = false;
  lastBeepTime = 0;
  
  // Khởi tạo GPS NEO-8M với HardwareSerial và TinyGPS++
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
  // Khởi tạo dữ liệu GPS
  gpsData.valid = false;
  gpsData.latitude = 0;
  gpsData.longitude = 0;
  gpsData.altitude = 0;
  gpsData.speed = 0;
  gpsData.satellites = 0;
  gpsData.hdop = 99.9;
  gpsData.time = "00:00:00";
  gpsData.date = "01/01/00";
  gpsData.course = 0;
  lastGPSUpdate = 0;
  
  // Khởi tạo biến tính vận tốc từ GPS
  lastGPSLat = 0;
  lastGPSLon = 0;
  lastGPSTime = 0;
  hasLastGPSPoint = false;
  
  Serial.println("GPS NEO-8M initialized với HardwareSerial + TinyGPS++");
  Serial.print("  RX pin: ");
  Serial.print(GPS_RX_PIN);
  Serial.print(" (nhận từ TX của GPS)");
  Serial.print(", TX pin: ");
  Serial.print(GPS_TX_PIN);
  Serial.println(" (gửi đến RX của GPS)");
  Serial.println("Đang chờ tín hiệu GPS...");
  Serial.println("(Cần thời gian để GPS lock vệ tinh - có thể mất vài phút)");
  
  // Khởi tạo DS1307 RTC
  Serial.println("\nInitializing DS1307 RTC...");
  if (initDS1307()) {
    ds1307Initialized = true;
    readDS1307Time();  // Đọc thời gian lần đầu
    if (rtcData.valid) {
      Serial.print("  Thời gian hiện tại: ");
      Serial.println(getRTCDateTimeString());
    }
  } else {
    ds1307Initialized = false;
    Serial.println("  ⚠️ DS1307 không khả dụng, sẽ không có thời gian RTC");
  }
  
  // Khởi tạo DHT11
  Serial.println("\nInitializing DHT11...");
  dht.begin();
  dhtData.valid = false;
  dhtData.temperature = 0;
  dhtData.humidity = 0;
  lastDHTRead = 0;
  delay(2000);  // DHT11 cần thời gian khởi động
  updateDHT();  // Đọc lần đầu
  if (dhtData.valid) {
    Serial.print("  ✓ DHT11 initialized - Temp: ");
    Serial.print(dhtData.temperature, 1);
    Serial.print("°C, Humidity: ");
    Serial.print(dhtData.humidity, 1);
    Serial.println("%");
  } else {
    Serial.println("  ⚠️ DHT11 không khả dụng, kiểm tra kết nối");
  }
  
  // Khởi tạo SIM 4G
  Serial.println("\nInitializing SIM 4G module...");
  initSIMModule();
  if (sim4gInitialized) {
    Serial.println("  ✓ SIM 4G initialized successfully");
  } else {
    Serial.println("  ⚠️ SIM 4G không khả dụng, kiểm tra kết nối và SIM card");
  }
  
  // Khởi tạo SD Card
  Serial.println("\nInitializing SD card...");
  if (initSDCard()) {
    sdCardInitialized = true;
    Serial.println("✓ SD card initialized successfully!");
    // Ghi header vào file log
    File logFile = SD.open(LOG_FILE, FILE_WRITE);
    if (logFile) {
      logFile.println("Time,Ax,Ay,Az,G_Total,Roll,Pitch,DeltaAngle,Lat,Lon,Alt,Speed,Satellites,Temperature,Humidity,Status");
      logFile.close();
    }
    // Ghi header vào file crash log
    File crashFile = SD.open(CRASH_LOG_FILE, FILE_WRITE);
    if (crashFile) {
      crashFile.println("Time,Type,Ax,Ay,Az,G_Total,Roll,Pitch,DeltaAngle,Lat,Lon,Alt,Speed,Satellites,GoogleMapsLink,Temperature,Humidity");
      crashFile.close();
    }
    Serial.println("✓ SD card logging ready!");
  } else {
    sdCardInitialized = false;
    Serial.println("NO SD CARD");
  }
  
  lastSampleTime = millis();
  lastPrintTime = millis();
  lastLogTime = millis();
  
  isManualSOS = false;  // Khởi tạo flag
  
  Serial.println("System ready!");
  Serial.println("Format: Ax Ay Az | G_total | Roll Pitch | DeltaAngle | Status");
  Serial.println("Nút 36: Ấn chậm (>=1s) để hủy SOS trong 30 giây");
  Serial.println("Nút 37: Ấn chậm (>=1s) để gửi SOS thủ công (mức độ trung bình)");
  Serial.println("============================================");
}

// ============================================
// LOOP
// ============================================
void loop() {
  unsigned long now = millis();
  
  // Đọc trạng thái nút 36 (dùng chung cho tất cả các phần code)
  int button36State = readButton36();
  
  // ============================================
  // XỬ LÝ WEB SETTING MODE
  // ============================================
  if (webSettingActive || systemState == STATE_WEB_SETTING) {
    // Đang ở chế độ Web Setting - CHỈ PHỤC VỤ WEB SERVER
    // TẤT CẢ CÔNG VIỆC KHÁC ĐỀU DỪNG LẠI
    
    // Kiểm tra và đảm bảo WiFi AP vẫn hoạt động (kiểm tra mỗi 5 giây)
    static unsigned long lastWiFiCheck = 0;
    if (now - lastWiFiCheck >= 5000) {
      lastWiFiCheck = now;
      IPAddress currentIP = WiFi.softAPIP();
      if (currentIP.toString() == "0.0.0.0" || WiFi.softAPgetStationNum() < 0) {
        // WiFi AP bị mất - Khởi động lại
        Serial.println("⚠️ WiFi AP bị mất - Đang khởi động lại...");
        WiFi.softAPdisconnect(false);
        delay(100);
        bool apResult = WiFi.softAP(AP_SSID, AP_PASSWORD);
        if (apResult) {
          Serial.println("✓ WiFi AP đã được khởi động lại thành công");
        } else {
          Serial.println("✗ Không thể khởi động lại WiFi AP");
        }
      }
    }
    
    // Xử lý web server (ưu tiên cao nhất) - Gọi nhiều lần để đảm bảo phản hồi nhanh
    for (int i = 0; i < 10; i++) {
      server.handleClient();
      delay(1);  // Delay nhỏ để cho phép xử lý
    }
    
    // Kiểm tra timeout 3 phút - Tự động thoát nếu không có tác động
    if (now - webSettingStartTime >= WEB_SETTING_TIMEOUT_MS) {
      // Đã hết 3 phút - Tự động lưu và thoát
      saveToEEPROM();
      stopWebServer();
      webSettingActive = false;
      systemState = STATE_NORMAL;
      webSettingExitTime = now;
      justExitedWebSetting = true;
      Serial.println("✓ Timeout 3 phút - Tự động lưu và thoát Web Setting mode");
      return;
    }
    
    // Kiểm tra nút 3 giây để lưu và thoát (kiểm tra trực tiếp thời gian giữ nút)
    static unsigned long button36PressStartTime = 0;
    static bool button36WasPressed = false;
    bool button36Pressed = (digitalRead(BUTTON_CANCEL_PIN) == LOW);
    
    if (button36Pressed && !button36WasPressed) {
      // Nút vừa được nhấn
      button36PressStartTime = now;
      button36WasPressed = true;
    } else if (!button36Pressed && button36WasPressed) {
      // Nút vừa được thả
      button36WasPressed = false;
    } else if (button36Pressed && button36WasPressed) {
      // Đang giữ nút - Kiểm tra thời gian
      unsigned long holdDuration = now - button36PressStartTime;
      if (holdDuration >= WEB_SETTING_BUTTON_HOLD_MS) {
        // Đã giữ >= 3 giây - Lưu và thoát
        saveToEEPROM();
        stopWebServer();
        webSettingActive = false;
        systemState = STATE_NORMAL;
        webSettingExitTime = now;
        justExitedWebSetting = true;
        button36WasPressed = false;  // Reset
        Serial.println("✓ Đã lưu và thoát Web Setting mode (giữ nút 3s)");
        return;
      }
    }
    
    // Cập nhật màn hình (chỉ mỗi 1000ms để không làm chậm web)
    static unsigned long lastDisplayUpdate = 0;
    if (now - lastDisplayUpdate >= 1000) {
      updateDisplay();
      lastDisplayUpdate = now;
    }
    
    // KHÔNG chạy bất kỳ hàm nào khác (GPS, DHT, Crash Detection, v.v.)
    // CHỈ phục vụ web server
    return;
  }
  
  // Cập nhật GPS liên tục
  updateGPS();
  
  // Cập nhật DHT11 liên tục
  updateDHT();
  
  // Cập nhật phát hiện tai nạn (chỉ khi không trong trạng thái SOS và cho phép cảnh báo)
  // Bỏ qua crash detection trong 3 giây sau khi hủy SOS để tránh trigger lại ngay
  // LUÔN đọc MPU để cập nhật dữ liệu mới, tránh lưu giá trị cũ vượt ngưỡng
  CrashState crashState = NO_CRASH;
  bool inSOSCooldown = (sosCancelTime > 0 && (now - sosCancelTime) < SOS_CANCEL_COOLDOWN_MS);
  
  if (systemState == STATE_NORMAL && allowSOSSent && !inSOSCooldown) {
    // Phát hiện crash và cập nhật dữ liệu MPU
    crashState = updateCrashDetection(imuData);
  } else {
    // Chỉ cập nhật dữ liệu MPU mà không phát hiện crash
    // Đảm bảo dữ liệu luôn mới, tránh giá trị cũ vượt ngưỡng
    updateMPUDataOnly(imuData);
  }
  
  // ============================================
  // XỬ LÝ TRẠNG THÁI HỆ THỐNG
  // ============================================
  // Khai báo biến trước switch để tránh lỗi "jump to case label"
  int manualSOSAction = 0;
  
  switch (systemState) {
    case STATE_NORMAL:
      {
        // Bình thường, kiểm tra phát hiện CRASH (chỉ khi cho phép cảnh báo)
        if (allowSOSSent && crashState == CRASH) {
          systemState = STATE_CRASH_DETECTED;
          crashDetectedTime = now;
          countdownStartTime = now;
          remainingSeconds = countdownTime;
          isManualSOS = false;  // SOS tự động
          Serial.println("============================================");
          Serial.println("*** CRASH DETECTED - COUNTDOWN STARTED ***");
          Serial.println("Ấn nút 36 (3-10s) để hủy SOS");
          Serial.println("============================================");
          String severity = determineCrashSeverity(false, imuData.g_total);
          updateEventState("CRASH", severity, true);
        }
        
        // Kiểm tra nút 37 - SOS thủ công (chỉ khi cho phép cảnh báo và không trong cooldown)
        if (allowSOSSent && !inSOSCooldown) {
          manualSOSAction = readButtonManualSOS();
          if (manualSOSAction == 1) {
            // Đã ấn chậm nút 37 - Bắt đầu đếm ngược SOS thủ công
            systemState = STATE_CRASH_DETECTED;
            crashDetectedTime = now;
            countdownStartTime = now;
            remainingSeconds = countdownTime;
            isManualSOS = true;  // SOS thủ công
            Serial.println("============================================");
            Serial.println("*** SOS THỦ CÔNG - COUNTDOWN STARTED ***");
            Serial.println("Người dùng nhấn nút 37 (ấn chậm >=1s)");
            Serial.println("Ấn nút 36 (>=3s) để hủy SOS");
            Serial.println("============================================");
            String severity = determineCrashSeverity(true, imuData.g_total);
            updateEventState("CRASH", severity, true);
          }
        }
        
        // Kiểm tra nút 36 (bỏ qua nếu vừa thoát Web Setting trong 2 giây)
        if (justExitedWebSetting && (now - webSettingExitTime < 2000)) {
          // Bỏ qua xử lý nút trong 2 giây sau khi thoát Web Setting
          if (button36State == 0) {
            // Nếu nút đã được thả, reset flag
            justExitedWebSetting = false;
          }
        } else {
          // Xử lý nút bình thường
          justExitedWebSetting = false;  // Reset flag
          
          if (button36State == 1) {
            // Đã ấn nhanh nút 36 (< 3s) - Chuyển màn hình
            nextScreen();
          }
          // Đã xóa logic kích hoạt Web Setting từ màn hình 5
        }
      }
      break;
      
    case STATE_CRASH_DETECTED:
      // Vừa phát hiện CRASH, chuyển sang đếm ngược
      systemState = STATE_COUNTDOWN;
      break;
      
    case STATE_COUNTDOWN:
      {
        // Đang đếm ngược (dùng biến động từ EEPROM)
        unsigned long elapsed = now - countdownStartTime;
        int newRemaining = countdownTime - (elapsed / 1000);
        
        // Cập nhật và in thời gian còn lại mỗi giây
        if (newRemaining != remainingSeconds && newRemaining >= 0) {
          remainingSeconds = newRemaining;
          if (remainingSeconds > 0) {
            Serial.print("⏰ SOS ");
            if (isManualSOS) {
              Serial.print("(THỦ CÔNG) ");
            } else {
              Serial.print("(TỰ ĐỘNG) ");
            }
            Serial.print("trong: ");
            Serial.print(remainingSeconds);
            Serial.println(" giây...");
          }
        }
        
        // Kiểm tra nút 36 (bỏ qua nếu vừa thoát Web Setting trong 2 giây)
        if (justExitedWebSetting && (now - webSettingExitTime < 2000)) {
          // Bỏ qua xử lý nút trong 2 giây sau khi thoát Web Setting
          if (button36State == 0) {
            // Nếu nút đã được thả, reset flag
            justExitedWebSetting = false;
          }
        } else {
          // Xử lý nút bình thường
          justExitedWebSetting = false;  // Reset flag
          
          if (button36State == 2) {
            // Đã ấn chậm nút 36 (>= 3s) - Hủy SOS
            systemState = STATE_NORMAL;
            isManualSOS = false;  // Reset flag
            sosCancelTime = now;   // Ghi nhận thời gian hủy SOS (để cooldown)
            // Dừng buzzer
            buzzerState = false;
            digitalWrite(BUZZER_PIN, LOW);
            Serial.println("============================================");
            Serial.println("✓ SOS ĐÃ ĐƯỢC HỦY");
            Serial.println("Hệ thống quay về trạng thái bình thường");
            Serial.println("Cooldown 3 giây để tránh trigger lại");
            Serial.println("============================================");
            updateEventState("NORMAL", "LOW", true);
            break;
          } else if (button36State == 1) {
            // Đã ấn nhanh nút 36 (< 3s) - Chuyển màn hình
            nextScreen();
          }
        }
        
        // Kiểm tra hết thời gian đếm ngược
        if (elapsed >= (countdownTime * 1000)) {
          // Hết 30 giây, gửi SOS (tự động hoặc thủ công)
          systemState = STATE_SOS_SENT;
          // Dừng buzzer
          buzzerState = false;
          digitalWrite(BUZZER_PIN, LOW);
          sendSOS(isManualSOS);  // Truyền flag để phân biệt
        }
      }
      break;
      
    case STATE_SOS_SENT:
      // Đã gửi SOS - Giữ nguyên trạng thái cho đến khi khởi động lại thiết bị
      // KHÔNG tự động reset về NORMAL
      
      // Vẫn cho phép chuyển màn hình khi ở STATE_SOS_SENT (bỏ qua nếu vừa thoát Web Setting)
      if (!(justExitedWebSetting && (now - webSettingExitTime < 2000))) {
        if (button36State == 1) {
          // Đã ấn nhanh nút 36 (< 3s) - Chuyển màn hình
          nextScreen();
        }
      }
      break;
  }

  // ============================================
  // GỬI TELEMETRY VÀ KIỂM TRA SOS STATUS
  // ============================================
  bool shouldSendTelemetry = telemetryPendingImmediateSend ||
    (now - lastTelemetrySendTime >= TELEMETRY_INTERVAL_MS);
  if (shouldSendTelemetry) {
    sendTelemetryToServer(currentEventType, currentSeverity, telemetryPendingImmediateSend);
  }

  if (now - lastSOSStatusCheckTime >= SOS_STATUS_CHECK_INTERVAL_MS) {
    lastSOSStatusCheckTime = now;
    checkSOSStatusFromServer();
  }
  
  // ============================================
  // CẬP NHẬT BUZZER
  // ============================================
  updateBuzzer();
  
  // ============================================
  // CẬP NHẬT MÀN HÌNH OLED
  // ============================================
  updateDisplay();
  
  // ============================================
  // GHI LOG ĐỊNH KỲ VÀO SD CARD (1 phút/lần)
  // ============================================
  if (now - lastLogTime >= LOG_INTERVAL_MS) {
    lastLogTime = now;
    writeLogToSD();
  }
  
  // ============================================
  // IN DEBUG DỮ LIỆU
  // ============================================
  if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
    lastPrintTime = now;
    
    // In dữ liệu
    Serial.print("A: ");
    Serial.print(imuData.ax, 2);
    Serial.print(" ");
    Serial.print(imuData.ay, 2);
    Serial.print(" ");
    Serial.print(imuData.az, 2);
    Serial.print(" | G: ");
    Serial.print(imuData.g_total, 2);
    Serial.print(" | R: ");
    Serial.print(imuData.roll, 1);
    Serial.print(" P: ");
    Serial.print(imuData.pitch, 1);
    Serial.print(" | Δ: ");
    Serial.print(imuData.deltaAngle, 1);
    Serial.print(" | ");
    
    // In trạng thái
    switch (systemState) {
      case STATE_NORMAL:
        if (crashState == CRASH) {
          Serial.println("CRASH");
        } else {
          Serial.println("NO_CRASH");
        }
        break;
      case STATE_CRASH_DETECTED:
        Serial.println("CRASH_DETECTED");
        break;
      case STATE_COUNTDOWN:
        Serial.print("COUNTDOWN (");
        Serial.print(remainingSeconds);
        Serial.println("s)");
        break;
      case STATE_SOS_SENT:
        Serial.println("SOS_SENT");
        break;
    }
  }
  
  // Không dùng delay(), chỉ dùng millis()
}