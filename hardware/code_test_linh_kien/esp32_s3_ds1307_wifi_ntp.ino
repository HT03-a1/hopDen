/*
 * ESP32-S3 - CẬP NHẬT DS1307 TỪ WIFI + NTP
 * 
 * Mục đích: Kết nối WiFi, lấy thời gian từ NTP server và cập nhật vào DS1307 RTC
 * 
 * Chức năng:
 * 1. Kết nối WiFi
 * 2. Lấy thời gian từ NTP server (pool.ntp.org)
 * 3. Cập nhật thời gian vào DS1307 RTC
 * 4. Hiển thị thời gian từ DS1307 liên tục
 * 5. Tự động đồng bộ lại thời gian định kỳ (mỗi 24 giờ)
 * 
 * Sơ đồ kết nối:
 * - I2C SDA: GPIO 8
 * - I2C SCL: GPIO 9
 * - DS1307: I2C address 0x68
 * - OLED (tùy chọn): I2C address 0x3C
 * 
 * THƯ VIỆN CẦN CÀI ĐẶT (Arduino Library Manager):
 * 1. RTClib (by Adafruit) - https://github.com/adafruit/RTClib
 * 2. U8g2 (by olikraus) - https://github.com/olikraus/u8g2 (tùy chọn, nếu dùng OLED)
 * 
 * CẤU HÌNH:
 * 1. Thay đổi YOUR_WIFI_SSID và YOUR_WIFI_PASSWORD ở dòng 30-31
 * 2. Nếu cần thay đổi múi giờ, sửa gmtOffset_sec (dòng 37)
 *    - GMT+7 (Việt Nam): 7 * 3600
 *    - GMT+8 (Trung Quốc): 8 * 3600
 *    - GMT+0 (London): 0
 */

#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <time.h>
#include <U8g2lib.h>

// ============================================
// CẤU HÌNH WIFI
// ============================================

const char* ssid = "YOUR_WIFI_SSID";        // Thay đổi tên WiFi của bạn
const char* password = "YOUR_WIFI_PASSWORD"; // Thay đổi mật khẩu WiFi của bạn

// ============================================
// CẤU HÌNH NTP
// ============================================

// NTP Server (pool.ntp.org hoặc time.nist.gov)
const char* ntpServer = "pool.ntp.org";
// Múi giờ (GMT+7 cho Việt Nam)
const long gmtOffset_sec = 7 * 3600;
// Điều chỉnh giờ mùa hè (DST) - Việt Nam không có DST
const int daylightOffset_sec = 0;

// ============================================
// CẤU HÌNH CHÂN GPIO
// ============================================

// I2C Pins (theo sơ đồ mới)
#define I2C_SDA_PIN     8
#define I2C_SCL_PIN     9

// OLED (tùy chọn - để hiển thị thông tin)
#define OLED_ADDRESS    0x3C
// Khai báo OLED - nếu lỗi, thử: U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0);

// ============================================
// BIẾN TOÀN CỤC
// ============================================

RTC_DS1307 rtc;

// Thời gian đồng bộ cuối cùng
unsigned long lastSyncTime = 0;
// Khoảng thời gian đồng bộ lại (24 giờ = 86400000 ms)
const unsigned long syncInterval = 24 * 60 * 60 * 1000;

// Trạng thái WiFi
bool wifiConnected = false;
bool rtcInitialized = false;
bool oledAvailable = false;

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== ESP32-S3 DS1307 WIFI NTP SYNC ===");
  
  // Khởi tạo I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("I2C initialized (SDA: GPIO 8, SCL: GPIO 9)");
  
  // Khởi tạo DS1307
  if (!rtc.begin()) {
    Serial.println("❌ ERROR: DS1307 RTC not found!");
    Serial.println("Kiểm tra kết nối I2C (SDA: GPIO 8, SCL: GPIO 9)");
    while (1) {
      delay(1000);
      Serial.print(".");
    }
  }
  
  Serial.println("✔ DS1307 RTC found!");
  rtcInitialized = true;
  
  // Hiển thị thời gian hiện tại từ RTC (nếu có)
  if (rtc.isrunning()) {
    DateTime now = rtc.now();
    Serial.println("\n=== THỜI GIAN HIỆN TẠI TỪ RTC ===");
    printDateTime(now);
    Serial.println("\n================================");
  } else {
    Serial.println("⚠️ RTC is NOT running - will sync from NTP");
  }
  
  // Khởi tạo OLED (nếu có)
  Wire.beginTransmission(OLED_ADDRESS);
  if (Wire.endTransmission() == 0) {
    oled.begin();
    oled.setFont(u8g2_font_ncenB08_tr);
    oled.clearBuffer();
    oled.drawStr(0, 20, "DS1307 NTP Sync");
    oled.drawStr(0, 40, "Connecting WiFi...");
    oled.sendBuffer();
    oledAvailable = true;
    Serial.println("OLED initialized");
  } else {
    Serial.println("OLED not found (optional)");
    oledAvailable = false;
  }
  
  // Kết nối WiFi
  connectWiFi();
  
  // Cấu hình NTP
  if (wifiConnected) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("NTP configured");
    
    // Đợi lấy thời gian từ NTP
    Serial.println("Waiting for NTP time...");
    updateRTCFromNTP();
    
    lastSyncTime = millis();
  } else {
    Serial.println("⚠️ WiFi not connected - using RTC time only");
    if (!rtc.isrunning()) {
      Serial.println("❌ ERROR: RTC not running and no WiFi!");
      Serial.println("Please connect WiFi or set RTC manually");
    }
  }
  
  Serial.println("\n=== BẮT ĐẦU ĐỌC THỜI GIAN ===");
  Serial.println("Format: YYYY-MM-DD HH:MM:SS (Day of week)");
  Serial.println("----------------------------------------\n");
}

// ============================================
// LOOP
// ============================================

void loop() {
  // Kiểm tra và đồng bộ lại thời gian định kỳ
  if (wifiConnected && (millis() - lastSyncTime > syncInterval)) {
    Serial.println("\n=== ĐỒNG BỘ LẠI THỜI GIAN TỪ NTP ===");
    updateRTCFromNTP();
    lastSyncTime = millis();
  }
  
  // Đọc và hiển thị thời gian từ DS1307
  if (rtcInitialized && rtc.isrunning()) {
    DateTime now = rtc.now();
    
    // Kiểm tra tính hợp lệ
    if (now.year() >= 2000 && now.year() <= 2100) {
      // Hiển thị trên Serial
      printDateTime(now);
      Serial.print("  | Sync: ");
      Serial.print((millis() - lastSyncTime) / 1000);
      Serial.println("s ago");
      
      // Hiển thị trên OLED (nếu có)
      if (oledAvailable) {
        oled.clearBuffer();
        oled.setFont(u8g2_font_ncenB08_tr);
        
        char dateStr[32];
        snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d",
                 now.year(), now.month(), now.day());
        oled.drawStr(0, 15, dateStr);
        
        char timeStr[32];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
                 now.hour(), now.minute(), now.second());
        oled.drawStr(0, 35, timeStr);
        
        const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        char dayStr[16];
        snprintf(dayStr, sizeof(dayStr), "%s", days[now.dayOfTheWeek()]);
        oled.drawStr(0, 55, dayStr);
        
        if (wifiConnected) {
          oled.drawStr(80, 55, "WiFi");
        } else {
          oled.drawStr(80, 55, "RTC");
        }
        
        oled.sendBuffer();
      }
    } else {
      Serial.println("❌ ERROR: Invalid time from RTC!");
    }
  } else {
    Serial.println("❌ ERROR: RTC not running!");
  }
  
  delay(1000);  // Cập nhật mỗi 1 giây
}

// ============================================
// KẾT NỐI WIFI
// ============================================

void connectWiFi() {
  Serial.print("\nConnecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
    
      // Cập nhật OLED
      if (oledAvailable) {
        oled.clearBuffer();
        oled.setFont(u8g2_font_ncenB08_tr);
        oled.drawStr(0, 20, "Connecting WiFi");
        char dotStr[16] = "";
        int dotCount = (attempts % 4) + 1;
        for (int i = 0; i < dotCount; i++) {
          strcat(dotStr, ".");
        }
        oled.drawStr(0, 40, dotStr);
        oled.sendBuffer();
      }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n✔ WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    // Cập nhật OLED
    if (oledAvailable) {
      oled.clearBuffer();
      oled.setFont(u8g2_font_ncenB08_tr);
      oled.drawStr(0, 20, "WiFi Connected");
      IPAddress ip = WiFi.localIP();
      char ipStr[32];
      snprintf(ipStr, sizeof(ipStr), "IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
      oled.drawStr(0, 40, ipStr);
      oled.sendBuffer();
      delay(2000);
    }
  } else {
    wifiConnected = false;
    Serial.println("\n❌ WiFi connection failed!");
    Serial.println("Please check SSID and password");
    
    // Cập nhật OLED
    if (oledAvailable) {
      oled.clearBuffer();
      oled.setFont(u8g2_font_ncenB08_tr);
      oled.drawStr(0, 20, "WiFi Failed");
      oled.drawStr(0, 40, "Check config");
      oled.sendBuffer();
    }
  }
}

// ============================================
// CẬP NHẬT RTC TỪ NTP
// ============================================

void updateRTCFromNTP() {
  if (!wifiConnected) {
    Serial.println("❌ WiFi not connected - cannot sync NTP");
    return;
  }
  
  Serial.println("Getting time from NTP server...");
  
  struct tm timeinfo;
  
  // Đợi lấy thời gian từ NTP (tối đa 10 lần thử)
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 10) {
    Serial.print(".");
    delay(1000);
    attempts++;
  }
  
  if (attempts >= 10) {
    Serial.println("\n❌ Failed to get time from NTP server");
    return;
  }
  
  // Chuyển đổi struct tm sang DateTime và cập nhật RTC
  DateTime ntpTime(
    timeinfo.tm_year + 1900,  // Năm (tm_year là năm - 1900)
    timeinfo.tm_mon + 1,      // Tháng (tm_mon bắt đầu từ 0)
    timeinfo.tm_mday,         // Ngày
    timeinfo.tm_hour,         // Giờ
    timeinfo.tm_min,          // Phút
    timeinfo.tm_sec            // Giây
  );
  
  // Cập nhật RTC
  rtc.adjust(ntpTime);
  
  Serial.println("\n✔ RTC updated from NTP!");
  Serial.print("NTP time: ");
  printDateTime(ntpTime);
  Serial.println();
  
  // Xác nhận bằng cách đọc lại từ RTC
  DateTime rtcTime = rtc.now();
  Serial.print("RTC time: ");
  printDateTime(rtcTime);
  Serial.println();
  
  // Cập nhật OLED
  if (oledAvailable) {
    oled.clearBuffer();
    oled.setFont(u8g2_font_ncenB08_tr);
    oled.drawStr(0, 20, "NTP Sync OK");
    oled.drawStr(0, 40, "Time updated");
    oled.sendBuffer();
    delay(2000);
  }
}

// ============================================
// HÀM TIỆN ÍCH
// ============================================

void printDateTime(DateTime dt) {
  // Format: YYYY-MM-DD HH:MM:SS (Day of week)
  char dateStr[32];
  snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d %02d:%02d:%02d",
           dt.year(), dt.month(), dt.day(),
           dt.hour(), dt.minute(), dt.second());
  
  Serial.print(dateStr);
  
  // Day of week
  const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  Serial.print(" (");
  Serial.print(days[dt.dayOfTheWeek()]);
  Serial.print(")");
}

// ============================================
// HÀM ĐẶT THỜI GIAN THỦ CÔNG (nếu cần)
// ============================================

void setTimeManually(int year, int month, int day, int hour, int minute, int second) {
  rtc.adjust(DateTime(year, month, day, hour, minute, second));
  Serial.print("Time set manually to: ");
  printDateTime(rtc.now());
  Serial.println();
}

