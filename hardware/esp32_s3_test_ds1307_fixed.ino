/*
 * ESP32-S3 TEST DS1307 RTC - FIXED VERSION
 * 
 * Sửa code mẫu của thư viện RTClib để dùng chân I2C tùy chỉnh
 * 
 * VẤN ĐỀ: Code mẫu dùng Wire.begin() mặc định (GPIO 21, 22)
 * GIẢI PHÁP: Phải gọi Wire.begin(SDA, SCL) với chân đúng trước rtc.begin()
 */

#include <Wire.h>
#include <RTClib.h>

// ============================================
// CẤU HÌNH CHÂN I2C (THEO SƠ ĐỒ MỚI)
// ============================================

#define I2C_SDA_PIN     8
#define I2C_SCL_PIN     9

// ============================================
// BIẾN TOÀN CỤC
// ============================================

RTC_DS1307 rtc;

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  delay(2000);  // Đợi Serial Monitor
  
  Serial.println("\n\n=== ESP32-S3 TEST DS1307 RTC (FIXED) ===");
  Serial.println("==========================================\n");
  
  // QUAN TRỌNG: Khởi tạo I2C với chân tùy chỉnh TRƯỚC KHI khởi tạo RTC
  Serial.print("Initializing I2C with custom pins... ");
  Serial.print("SDA: GPIO ");
  Serial.print(I2C_SDA_PIN);
  Serial.print(", SCL: GPIO ");
  Serial.println(I2C_SCL_PIN);
  
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);  // ← QUAN TRỌNG: Phải có dòng này!
  delay(100);  // Đợi I2C ổn định
  
  Serial.println("✓ I2C initialized\n");
  
  // Bây giờ mới khởi tạo DS1307
  Serial.print("Initializing DS1307... ");
  
  if (!rtc.begin()) {
    Serial.println("❌ ERROR: DS1307 RTC not found!");
    Serial.println("\nKiểm tra:");
    Serial.println("  - I2C connections (SDA: GPIO 8, SCL: GPIO 9)");
    Serial.println("  - I2C address: 0x68");
    Serial.println("  - Power supply (3.3V)");
    Serial.println("  - Pull-up resistors (4.7kΩ)");
    Serial.println("\nLưu ý: Phải gọi Wire.begin(SDA, SCL) trước rtc.begin()!");
    while (1) {
      delay(1000);
      Serial.print(".");
    }
  }
  
  Serial.println("✓ DS1307 RTC found!");
  
  // Kiểm tra RTC có đang chạy không
  if (!rtc.isrunning()) {
    Serial.println("\n⚠️ WARNING: RTC is NOT running!");
    Serial.println("Setting time from compile time...");
    
    // Đặt thời gian từ thời gian compile
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("✓ Time set from compile time");
  } else {
    Serial.println("✓ RTC is running");
  }
  
  // Hiển thị thời gian hiện tại
  DateTime now = rtc.now();
  Serial.println("\n=== CURRENT TIME ===");
  printDateTime(now);
  Serial.println("\n===================\n");
  
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
  // Ví dụ: Đặt thời gian 2025-01-15 10:30:00
  // Format: DateTime(year, month, day, hour, minute, second)
  rtc.adjust(DateTime(2025, 1, 15, 10, 30, 0));
  Serial.println("Time set manually to: 2025-01-15 10:30:00");
}



