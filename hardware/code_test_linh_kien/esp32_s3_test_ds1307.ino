/*
 * ESP32-S3 TEST DS1307 RTC MODULE
 * 
 * Mục đích: Test module thời gian thực DS1307
 * 
 * Chức năng:
 * 1. Khởi tạo DS1307 qua I2C
 * 2. Đặt thời gian (nếu cần)
 * 3. Đọc và hiển thị thời gian liên tục
 * 4. Kiểm tra pin backup (battery)
 * 5. Kiểm tra xem RTC có đang chạy không
 */

#include <Wire.h>
#include <RTClib.h>

// ============================================
// CẤU HÌNH
// ============================================

// I2C Pins (theo sơ đồ mới)
#define I2C_SDA_PIN     8
#define I2C_SCL_PIN     9

// DS1307 I2C Address: 0x68

// ============================================
// BIẾN TOÀN CỤC
// ============================================

RTC_DS1307 rtc;

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== ESP32-S3 TEST DS1307 RTC MODULE ===");
  
  // Khởi tạo I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("I2C initialized");
  
  // Khởi tạo DS1307
  if (!rtc.begin()) {
    Serial.println("❌ ERROR: DS1307 RTC not found!");
    Serial.println("Kiểm tra:");
    Serial.println("  - Kết nối I2C (SDA, SCL)");
    Serial.println("  - Địa chỉ I2C: 0x68");
    Serial.println("  - Nguồn cấp cho DS1307");
    while (1) delay(1000);  // Dừng nếu không tìm thấy
  }
  
  Serial.println("✔ DS1307 RTC found!");
  
  // Kiểm tra xem RTC có đang chạy không
  if (!rtc.isrunning()) {
    Serial.println("⚠️ WARNING: RTC is NOT running!");
    Serial.println("Setting time from compile time...");
    
    // Đặt thời gian từ thời gian compile
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("Time set from compile time");
  } else {
    Serial.println("✔ RTC is running");
  }
  
  // Hiển thị thời gian hiện tại
  DateTime now = rtc.now();
  Serial.println("\n=== CURRENT TIME ===");
  printDateTime(now);
  Serial.println("===================\n");
  
  Serial.println("Reading time every second...");
  Serial.println("Format: YYYY-MM-DD HH:MM:SS (Day of week)");
  Serial.println("----------------------------------------\n");
}

// ============================================
// LOOP
// ============================================

void loop() {
  // Đọc thời gian từ DS1307
  DateTime now = rtc.now();
  
  // Kiểm tra tính hợp lệ
  if (now.year() < 2000 || now.year() > 2100) {
    Serial.println("❌ ERROR: Invalid time from RTC!");
    delay(1000);
    return;
  }
  
  // Hiển thị thời gian
  printDateTime(now);
  
  // Hiển thị thêm thông tin
  Serial.print("  | Unix timestamp: ");
  Serial.print(now.unixtime());
  Serial.print(" | Day of year: ");
  Serial.println(now.dayOfTheYear());
  
  delay(1000);  // Đọc mỗi 1 giây
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

void setTimeManually() {
  // Ví dụ: Đặt thời gian 2025-01-13 14:30:00
  // Format: DateTime(year, month, day, hour, minute, second)
  rtc.adjust(DateTime(2025, 1, 13, 14, 30, 0));
  Serial.println("Time set manually to: 2025-01-13 14:30:00");
}

// ============================================
// HÀM KIỂM TRA PIN BACKUP
// ============================================

void checkBattery() {
  // DS1307 có pin backup (CR2032) để duy trì thời gian khi mất điện
  // Không có hàm trực tiếp để kiểm tra pin, nhưng có thể kiểm tra:
  // - Nếu RTC không chạy sau khi mất điện → pin hết hoặc không có pin
  // - Nếu RTC vẫn chạy sau khi mất điện → pin còn tốt
  
  Serial.println("\n=== BATTERY CHECK ===");
  Serial.println("DS1307 uses CR2032 battery for backup");
  Serial.println("To check battery:");
  Serial.println("  1. Note current time");
  Serial.println("  2. Disconnect power");
  Serial.println("  3. Wait a few minutes");
  Serial.println("  4. Reconnect power");
  Serial.println("  5. Check if time is still correct");
  Serial.println("If time is wrong → battery may be dead");
  Serial.println("===================\n");
}

